//================================================================
//      Controlador MIDI Personalizado para Valeton GP-200 v7.0 (SoftSerial)
//================================================================
// Por: ROBERT CODER
// Refactorizado: Dual Comm (USB + SoftwareSerial)
//================================================================

#include <MIDI.h>
#include <SoftwareSerial.h>
#include "Button.h"
#include "LedManager.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "SerialCommander.h"
#include "MidiDictionary.h"

// --- CONFIGURACIÓN MIDI ---
MIDI_CREATE_DEFAULT_INSTANCE();

// --- BLUETOOTH (Software Serial) ---
// Usaremos A0 como RX (Recibe del TX del HC-06)
// Usaremos A1 como TX (Envía al RX del HC-06)
const int BT_RX_PIN = A0; 
const int BT_TX_PIN = A1;
SoftwareSerial btSerial(BT_RX_PIN, BT_TX_PIN);

// --- OBJETOS DE HARDWARE ---
// Definición de Pines
const int BTN_BANK_DOWN_PIN = 2;
const int BTN_BANK_UP_PIN = 4;
const int BTN_TOGGLE_PIN = 12; 
const int BTN_PRESET_1_PIN = 5;
const int BTN_PRESET_2_PIN = 6;
const int BTN_PRESET_3_PIN = 7;
const int BTN_GUITAR_CHANGE_PIN = 11;
const int BTN_CTRL_2_PIN = 3;

// Objetos Botones
Button btnBankDown(BTN_BANK_DOWN_PIN);
Button btnBankUp(BTN_BANK_UP_PIN);
Button btnToggle(BTN_TOGGLE_PIN);
Button btnPreset1(BTN_PRESET_1_PIN);
Button btnPreset2(BTN_PRESET_2_PIN);
Button btnPreset3(BTN_PRESET_3_PIN);
Button btnGuitarChange(BTN_GUITAR_CHANGE_PIN);
Button btnCtrl2(BTN_CTRL_2_PIN);

// Leds
const int ledPins[] = {8, 9, 10}; 
LedManager ledManager(ledPins, 3);

// Display
DisplayManager display(0x27, 16, 2);

// Configuration & Comm
// Configuration & Comm
ConfigManager configManager;
// Instanciamos dos comandantes separados para tener buffers independientes
// y evitar conflictos si llegan datos simultáneos por USB y BT.
SerialCommander commanderUSB(&configManager); 
SerialCommander commanderBT(&configManager);

// --- DATOS Y ESTADO ---
// --- DATOS Y ESTADO ---
// Las constantes NUM_BANKS etc vienen de ConfigManager.h

// Variables de Estado
int currentBank = 0;
int currentPresetIndex = -1;

// Historial para Toggle
int previousBank = 0;
int previousPresetIndex = -1;
int lastPresetBank = -1;
bool inToggleView = false;

// Estados adicionales
bool ctrl2Status = false;
bool ledStates[3] = {false, false, false}; 
// Global Effect States (for toggling via Long Press / Global buttons)
bool globalEffectStates[DICT_SIZE]; 

// --- SCROLL CONTROL ---
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_DELAY = 250; // ms entre saltos de banco 

// --- FUNCIONES AUXILIARES (Lógica de Negocio) ---

