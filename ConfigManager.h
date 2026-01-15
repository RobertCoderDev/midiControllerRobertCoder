#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <EEPROM.h>
#include <Arduino.h>

// Definición de Configuración por Botón
struct ButtonConfig {
    char name[5];     // Nombre de 4 letras + null terminator (ej. "SOLO")
    char type;        // 'P' (Preset), 'D' (Dictionary/Effect), 'C' (Custom raw CC)
    byte value1;      // Si 'P': ProgramNum. Si 'D': DictIndex. Si 'C': CC Number.
    byte value2;      // Si 'P': BankNum. Si 'C': Value (0=Toggle).
    // Nota: Para simplificar, en 'D' (Dict) asumimos toggle (0/127) o trigger (127) según lógica.
};

// Estructura global de datos
const int NUM_GUITARS_CFG = 2;
const int NUM_BANKS_CFG = 4;
const int NUM_PRESETS_CFG = 3; 

// Magic number para verificar si la EEPROM está inicializada
const int EEPROM_MAGIC = 12345; 

class ConfigManager {
  private:
    int _eepromStartAddress;
  
  public:
    // Matriz de configuraciones en RAM
    ButtonConfig configs[NUM_GUITARS_CFG][NUM_BANKS_CFG][NUM_PRESETS_CFG];

    ConfigManager() {
        _eepromStartAddress = 0;
    }

    void begin() {
        int magic;
        EEPROM.get(_eepromStartAddress, magic);
        if (magic != EEPROM_MAGIC) {
            // EEPROM vacía o corrupta: Cargar defaults y guardar
            resetToDefaults();
            save();
        } else {
            load();
        }
    }

    void load() {
        int addr = _eepromStartAddress + sizeof(int); // Saltar magic number
        for (int g = 0; g < NUM_GUITARS_CFG; g++) {
            for (int b = 0; b < NUM_BANKS_CFG; b++) {
                for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                    EEPROM.get(addr, configs[g][b][p]);
                    addr += sizeof(ButtonConfig);
                }
            }
        }
    }

    void save() {
        int addr = _eepromStartAddress;
        EEPROM.put(addr, EEPROM_MAGIC);
        addr += sizeof(int);
        
        for (int g = 0; g < NUM_GUITARS_CFG; g++) {
            for (int b = 0; b < NUM_BANKS_CFG; b++) {
                for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                    EEPROM.put(addr, configs[g][b][p]);
                    addr += sizeof(ButtonConfig);
                }
            }
        }
    }

    void resetToDefaults() {
        // Cargar valores por defecto (similar a lo que estaba hardcoded)
        // Solo un ejemplo básico para rellenar todo, luego el usuario edita.
        for (int g = 0; g < NUM_GUITARS_CFG; g++) {
            for (int b = 0; b < NUM_BANKS_CFG; b++) {
                for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                    // Default: Presets simples
                    sprintf(configs[g][b][p].name, "P%d-%d", b, p);
                    configs[g][b][p].type = 'P';
                    configs[g][b][p].value1 = (b * 3) + p; // Program Number secuencial
                    configs[g][b][p].value2 = 0;           // Bank 0
                    
                    // Ejemplo: Banco 2 (Efectos) ponerlo como Effects del diccionario
                    if (b == 2) {
                        configs[g][b][p].type = 'D'; // Dictionary
                        
                        if (p == 0) {
                           strcpy(configs[g][b][p].name, "DIST");
                           configs[g][b][p].value1 = 0; // Index 0 (DIST)
                        } else if (p == 1) {
                           strcpy(configs[g][b][p].name, "DLY");
                           configs[g][b][p].value1 = 3; // Index 3 (DLY)
                        } else {
                           strcpy(configs[g][b][p].name, "MOD");
                           configs[g][b][p].value1 = 2; // Index 2 (MOD)
                        }
                    }
                }
            }
        }
    }
    
    ButtonConfig* getButtonConfig(int guitar, int bank, int preset) {
        if (guitar >= 0 && guitar < NUM_GUITARS_CFG && 
            bank >= 0 && bank < NUM_BANKS_CFG && 
            preset >= 0 && preset < NUM_PRESETS_CFG) {
            return &configs[guitar][bank][preset];
        }
        return nullptr;
    }
};

#endif
