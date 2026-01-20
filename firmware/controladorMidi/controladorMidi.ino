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
ButtonSimple btnBankDown(BTN_BANK_DOWN_PIN);
ButtonSimple btnBankUp(BTN_BANK_UP_PIN);
ButtonSimple btnToggle(BTN_TOGGLE_PIN);
ButtonSimple btnPreset1(BTN_PRESET_1_PIN);
ButtonSimple btnPreset2(BTN_PRESET_2_PIN);
ButtonSimple btnPreset3(BTN_PRESET_3_PIN);
ButtonSimple btnGuitarChange(BTN_GUITAR_CHANGE_PIN);
ButtonSimple btnCtrl2(BTN_CTRL_2_PIN);

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
SerialCommander commanderUSB(&configManager, &btSerial); 
SerialCommander commanderBT(&configManager, &btSerial);

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

// --- FUNCIONES AUXILIARES (Lógica de Negocio) ---

void refreshUI() {
    if (inToggleView && previousPresetIndex != -1) {
        // En modo toggle
        ButtonConfig* currCfg = configManager.getButtonConfig(currentBank, currentPresetIndex);
        ButtonConfig* prevCfg = configManager.getButtonConfig(previousBank, previousPresetIndex);
        
        display.showToggleView(currCfg ? currCfg->name : "???", prevCfg ? prevCfg->name : "???");
        ledManager.setExclusive(currentPresetIndex); 
    } else {
        // Vista Principal
        ButtonConfig* p1 = configManager.getButtonConfig(currentBank, 0);
        ButtonConfig* p2 = configManager.getButtonConfig(currentBank, 1);
        ButtonConfig* p3 = configManager.getButtonConfig(currentBank, 2);

        // Usamos el nombre del banco desde ConfigManager (EEPROM)
        display.showMainView(
            "GP-200", // Título fijo o personalizado 
            configManager.getBankName(currentBank),
            p1 ? p1->name : "---",
            p2 ? p2->name : "---",
            p3 ? p3->name : "---"
        );
        
        // LEDs logic
        ButtonConfig* firstBtn = configManager.getButtonConfig(currentBank, 0);
        char type = firstBtn ? firstBtn->type : 'P';
        
        if (type == 'P') {
            if (currentPresetIndex != -1 && lastPresetBank == currentBank) {
                 ledManager.setExclusive(currentPresetIndex);
            } else {
                 ledManager.setAllOff();
            }
        } else {
             // Effect mode logic
             for(int i=0; i<3; i++) ledManager.setLed(i, ledStates[i]);
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
        if (currentPresetIndex != -1) {
            previousBank = lastPresetBank;
            previousPresetIndex = currentPresetIndex;
        } else {
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
        int val = ledStates[presetIndex] ? 127 : 0;
        
        int cc = getCCFromDict(cmd->value1); // Value1 is index
        MIDI.sendControlChange(cc, val, 1);
        
    } else {
        // Custom
    }
    
    refreshUI();
}

void handleToggle() {
    if (previousPresetIndex == -1) return;

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

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); 
    digitalWrite(LED_BUILTIN, LOW);

    // inicializar MIDI primero (esto pone el puerto a 31250)
    MIDI.begin(1);

    // Configuración BT (independiente del MIDI)
    btSerial.begin(9600);  
    
    // ... codigo de init bt ...
    
    display.begin();
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

    // 1. ESCUCHAR COMANDOS DE LA APP
    bool configChanged = false;
    // Usamos instancias separadas para cada puerto
    if (commanderUSB.update(Serial)) configChanged = true;
    if (commanderBT.update(btSerial)) configChanged = true;
    
    if (configChanged) {
        refreshUI();
    }
    
    // 2. Update Hardware
    btnBankUp.update();
    btnBankDown.update();
    btnToggle.update();
    btnPreset1.update();
    btnPreset2.update();
    btnPreset3.update();
    
    // 3. LOGICA PERFORMANCE
    
    // Cambios de Banco Global
    if (btnBankUp.pressed) {
        currentBank++;
        if (currentBank >= NUM_BANKS_CFG) currentBank = 0; // Usar contante de ConfigManager
        inToggleView = false;
        refreshUI();
    }
    
    if (btnBankDown.pressed) {
        currentBank--;
        if (currentBank < 0) currentBank = NUM_BANKS_CFG - 1;
        inToggleView = false;
        refreshUI();
    }

    // Toggle short press
    if (btnToggle.pressed) {
        handleToggle();
    }
    
    // Presets
    if (btnPreset1.pressed) triggerMidiAction(0);
    if (btnPreset2.pressed) triggerMidiAction(1);
    if (btnPreset3.pressed) triggerMidiAction(2);
}