void refreshUI() {
    // 1. DISPLAY UPDATE (SAFE MODE)
    // Usamos buffers temporales para asegurar null-termination y evitar crashes por strings corruptos.
    char line1[17];
    char line2[17];
    memset(line1, 0, 17);
    memset(line2, 0, 17);

    if (inToggleView && previousPresetIndex != -1) {
        // --- TOGGLE VIEW ---
        ButtonConfig* currCfg = configManager.getButtonConfig(currentBank, currentPresetIndex);
        ButtonConfig* prevCfg = configManager.getButtonConfig(previousBank, previousPresetIndex);
        
        display.showToggleView(currCfg ? currCfg->name : "???", prevCfg ? prevCfg->name : "???");
    } else {
        // --- MAIN VIEW ---
        ButtonConfig* p1 = configManager.getButtonConfig(currentBank, 0);
        ButtonConfig* p2 = configManager.getButtonConfig(currentBank, 1);
        ButtonConfig* p3 = configManager.getButtonConfig(currentBank, 2);

        display.showMainView(
            "GP-200", 
            configManager.getBankName(currentBank),
            p1 ? p1->name : "---",
            p2 ? p2->name : "---",
            p3 ? p3->name : "---"
        );
    }
        
    // 2. LED LOGIC (ROBUST MODE)
    // Si estamos en un preset válido (0, 1, 2) y no en modo Toggle loco -> LED ENCENDIDO.
    // Ignoramos el "tipo" del boton 0, simplemente mostramos el estado actual.
    
    if (currentPresetIndex >= 0 && currentPresetIndex < 3) {
        // PRESET MODE: Solo 1 LED encendido (el actual)
        // Check extra: solo si estamos en el banco correcto (para latencia visual)
        if (lastPresetBank == currentBank) {
            ledManager.setExclusive(currentPresetIndex);
        } else {
            ledManager.setAllOff();
        }
    } else {
        // EFFECT MODE / OTHER: Mostrar estado individual
        for(int i=0; i<3; i++) {
            ledManager.setLed(i, ledStates[i]);
        }
    }
}

void triggerMidiAction(int presetIndex) {
    inToggleView = false;
    
    ButtonConfig* cmd = configManager.getButtonConfig(currentBank, presetIndex);
    if (!cmd) return;

    // Interpretamos la configuración
    if (cmd->type == 'P') {
        // --- PRESET MODE (PROGRAM CHANGE) ---
        // FIX: Solo actualizar historial si cambiamos a un preset DIFERENTE
        // Esto evita que "machaquemos" el historial si pulsamos el mismo botón 2 veces.
        bool isDifferent = (currentPresetIndex != presetIndex) || (lastPresetBank != currentBank);

        if (currentPresetIndex != -1 && isDifferent) {
            previousBank = lastPresetBank;
            previousPresetIndex = currentPresetIndex;
        } else if (currentPresetIndex == -1) {
            previousPresetIndex = -1;
        }
        
        currentPresetIndex = presetIndex;
        lastPresetBank = currentBank;
        
        // Value2 = Bank, Value1 = Program
        MIDI.sendControlChange(0, cmd->value2, 1);
        MIDI.sendProgramChange(cmd->value1, 1);
        
    } else if (cmd->type == 'D') {
        // --- DICTIONARY MODE (EFFECTS) ---
        // Toggle Effects logic
        ledStates[presetIndex] = !ledStates[presetIndex];
        
        // SYNC Global State too (if value1 is valid index)
        int idx = cmd->value1;
        if (idx >= 0 && idx < DICT_SIZE) {
             globalEffectStates[idx] = ledStates[presetIndex]; // Sync internal param state with LED state
        }
        
        int val = ledStates[presetIndex] ? 127 : 0;
        
        int cc = getCCFromDict(cmd->value1); // Value1 is index
        MIDI.sendControlChange(cc, val, 1);
        
    } else {
        // Custom
    }
    
    refreshUI();
}

void triggerLongPressAction(int presetIndex) {
    ButtonConfig* cmd = configManager.getButtonConfig(currentBank, presetIndex);
    if (!cmd) return;

    if (cmd->lpType == 'N') return; // Sin acción

    if (cmd->lpType == 'C') {
        // CC Custom
        MIDI.sendControlChange(cmd->lpValue1, cmd->lpValue2, 1);
    } else if (cmd->lpType == 'P') {
        // Program Change (Bank LSB only? Or just PC)
        // Usamos Value1=PC, Value2=Bank
        MIDI.sendControlChange(0, cmd->lpValue2, 1);
        MIDI.sendProgramChange(cmd->lpValue1, 1);
    } else if (cmd->lpType == 'D') {
        // Dict Effect
        int idx = cmd->lpValue1;
        if (idx >= 0 && idx < DICT_SIZE) {
            globalEffectStates[idx] = !globalEffectStates[idx]; // Toggle State
            int val = globalEffectStates[idx] ? 127 : 0;
            int cc = getCCFromDict(idx);
            MIDI.sendControlChange(cc, val, 1);
            
            // Visual feedback
            display.showMessage(getNameFromDict(idx), val ? "ON" : "OFF", 600);
            refreshUI();
        }
    }
}

