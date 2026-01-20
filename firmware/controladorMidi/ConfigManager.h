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
// Eliminamos Guitarras, Single Context.
const int MAX_BANKS_CFG = 10; // LIMITE MAXIMO 10
const int NUM_PRESETS_CFG = 3; 

// Magic number actualizado para forzar reset de estructura
const int EEPROM_MAGIC = 12348; // Bump version for dynamic banks

class ConfigManager {
  private:
    int _eepromStartAddress;
    int _activeBanks; // Variable en RAM
  
  public:
    // Matriz de configuraciones en RAM [Bank][Preset]
    // Reservamos memoria para el máximo posible
    ButtonConfig configs[MAX_BANKS_CFG][NUM_PRESETS_CFG];
    
    // Nombres de Bancos (Max 8 chars + null)
    char bankNames[MAX_BANKS_CFG][9];

    ConfigManager() {
        _eepromStartAddress = 0;
        _activeBanks = 1;
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
        
        // Cargar Count
        EEPROM.get(addr, _activeBanks);
        addr += sizeof(int);
        
        // Validar rango por si aca
        if (_activeBanks < 1) _activeBanks = 1;
        if (_activeBanks > MAX_BANKS_CFG) _activeBanks = MAX_BANKS_CFG;

        // Cargar Botones
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                EEPROM.get(addr, configs[b][p]);
                addr += sizeof(ButtonConfig);
            }
        }
        
        // Cargar Nombres de Banco
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            EEPROM.get(addr, bankNames[b]);
            addr += 9; // 8 chars + null
        }
    }

    void save() {
        int addr = _eepromStartAddress;
        EEPROM.put(addr, EEPROM_MAGIC);
        addr += sizeof(int);
        
        EEPROM.put(addr, _activeBanks);
        addr += sizeof(int);
        
        // Guardar Botones
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                EEPROM.put(addr, configs[b][p]);
                addr += sizeof(ButtonConfig);
            }
        }
        
        // Guardar Nombres de Banco
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            EEPROM.put(addr, bankNames[b]);
            addr += 9;
        }
    }

    // Default: Reset to 1 bank
    void resetToDefaults() {
        _activeBanks = 1; // Solo 1 banco por defecto
        
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            initBank(b);
        }
    }
    
    void initBank(int b) {
        snprintf(bankNames[b], 9, "BANK %d", b);
        for (int p = 0; p < NUM_PRESETS_CFG; p++) {
            snprintf(configs[b][p].name, 5, "P%d-%d", b, p);
            configs[b][p].type = 'P';
            configs[b][p].value1 = (b * 3) + p; 
            configs[b][p].value2 = 0;       
        }
    }
    
    // --- Dynamic Management ---
    
    int getActiveBanksCount() {
        return _activeBanks;
    }
    
    bool addBank() {
        if (_activeBanks < MAX_BANKS_CFG) {
            // Inicializar el nuevo banco antes de activarlo
            initBank(_activeBanks); 
            _activeBanks++;
            save(); // Persistir cambio
            return true;
        }
        return false;
    }
    
    bool removeBank() {
        if (_activeBanks > 1) {
            _activeBanks--;
            save();
            return true;
        }
        return false;
    }
    
    ButtonConfig* getButtonConfig(int bank, int preset) {
        if (bank >= 0 && bank < MAX_BANKS_CFG && 
            preset >= 0 && preset < NUM_PRESETS_CFG) {
            return &configs[bank][preset];
        }
        return nullptr;
    }
    
    char* getBankName(int bank) {
         if (bank >= 0 && bank < MAX_BANKS_CFG) {
            return bankNames[bank];
         }
         return (char*)"ERR";
    }
    
    void setBankName(int bank, const char* name) {
        if (bank >= 0 && bank < MAX_BANKS_CFG) {
            strncpy(bankNames[bank], name, 8);
            bankNames[bank][8] = '\0'; // Ensure null term
        }
    }
};

#endif
