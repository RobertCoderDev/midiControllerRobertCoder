#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include "DisplayManager.h"
#include "ConfigManager.h"
#include "MidiDictionary.h"
#include <Arduino.h> // Necesario para sprintf

enum MenuState {
    MENU_OFF,
    MENU_SELECT_SLOT, // Eligiendo qué botón editar (G:0 B:0 P:0)
    MENU_EDIT_NAME,   // Editando nombre caracter por caracter
    MENU_EDIT_TYPE,   // Eligiendo Tipo (Preset, Dict, Custom)
    MENU_EDIT_VAL1,   // Editando Valor 1 (ProgNum o DictIndex)
    MENU_EDIT_VAL2    // Editando Valor 2 (BankNum o nada)
};

class MenuManager {
  private:
    DisplayManager* _display;
    ConfigManager* _config;
    
    MenuState _state;
    
    // Variables temporales de edición
    int _editGuitar;
    int _editBank;
    int _editPreset;
    
    ButtonConfig _tempConfig; // Copia temporal para editar
    int _cursorPos; // Para editar nombre (0-3)

  public:
    bool isActive;

    MenuManager(DisplayManager* display, ConfigManager* config) {
        _display = display;
        _config = config;
        _state = MENU_OFF;
        isActive = false;
        _editGuitar = 0;
        _editBank = 0;
        _editPreset = 0;
    }

    void enterMenu(int currentGuitar, int currentBank) {
        _state = MENU_SELECT_SLOT;
        isActive = true;
        _editGuitar = currentGuitar;
        _editBank = currentBank;
        _editPreset = 0;
        
        // Cargar config actual en temp
        ButtonConfig* original = _config->getButtonConfig(_editGuitar, _editBank, _editPreset);
        if (original) _tempConfig = *original;
        
        updateDisplay();
    }

    void exitMenu(bool save) {
        if (save) {
             // Guardar cambios en RAM y EEPROM
             ButtonConfig* target = _config->getButtonConfig(_editGuitar, _editBank, _editPreset);
             if (target) *target = _tempConfig;
             _config->save();
             _display->showMessage("CONFIGURATION", "SAVED!", 1500);
        } else {
             _display->showMessage("EDIT MODE", "CANCELLED", 1000);
        }
        
        _state = MENU_OFF;
        isActive = false;
    }

