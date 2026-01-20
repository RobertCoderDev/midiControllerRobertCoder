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
// Estructura global de datos
// Eliminamos Guitarras, Single Context.
const int NUM_BANKS_CFG = 8; // Aumentamos a 8 bancos disponibles
const int NUM_PRESETS_CFG = 3; 

// Magic number actualizado para forzar reset de estructura
const int EEPROM_MAGIC = 12347; 

class ConfigManager {
  private:
    int _eepromStartAddress;
  
  public:
    // Matriz de configuraciones en RAM [Bank][Preset]
    ButtonConfig configs[NUM_BANKS_CFG][NUM_PRESETS_CFG];
    
    // Nombres de Bancos (Max 8 chars + null)
    char bankNames[NUM_BANKS_CFG][9];

    ConfigManager() {
        _eepromStartAddress = 0;
    }

    void begin() {
        int magic;
        EEPROM.get(_eepromStartAddress, magic);
        if (magic != EEPROM_MAGIC) {
            resetToDefaults();
            save();
        } else {
            load();
        }
    }

    void load() {
        int addr = _eepromStartAddress + sizeof(int); // Saltar magic number
        
        // Cargar Botones
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                EEPROM.get(addr, configs[b][p]);
                addr += sizeof(ButtonConfig);
            }
        }
        
        // Cargar Nombres de Banco
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            EEPROM.get(addr, bankNames[b]);
            addr += 9; // 8 chars + null
        }
    }

    void save() {
        int addr = _eepromStartAddress;
        EEPROM.put(addr, EEPROM_MAGIC);
        addr += sizeof(int);
        
        // Guardar Botones
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                EEPROM.put(addr, configs[b][p]);
                addr += sizeof(ButtonConfig);
            }
        }
        
        // Guardar Nombres de Banco
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            EEPROM.put(addr, bankNames[b]);
            addr += 9;
        }
    }

    void resetToDefaults() {
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            // Default Name
            sprintf(bankNames[b], "BANK %d", b);
            
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                // Default: Presets simples
                sprintf(configs[b][p].name, "P%d-%d", b, p);
                configs[b][p].type = 'P';
                configs[b][p].value1 = (b * 3) + p; 
                configs[b][p].value2 = 0;       
                
                // Ejemplo: Banco 2 (Antes Efectos)
                if (b == 2) {
                    strcpy(bankNames[b], "EFECTOS");
                    configs[b][p].type = 'D'; 
                    if (p == 0) {
                       strcpy(configs[b][p].name, "DIST"); configs[b][p].value1 = 0;
                    } else if (p == 1) {
                       strcpy(configs[b][p].name, "DLY"); configs[b][p].value1 = 3;
                    } else {
                       strcpy(configs[b][p].name, "MOD"); configs[b][p].value1 = 2;
                    }
                }
            }
        }
    }
    
    ButtonConfig* getButtonConfig(int bank, int preset) {
        if (bank >= 0 && bank < NUM_BANKS_CFG && 
            preset >= 0 && preset < NUM_PRESETS_CFG) {
            return &configs[bank][preset];
        }
        return nullptr;
    }
    
    char* getBankName(int bank) {
         if (bank >= 0 && bank < NUM_BANKS_CFG) {
            return bankNames[bank];
         }
         return "ERR";
    }
    
    void setBankName(int bank, const char* name) {
        if (bank >= 0 && bank < NUM_BANKS_CFG) {
            strncpy(bankNames[bank], name, 8);
            bankNames[bank][8] = '\0'; // Ensure null term
        }
    }
};

#endif
