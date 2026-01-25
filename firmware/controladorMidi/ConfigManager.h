#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <EEPROM.h>
#include <Arduino.h>

// Definición de Configuración por Botón
struct ButtonConfig {
    char name[5];     // Nombre de 4 letras + null terminator
    char type;        // 'P' (Preset), 'D' (Dictionary/Effect), 'C' (Custom raw CC)
    byte value1;      // Si 'P': ProgramNum. Si 'D': DictIndex. Si 'C': CC Number.
    byte value2;      // Si 'P': BankNum. Si 'C': Value (0=Toggle).
    
    // --- NUEVO: Configuración Long Press ---
    char lpType;      // 'N' (None), 'C' (CC), 'P' (Program), 'D' (Dict)
    byte lpValue1;
    byte lpValue2;
};

// Estructura global de datos
// Eliminamos Guitarras, Single Context.
const int MAX_BANKS_CFG = 4; // REDUCED FROM 10 TO SAVE RAM (CRITICAL)
const int NUM_PRESETS_CFG = 3; 

// Magic number actualizado para forzar reset de estructura
// Magic number actualizado para forzar reset de estructura por nuevos campos LP
const int EEPROM_MAGIC = 12350; // Bump version to force Reset (1 Bank Default)

class ConfigManager {
  private:
    int _eepromStartAddress;
    int _activeBanks; // Variable en RAM
  
  public:
    // Matriz de configuraciones en RAM [Bank][Preset]
    // Reservamos memoria para el máximo posible
    ButtonConfig configs[MAX_BANKS_CFG][NUM_PRESETS_CFG];
    
    // Configuraciones Globales (0=Lateral/Guitar, 1=Central/Ctrl2)
    ButtonConfig globalConfigs[2];
    
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

        // Cargar Botones de Banco
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                EEPROM.get(addr, configs[b][p]);
                addr += sizeof(ButtonConfig);
            }
        }
        
        // Cargar Globales
        for (int i = 0; i < 2; i++) {
            EEPROM.get(addr, globalConfigs[i]);
            addr += sizeof(ButtonConfig);
        }
        
        // Cargar Nombres de Banco
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            EEPROM.get(addr, bankNames[b]);
            addr += 9; // 8 chars + null
        }
    }

    void save() {
        int addr = _eepromStartAddress;
        // Magic + Count
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
        
        // Guardar Globales
        for (int i = 0; i < 2; i++) {
            EEPROM.put(addr, globalConfigs[i]);
            addr += sizeof(ButtonConfig);
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
        
        // Init Global Defaults
        // 0: Lateral (Antes Guitar) -> Default PC ?? or Empty
        snprintf(globalConfigs[0].name, 5, "LAT");
        globalConfigs[0].type = 'P'; 
        globalConfigs[0].value1 = 0; 
        globalConfigs[0].value2 = 0;
        globalConfigs[0].lpType = 'N'; 
        globalConfigs[0].lpValue1 = 0;
        globalConfigs[0].lpValue2 = 0;
        
        // 1: Central (Antes Ctrl2) -> Default
        snprintf(globalConfigs[1].name, 5, "CEN");
        globalConfigs[1].type = 'P'; 
        globalConfigs[1].value1 = 0; 
        globalConfigs[1].value2 = 0;
        globalConfigs[1].lpType = 'N';
        globalConfigs[1].lpValue1 = 0;
        globalConfigs[1].lpValue2 = 0;
    }
    
    void initBank(int b) {
        snprintf(bankNames[b], 9, "BANK %d", b);
        for (int p = 0; p < NUM_PRESETS_CFG; p++) {
            snprintf(configs[b][p].name, 5, "P%d-%d", b, p);
            configs[b][p].type = 'P';
            configs[b][p].value1 = (b * 3) + p; 
            configs[b][p].value2 = 0;
            
            // Init Long Press (None by default)
            configs[b][p].lpType = 'N'; 
            configs[b][p].lpValue1 = 0;
            configs[b][p].lpValue2 = 0;       
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
    
    // Delete specific bank and shift down
    bool removeBank(int index) {
        if (_activeBanks <= 1 || index < 0 || index >= _activeBanks) return false;

        // Shift everything down from index + 1
        for (int b = index; b < _activeBanks - 1; b++) {
             // Copy Bank b+1 into Bank b
             for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                 configs[b][p] = configs[b+1][p];
             }
             strncpy(bankNames[b], bankNames[b+1], 9);
        }

        _activeBanks--;
        
        // CLEANUP: Wipe the old last bank (now unused) to avoid confusion
        // if re-added later or accessed by mistake.
        initBank(_activeBanks); // Reset to default Px-y logic
        snprintf(bankNames[_activeBanks], 9, "EMPTY"); // Mark explicitly
        
        save();
        return true;
    }

    // Legacy/Default (Delete Last)
    bool removeBankLast() {
        if (_activeBanks > 1) {
            _activeBanks--;
            
            // Cleanup
            initBank(_activeBanks);
            snprintf(bankNames[_activeBanks], 9, "EMPTY");
            
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
    
    // Accessor para globales
    ButtonConfig* getGlobalConfig(int index) {
        if (index >= 0 && index < 2) {
            return &globalConfigs[index];
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