    void handleInput(int btnCode) {
        // btnCode: 
        // 1=Up(Next Item/Val), -1=Down(Prev Item/Val)
        // 10=Enter/NextStep
        // 20=Back/Exit (Opcional)
        
        switch (_state) {
            case MENU_SELECT_SLOT:
                if (btnCode == 1) { // Next Preset Slot
                    _editPreset++;
                    if (_editPreset > 2) _editPreset = 0;
                    // Recargar temp config al cambiar slot
                    ButtonConfig* original = _config->getButtonConfig(_editGuitar, _editBank, _editPreset);
                    if(original) _tempConfig = *original;
                } else if (btnCode == -1) { // Prev Slot
                    _editPreset--;
                    if (_editPreset < 0) _editPreset = 2;
                    ButtonConfig* original = _config->getButtonConfig(_editGuitar, _editBank, _editPreset);
                    if(original) _tempConfig = *original;
                } else if (btnCode == 10) { // Enter -> Edit Name
                     _state = MENU_EDIT_NAME;
                     _cursorPos = 0;
                }
                break;
                
            case MENU_EDIT_NAME:
                if (btnCode == 1) { // Next Char
                    _tempConfig.name[_cursorPos]++;
                    if (_tempConfig.name[_cursorPos] > 'Z') _tempConfig.name[_cursorPos] = ' '; // Loop chars simple
                    if (_tempConfig.name[_cursorPos] == '!' ) _tempConfig.name[_cursorPos] = 'A'; // Espacio a A
                } else if (btnCode == -1) { // Prev Char
                     _tempConfig.name[_cursorPos]--;
                     if (_tempConfig.name[_cursorPos] < ' ') _tempConfig.name[_cursorPos] = 'Z';
                } else if (btnCode == 10) { // Enter -> Next Cursor or Next Step
                    _cursorPos++;
                    if (_cursorPos >= 4) {
                        _state = MENU_EDIT_TYPE;
                    }
                }
                break;
                
            case MENU_EDIT_TYPE:
                if (btnCode == 1 || btnCode == -1) {
                    // Cycle types: P -> D -> P
                    if (_tempConfig.type == 'P') _tempConfig.type = 'D';
                    else _tempConfig.type = 'P';
                    
                    // Reset defaults for new type
                    if(_tempConfig.type == 'P') { _tempConfig.value1 = 0; _tempConfig.value2 = 0; }
                    if(_tempConfig.type == 'D') { _tempConfig.value1 = 0; } // Index 0
                } else if (btnCode == 10) {
                    _state = MENU_EDIT_VAL1;
                }
                break;
                
            case MENU_EDIT_VAL1:
                if (btnCode == 1) { // Inc Value
                    _tempConfig.value1++;
                    if (_tempConfig.type == 'D' && _tempConfig.value1 >= DICT_SIZE) _tempConfig.value1 = 0;
                    if (_tempConfig.type == 'P' && _tempConfig.value1 > 99) _tempConfig.value1 = 0;
                } else if (btnCode == -1) { // Dec Value
                    _tempConfig.value1--; // Be careful with unsigned byte wrap
                    if (_tempConfig.type == 'D' && _tempConfig.value1 > 200) _tempConfig.value1 = DICT_SIZE - 1; // Wrap
                    if (_tempConfig.type == 'P' && _tempConfig.value1 > 200) _tempConfig.value1 = 99;
                } else if (btnCode == 10) {
                     if (_tempConfig.type == 'P') _state = MENU_EDIT_VAL2;
                     else exitMenu(true); // Dictionary terminamos aqui
                }
                break;
                
            case MENU_EDIT_VAL2: // Bank for Presets
                 if (btnCode == 1) {
                     _tempConfig.value2++;
                     if (_tempConfig.value2 > 99) _tempConfig.value2 = 0;
                 } else if (btnCode == -1) {
                     _tempConfig.value2--;
                     if (_tempConfig.value2 > 200) _tempConfig.value2 = 99;
                 } else if (btnCode == 10) {
                     exitMenu(true);
                 }
                 break;
        }
        
        if (isActive) updateDisplay();
    }
    
    void updateDisplay() {
        char line1[17];
        char line2[17];
        
        switch (_state) {
            case MENU_SELECT_SLOT:
                sprintf(line1, "EDIT BUTTON %d", _editPreset + 1);
                sprintf(line2, "NOW: %s", _tempConfig.name);
                break;
            case MENU_EDIT_NAME:
                sprintf(line1, "NAME: %s", _tempConfig.name);
                // Indicar cursor visualmente es dificil sin funcion blink cursor custom, 
                // usaremos corchetes moviles
                strcpy(line2, "    ");
                line2[_cursorPos] = '^';
                line2[_cursorPos+1] = 0;
                break;
            case MENU_EDIT_TYPE:
                sprintf(line1, "TYPE: %s", (_tempConfig.type == 'P' ? "PRESET" : "EFFECT"));
                sprintf(line2, "CHANGE [UP/DN]");
                break;
            case MENU_EDIT_VAL1:
                if (_tempConfig.type == 'P') {
                    sprintf(line1, "PROG NUM: %d", _tempConfig.value1);
                } else {
                    sprintf(line1, "FX: %s", getNameFromDict(_tempConfig.value1));
                }
                sprintf(line2, "[TOGGLE]=OK");
                break;
             case MENU_EDIT_VAL2:
                sprintf(line1, "BANK NUM: %d", _tempConfig.value2);
                sprintf(line2, "LONG PRS=SAVE");
                break;
        }
        
        if (_state != MENU_OFF) {
            _display->showCustom(line1, line2);
        }
    }
    
    // Getter para que main loop sepa si estamos editando
    bool isEditing() { return isActive; }
};

#endif
