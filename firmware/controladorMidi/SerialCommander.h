
#ifndef SERIALCOMMANDER_H
#define SERIALCOMMANDER_H

#include <Arduino.h>
#include "ConfigManager.h"

// Buffer para entrada serial
const int SC_BUFFER_SIZE = 50;

class SerialCommander {
  private:
    ConfigManager* _config;
    Stream* _btStream; // Canal dedicado al Bluetooth
    char _inputBuffer[SC_BUFFER_SIZE];
    int _bufferIndex;

    // Moved sendAllConfig to be before processCommand as per snippet,
    // but keeping processCommand private as it was originally.
    void sendAllConfig(Stream& port) {
        port.println(F("BEGIN:CONFIG"));
        // 1. Enviar Info de Bancos
        // Protocolo: BANK:ID:NAME
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            port.print(F("BANK:"));
            port.print(b); port.print(F(":"));
            port.println(_config->getBankName(b));
            delay(5);
        }

        // 2. Enviar Datos de Botones
        // Protocolo: DATA:B:P:NAME:TYPE:V1:V2
        for (int b = 0; b < NUM_BANKS_CFG; b++) {
            for (int p = 0; p < NUM_PRESETS_CFG; p++) {
                ButtonConfig* btn = _config->getButtonConfig(b, p);
                port.print(F("DATA:"));
                port.print(b); port.print(F(":"));
                port.print(p); port.print(F(":"));
                port.print(btn->name); port.print(F(":"));
                port.print(btn->type); port.print(F(":"));
                port.print(btn->value1); port.print(F(":"));
                port.println(btn->value2);
                delay(5);
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
            
        } else if (strcmp(token, "SAVE") == 0) {
            // SAVE:B:P:NAME:TYPE:V1:V2
            char* sBank   = strtok(NULL, ":");
            char* sPreset = strtok(NULL, ":");
            char* sName   = strtok(NULL, ":");
            char* sType   = strtok(NULL, ":");
            char* sVal1   = strtok(NULL, ":");
            char* sVal2   = strtok(NULL, ":");
            
            if (sBank && sPreset && sName && sType && sVal1 && sVal2) {
                int b = atoi(sBank);
                int p = atoi(sPreset);
                char t = sType[0];
                int v1 = atoi(sVal1);
                int v2 = atoi(sVal2);
                
                ButtonConfig* btn = _config->getButtonConfig(b, p);
                if (btn) {
                    strncpy(btn->name, sName, 4);
                    btn->name[4] = 0; 
                    btn->type = t;
                    btn->value1 = v1;
                    btn->value2 = v2;
                    
                    _config->save();
                    port.println(F("OK:SAVED"));
                    return true; 
                } 
            }
            port.println(F("ERR:SAVE_FAIL"));

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
        }

        return false;
    }

  public:
    SerialCommander(ConfigManager* config, Stream* btStream = nullptr) {
        _config = config;
        _btStream = btStream;
        _bufferIndex = 0;
    }

    bool update(Stream& port) {
        bool changed = false;
        while (port.available() > 0) {
            char inChar = (char)port.read();
            
            // Visual Feedback: Blink LED on RX
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            
            // DEBUG EXTREME: Ver qué carajos llega
            // port.print(F("DBGC:")); port.println((int)inChar);

            if (inChar == '\n' || inChar == '\r') {
                // Al recibir Enter, procesamos
                if (_bufferIndex > 0) {
                    _inputBuffer[_bufferIndex] = 0; // Null terminate
                    
                    // DEBUG: Avisar que intentamos procesar
                    // port.print(F("DBG:EOL_DETECTED:")); port.println(_inputBuffer);
                    
                    if (processCommand(_inputBuffer, port)) {
                        changed = true;
                    }
                    _bufferIndex = 0;
                }
            } else {
                if (_bufferIndex < SC_BUFFER_SIZE - 1) {
                    _inputBuffer[_bufferIndex] = inChar;
                    _bufferIndex++;
                } else {
                    // Buffer Overflow protection
                     _bufferIndex = 0; // Reset safe
                     port.println(F("ERR:BUFF_OVF"));
                }
            }
        }
        return changed; // Retorna true si hubo cambios (SAVE)
    }
};

#endif
