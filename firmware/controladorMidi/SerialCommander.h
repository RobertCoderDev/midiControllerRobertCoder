
#ifndef SERIALCOMMANDER_H
#define SERIALCOMMANDER_H

#include <Arduino.h>
#include "ConfigManager.h"

// Buffer para entrada serial
const int SC_BUFFER_SIZE = 40; // Reduced to save RAM

class SerialCommander {
  private:
    ConfigManager* _config;
    char _inputBuffer[SC_BUFFER_SIZE];
    int _bufferIndex;

    // Moved sendAllConfig to be before processCommand as per snippet,
    // but keeping processCommand private as it was originally.
    void sendAllConfig(Stream& port) {
        port.println(F("BEGIN:CONFIG"));
        port.print(F("BANK_COUNT:"));
        port.println(_config->getActiveBanksCount());
        // 1. Enviar Info de Bancos
        // Protocolo: BANK:ID:NAME
        for (int b = 0; b < MAX_BANKS_CFG; b++) {
            port.print(F("BANK:"));
            port.print(b); port.print(F(":"));
            port.println(_config->getBankName(b));
            delay(5);
        }

        // 2b. Enviar Datos Globales
        // Protocolo: DATAGLO:ID:NAME:TYPE:V1:V2
        for(int i=0; i<2; i++) {
             ButtonConfig* btn = _config->getGlobalConfig(i);
                port.print(F("DATAGLO:"));
                port.print(i); port.print(F(":"));
                port.print(btn->name); port.print(F(":"));
                port.print(btn->type); port.print(F(":"));
                port.print(btn->value1); port.print(F(":"));
                port.println(btn->value2);
                delay(5);
        }

        // 2. Enviar Datos de Botones
        // Protocolo: DATA:B:P:NAME:TYPE:V1:V2
        for (int b = 0; b < _config->getActiveBanksCount(); b++) { // Use active
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                ButtonConfig* btn = _config->getButtonConfig(b, p);
                port.print(F("DATA:"));
                port.print(b); port.print(F(":"));
                port.print(p); port.print(F(":"));
                port.print(btn->name); port.print(F(":"));
                port.print(btn->type); port.print(F(":"));
                port.print(btn->value1); port.print(F(":"));
                port.print(btn->value2); port.print(F(":"));
                port.print(btn->lpType); port.print(F(":"));
                port.print(btn->lpValue1); port.print(F(":"));
                port.print(btn->lpValue2); port.println();
                delay(10); // Aumentado para dar respiro a la App
            }
        }
        port.println(F("END:CONFIG"));
    }

    bool processCommand(char* cmd, Stream& port) {
        // Formato esperado: CMD:ARG1:ARG2...
        char* token = strtok(cmd, ":");
        if (!token) return false;

        if (strcmp(token, "HELLO") == 0) {
            port.println(F("READY:GP200_CONTROLLER_V3")); // Version bumped
            return false;
            
        } else if (strcmp(token, "GETALL") == 0) {
            sendAllConfig(port);
            return false;
        
        } else if (strcmp(token, "ADDBANK") == 0) {
             if (_config->addBank()) {
                 port.println(F("OK:BANK_ADDED"));
             } else {
                 port.println(F("ERR:MAX_BANKS"));
             }
             return true; 
             
        } else if (strcmp(token, "DELBANK") == 0) {
             char* sId = strtok(NULL, ":");
             bool success = false;
             
             if (sId) {
                 int idx = atoi(sId);
                 success = _config->removeBank(idx);
             } else {
                 // Backward compatibility (borrar ultimo)
                 success = _config->removeBankLast();
             }

             if (success) {
                  port.println(F("OK:BANK_REMOVED"));
                  return true; 
             } else {
                  port.println(F("ERR:MIN_BANKS"));
             }
             return true;
            
        } else if (strcmp(token, "SAVE") == 0) {
            // SAVE:B:P:NAME:TYPE:V1:V2:LPT:LPV1:LPV2
            char* sBank   = strtok(NULL, ":");
            char* sPreset = strtok(NULL, ":");
            char* sName   = strtok(NULL, ":");
            char* sType   = strtok(NULL, ":");
            char* sVal1   = strtok(NULL, ":");
            char* sVal2   = strtok(NULL, ":");
            // Optional/New args
            char* sLpType = strtok(NULL, ":");
            char* sLpV1   = strtok(NULL, ":");
            char* sLpV2   = strtok(NULL, ":");
            
            if (sBank && sPreset && sName && sType && sVal1 && sVal2) {
                int b = atoi(sBank);
                int p = atoi(sPreset);
                char t = sType[0];
                int v1 = atoi(sVal1);
                int v2 = atoi(sVal2);
                
                ButtonConfig* btn = _config->getButtonConfig(b, p);
                if (btn) {
                    strncpy(btn->name, sName, 4);
                    btn->name[4] = 0; // Ensure null term
                    btn->type = t;
                    btn->value1 = v1;
                    btn->value2 = v2;
                    
                    // Update LP fields if provided
                    if (sLpType && sLpV1 && sLpV2) {
                        btn->lpType = sLpType[0];
                        btn->lpValue1 = atoi(sLpV1);
                        btn->lpValue2 = atoi(sLpV2);
                    } else {
                        // Default if missing
                        btn->lpType = 'N';
                        btn->lpValue1 = 0;
                        btn->lpValue2 = 0;
                    }
                    
                    _config->save();
                    port.println(F("OK:SAVED"));
                    return true; 
                } 
            }
            port.println(F("ERR:SAVE_FAIL"));

        } else if (strcmp(token, "SAVEGLO") == 0) {
            // SAVEGLO:ID:NAME:TYPE:V1:V2
            char* sID     = strtok(NULL, ":");
            char* sName   = strtok(NULL, ":");
            char* sType   = strtok(NULL, ":");
            char* sVal1   = strtok(NULL, ":");
            char* sVal2   = strtok(NULL, ":");
            
            if (sID && sName && sType && sVal1 && sVal2) {
                int id = atoi(sID);
                char t = sType[0];
                int v1 = atoi(sVal1);
                int v2 = atoi(sVal2);
                
                ButtonConfig* btn = _config->getGlobalConfig(id);
                if (btn) {
                    strncpy(btn->name, sName, 4);
                    btn->name[4] = 0; 
                    btn->type = t;
                    btn->value1 = v1;
                    btn->value2 = v2;
                    
                    _config->save();
                    port.println(F("OK:SAVED_GLO"));
                    return true; 
                } 
            }
            port.println(F("ERR:SAVE_GLO_FAIL"));

        } else if (strcmp(token, "SAVEBANK") == 0) {
             // SAVEBANK:B:NAME
             char* sBank = strtok(NULL, ":");
             char* sName = strtok(NULL, ":");
             
             if (sBank && sName) {
                 int b = atoi(sBank);
                 _config->setBankName(b, sName);
                 _config->save();
                 port.println(F("OK:BANK_RENAMED"));
                 return true; // Refrescar UI (título banco)
             }
        } else if (strcmp(token, "RESET") == 0) {
             _config->resetToDefaults();
             _config->save();
             port.println(F("OK:RESET_DONE"));
             return true;  
        }

        return false;
    }

  public:
    SerialCommander(ConfigManager* config) {
        _config = config;
        _bufferIndex = 0;
        // Inicializar buffer limpio
        memset(_inputBuffer, 0, SC_BUFFER_SIZE);
    }

    bool update(Stream& port) {
        bool changed = false;
        while (port.available() > 0) {
            char inChar = (char)port.read();
            
            // Visual Feedback: Blink LED on RX
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            
            if (inChar == '\n' || inChar == '\r') {
                // Al recibir Enter, procesamos
                if (_bufferIndex > 0) {
                    _inputBuffer[_bufferIndex] = 0; // Null terminate
                    
                    if (processCommand(_inputBuffer, port)) {
                        changed = true;
                    }
                    _bufferIndex = 0;
                    memset(_inputBuffer, 0, SC_BUFFER_SIZE); // Clean buffer after process
                }
            } else {
                if (_bufferIndex < SC_BUFFER_SIZE - 1) {
                    _inputBuffer[_bufferIndex] = inChar;
                    _bufferIndex++;
                } else {
                    // Buffer Overflow protection
                     _bufferIndex = 0; 
                     memset(_inputBuffer, 0, SC_BUFFER_SIZE); // Force reset
                     port.println(F("ERR:BUFF_OVF"));
                }
            }
        }
        return changed; // Retorna true si hubo cambios (SAVE)
    }
};

#endif