void handleToggle() {
    if (previousPresetIndex == -1 || previousBank == -1) {
        // No hay historial válido, no hacemos nada
        return;
    }

    int tempBank = lastPresetBank; 
    int tempPreset = currentPresetIndex;

    currentBank = previousBank;
    currentPresetIndex = previousPresetIndex;
    lastPresetBank = previousBank;

    previousBank = tempBank;
    previousPresetIndex = tempPreset;
    
    // Re-enviar comando MIDI
    ButtonConfig* cmd = configManager.getButtonConfig(currentBank, currentPresetIndex);
    if (cmd && cmd->type == 'P') {
        MIDI.sendControlChange(0, cmd->value2, 1);
        MIDI.sendProgramChange(cmd->value1, 1);
    }
    
    inToggleView = true;
    refreshUI();
}

// --- SETUP & LOOP ---

void triggerGlobalAction(int globalId) {
    ButtonConfig* cmd = configManager.getGlobalConfig(globalId);
    if (!cmd) return;

    // Interpretamos la configuración
    if (cmd->type == 'P') {
        // --- PRESET MODE ---
        MIDI.sendControlChange(0, cmd->value2, 1);
        MIDI.sendProgramChange(cmd->value1, 1);
    } else if (cmd->type == 'D') {
        // --- EFFECT MODE ---
        // Para botones globales, toggleamos un estado interno local o simplemente enviamos trigger?
        // Como son "extra", asumiremos TOGGLE simple con estado efímero o trigger.
        // Simulamos toggle de 127/0 en cada pulsacion?
        // Mejor enviar 127 siempre (trigger) o CC value fijo?
        // El formato standard: value1=IndexDict.
        int cc = getCCFromDict(cmd->value1);
        // Enviamos toggle simple ciego: ON -> OFF (simulado con var estatica o simplemente 127??)
        // Por simplicidad en globales, mandamos 127 (ON). El usuario puede querer controlar loops.
        // Si queremos estado real necesitamos var de estado global.
        // Vamos a usar una estática fea aquí por ahora.
        static bool gState[2] = {false, false};
        gState[globalId] = !gState[globalId];
        MIDI.sendControlChange(cc, gState[globalId] ? 127 : 0, 1);
    }
}

// --- SETUP & LOOP ---

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); 
    digitalWrite(LED_BUILTIN, LOW);

    // 1. HARDWARE SERIAL (USB + MIDI) -> 31250
    // MIDI.begin() inicializa Serial a 31250 automáticamente.
    MIDI.begin(MIDI_CHANNEL_OMNI); 
    // Serial.begin(9600); // DEBUG ONLY
    
    // 2. SOFTWARE SERIAL (BLUETOOTH) -> 9600
    // Usamos la velocidad por defecto segura del HC-06.
    btSerial.begin(9600); 
    
    // --- BT CONFIGURATION (AUTO-RUN) ---
    // Configura Name y PIN al arrancar.
    // Una vez configurado, estas lineas se pueden comentar para ahorrar 2 segundos de inicio.
    delay(500); 
    btSerial.print("AT+NAMEMidiController"); 
    delay(1000); 
    btSerial.print("AT+PIN0290"); 
    delay(1000); 
    
    // Display Init
    display.begin();

    /* HARDWARE RESET DISABLED - CAUSING BOOT LOOP
    // HARDWARE FACTORY RESET CHECK
    // Explicitly set PULLUPs and wait a bit to avoid floating pins triggering false reset
    pinMode(BTN_BANK_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_BANK_DOWN_PIN, INPUT_PULLUP);
    delay(100); // 100ms stabilization

    // Si se mantienen presionados UP y DOWN al arrancar -> Reset
    if (digitalRead(BTN_BANK_UP_PIN) == LOW && digitalRead(BTN_BANK_DOWN_PIN) == LOW) {
        display.showCustom("FACTORY RESET...", " PLEASE WAIT ");
        configManager.resetToDefaults();
        configManager.save();
        delay(2000);
        display.showCustom(" RESET DONE ", " REBOOTING ");
        delay(1000);
    }
    */

    display.showWelcome();
    
    configManager.begin(); // Carga de EEPROM
    
    refreshUI();
}

