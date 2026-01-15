//================================================================
//      Controlador MIDI Personalizado para Valeton GP-200 v5.0
//================================================================
// Por: ROBERT CODER
// Refactorizado a POO + Long Press + Configurable (EEPROM/Menu)
//================================================================

#include <MIDI.h>
#include "Button.h"
#include "LedManager.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "MenuManager.h"
#include "MidiDictionary.h"

// --- CONFIGURACIÓN MIDI ---
MIDI_CREATE_DEFAULT_INSTANCE();

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

// Configuration & Menu
ConfigManager configManager;
MenuManager menuManager(&display, &configManager);

// --- DATOS Y ESTADO ---
const int NUM_GUITARS = 2;
const int NUM_BANKS = 4;
const char* guitarNames[NUM_GUITARS] = {"IBANEZ", "EPIPHONE"};
const char* bankNames[NUM_BANKS] = {"LEAD", "AMP", "EFECTOS", "TABLET"};

// Variables de Estado
int currentGuitar = 0;
int currentBank = 0;
int currentPresetIndex = -1;

// Historial para Toggle
int previousGuitar = 0;
int previousBank = 0;
int previousPresetIndex = -1;
int lastPresetBank = -1;
bool inToggleView = false;

// Estados adicionales
bool ctrl2Status = false;
bool ledStates[3] = {false, false, false}; // Estado lógico para toggles tipo 'C'

// --- FUNCIONES AUXILIARES (Lógica de Negocio) ---

void refreshUI() {
    // Si estamos editando, NO refrescar la UI principal
    if (menuManager.isEditing()) return;

    if (inToggleView && previousPresetIndex != -1) {
        // En modo toggle, necesitamos nombres. Los leemos de la config guardada.
        ButtonConfig* currCfg = configManager.getButtonConfig(currentGuitar, currentBank, currentPresetIndex);
        ButtonConfig* prevCfg = configManager.getButtonConfig(previousGuitar, previousBank, previousPresetIndex);
        
        display.showToggleView(currCfg ? currCfg->name : "???", prevCfg ? prevCfg->name : "???");
        ledManager.setExclusive(currentPresetIndex); 
    } else {
        ButtonConfig* p1 = configManager.getButtonConfig(currentGuitar, currentBank, 0);
        ButtonConfig* p2 = configManager.getButtonConfig(currentGuitar, currentBank, 1);
        ButtonConfig* p3 = configManager.getButtonConfig(currentGuitar, currentBank, 2);

        display.showMainView(
            guitarNames[currentGuitar], 
            bankNames[currentBank],
            p1 ? p1->name : "---",
            p2 ? p2->name : "---",
            p3 ? p3->name : "---"
        );
        
        // LEDs logic
        ButtonConfig* firstBtn = configManager.getButtonConfig(currentGuitar, currentBank, 0);
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
    if (menuManager.isEditing()) return; // No enviar MIDI editando

    inToggleView = false;
    
    ButtonConfig* cmd = configManager.getButtonConfig(currentGuitar, currentBank, presetIndex);
    if (!cmd) return;

    // Interpretamos la configuración
    if (cmd->type == 'P') {
        // --- PRESET MODE (PROGRAM CHANGE) ---
        if (currentPresetIndex != -1) {
            previousGuitar = currentGuitar;
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
        // Custom / Other - Not implemented in detail yet
    }
    
    refreshUI();
}

void handleToggle() {
    if (menuManager.isEditing()) return;
    if (previousPresetIndex == -1) return;

    int tempGuitar = currentGuitar;
    int tempBank = lastPresetBank; 
    int tempPreset = currentPresetIndex;

    currentGuitar = previousGuitar;
    currentBank = previousBank;
    currentPresetIndex = previousPresetIndex;
    lastPresetBank = previousBank;

    previousGuitar = tempGuitar;
    previousBank = tempBank;
    previousPresetIndex = tempPreset;
    
    // Re-enviar comando MIDI del estado restaurado
    ButtonConfig* cmd = configManager.getButtonConfig(currentGuitar, currentBank, currentPresetIndex);
    if (cmd && cmd->type == 'P') {
        MIDI.sendControlChange(0, cmd->value2, 1);
        MIDI.sendProgramChange(cmd->value1, 1);
    }
    
    inToggleView = true;
    refreshUI();
}

// --- SETUP & LOOP ---

void setup() {
    MIDI.begin(1);
    display.begin();
    display.showWelcome();
    
    configManager.begin(); // Carga de EEPROM
    
    refreshUI();
}

void loop() {
    // Update Hardware
    btnBankUp.update();
    btnBankDown.update();
    btnToggle.update();
    btnPreset1.update();
    btnPreset2.update();
    btnPreset3.update();
    
    // --- MODE: MENU EDITING ---
    if (menuManager.isEditing()) {
        // Mapeo especial de botones para el menú
        if (btnBankUp.pressed)   menuManager.handleInput(1);  // Next Item / Inc
        if (btnBankDown.pressed) menuManager.handleInput(-1); // Prev Item / Dec
        if (btnToggle.pressed)   menuManager.handleInput(10); // Enter / Next Field
        if (btnToggle.longPressed) menuManager.exitMenu(true); // SAVE & EXIT
        
        // Usar presets para valores también
        if (btnPreset1.pressed)  menuManager.handleInput(-1); // Value -
        if (btnPreset2.pressed)  menuManager.handleInput(1);  // Value +
        if (btnPreset3.pressed)  menuManager.handleInput(1); // Value ++ ??
        
        return; // IMPORTANTE: No ejecutar lógica de performance si estamos editando
    }

    // --- MODE: PERFORMANCE (NORMAL) ---
    
    // Cambios de Banco Global
    if (btnBankUp.pressed) {
        currentBank++;
        if (currentBank >= NUM_BANKS) currentBank = 0;
        inToggleView = false;
        refreshUI();
    }
    
    if (btnBankDown.pressed) {
        currentBank--;
        if (currentBank < 0) currentBank = NUM_BANKS - 1;
        inToggleView = false;
        refreshUI();
    }

    // Toggle short press
    if (btnToggle.pressed) {
        handleToggle();
    }
    
    // ** TOGGLE LONG PRESS -> ENTER MENU **
    if (btnToggle.longPressed) {
        menuManager.enterMenu(currentGuitar, currentBank);
    }

    // Presets
    if (btnPreset1.pressed) triggerMidiAction(0);
    if (btnPreset2.pressed) triggerMidiAction(1);
    if (btnPreset3.pressed) triggerMidiAction(2);
}