unsigned long lastHeartbeat = 0;

// --- DEBUG LOOP ---
void loop() {
    // 0. HEARTBEAT (Debug suave)
    if (millis() - lastHeartbeat > 2000) {
        lastHeartbeat = millis();
        // Serial.println(F("DEBUG:HEARTBEAT"));
    }

    // 0.1 MIDI READ - PERMANENTLY DISABLED
    // We prioritize App Commands (Text). MIDI.read() consumes characters and breaks commands.
    // If we need MIDI IN in the future, we must use a separate port or advanced parsing.
    /*
    if (MIDI.read()) {
    }
    */

    // 1. ESCUCHAR COMANDOS DE LA APP
    bool configChanged = false;
    // Usamos instancias separadas para cada puerto
    if (commanderUSB.update(Serial)) configChanged = true;
    if (commanderBT.update(btSerial)) configChanged = true;
    
    if (configChanged) {
        // CRASH FIX: Update currentBank if we deleted the last one
        if (currentBank >= configManager.getActiveBanksCount()) {
            currentBank = configManager.getActiveBanksCount() - 1;
            if (currentBank < 0) currentBank = 0; // Safety for 1 bank
        }
        refreshUI();
    }
    
    // 2. Update Hardware
    // Global Cooldown Check to prevent "stacking"
    static unsigned long lastActionTime = 0;
    const unsigned long ACTION_COOLDOWN = 300; 

    btnBankUp.update();
    btnBankDown.update();
    btnToggle.update();
    btnPreset1.update();
    btnPreset2.update();
    btnPreset3.update();
    btnGuitarChange.update(); 
    btnCtrl2.update();        
    
    // Si hace menos de 300ms que hicimos algo, ignoramos nuevas acciones
    if (millis() - lastActionTime < ACTION_COOLDOWN) return;

    // 3. LOGICA PERFORMANCE
    
    // Cambios de Banco Global (Short Press)
    if (btnBankUp.pressed) {
        lastActionTime = millis(); // Reset cooldown
        currentBank++;
        if (currentBank >= configManager.getActiveBanksCount()) currentBank = 0; 
        inToggleView = false;
        refreshUI();
    }
    
    if (btnBankDown.pressed) {
        lastActionTime = millis();
        currentBank--;
        if (currentBank < 0) currentBank = configManager.getActiveBanksCount() - 1;
        inToggleView = false;
        refreshUI();
    }
    
    // --- BANK SCROLL LONG PRESS ---
    // DISABLED: User rule "Solo los preseset 1,2,3 usan long press/toggle"
    // (Intentionally removed scrolling logic)

    // Toggle short press
    // Toggle short press
    if (btnToggle.pressed) {
        lastActionTime = millis();
        handleToggle();
    }
    // Toggle Long Press
    // DISABLED: User rule
    if (btnToggle.longPressed) {
        // Do nothing
    }
    
    // Presets Short
    if (btnPreset1.pressed) { lastActionTime = millis(); triggerMidiAction(0); }
    if (btnPreset2.pressed) { lastActionTime = millis(); triggerMidiAction(1); }
    if (btnPreset3.pressed) { lastActionTime = millis(); triggerMidiAction(2); }
    
    // Presets Long
    if (btnPreset1.longPressed) { lastActionTime = millis(); triggerLongPressAction(0); }
    if (btnPreset2.longPressed) { lastActionTime = millis(); triggerLongPressAction(1); }
    if (btnPreset3.longPressed) { lastActionTime = millis(); triggerLongPressAction(2); }
    
    // Globales
    if (btnGuitarChange.pressed) { lastActionTime = millis(); triggerGlobalAction(0); }
    if (btnCtrl2.pressed)        { lastActionTime = millis(); triggerGlobalAction(1); }
}
