#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definición de caracteres personalizados
const byte notaMusical[8] = { B00110, B00101, B00101, B00100, B01100, B01100, B00000, B00000 };
const byte notaMusical2[8] = { 0b00001, 0b00011, 0b00101, 0b01001, 0b01001, 0b01011, 0b11011, 0b11000 };

class DisplayManager {
  private:
    LiquidCrystal_I2C _lcd;

  public:
    DisplayManager(uint8_t addr, uint8_t cols, uint8_t rows) : _lcd(addr, cols, rows) {
      // Constructor vacío
    }

    void begin() {
      _lcd.init();
      _lcd.backlight();
      _lcd.createChar(0, (uint8_t*)notaMusical);
      _lcd.createChar(1, (uint8_t*)notaMusical2);
    }

    void showWelcome() {
      _lcd.clear();
      _lcd.setCursor(0, 0); _lcd.print("MIDI Controller");
      _lcd.setCursor(0, 1); _lcd.print("Valeton GP-200");
      delay(2000);
      _lcd.clear();
      _lcd.setCursor(0, 0); _lcd.print("BY");
      _lcd.setCursor(0, 1); _lcd.print("ROBERT CODER");
      delay(2000);
      _lcd.clear();
      _lcd.setCursor(0, 0); _lcd.print("HI ROBERT ");
      delay(700); _lcd.write((byte)0); _lcd.print(" ");
      delay(700); _lcd.write((byte)1); _lcd.print(" ");
      delay(700); _lcd.write((byte)0);
      delay(2500);
    }

    void showMainView(const char* guitarName, const char* bankName, const char* p1, const char* p2, const char* p3) {
      _lcd.clear();
      _lcd.setCursor(0, 0); _lcd.print(guitarName); _lcd.print(": "); _lcd.print(bankName);
      _lcd.setCursor(0, 1); _lcd.print(p1);
      _lcd.setCursor(6, 1); _lcd.print(p2);
      _lcd.setCursor(12, 1); _lcd.print(p3);
    }

    void showToggleView(const char* currentName, const char* previousName) {
      _lcd.clear();
      _lcd.setCursor(0, 0); _lcd.print("[");
      _lcd.setCursor(1, 0); _lcd.print(currentName);
      _lcd.setCursor(5, 0); _lcd.print("]");
      _lcd.setCursor(7, 0); _lcd.print("<=>");
      _lcd.setCursor(11, 0); _lcd.print(previousName);
    }
    
    // Nuevo: Feedback temporal para acciones como Long Press
    void showMessage(const char* line1, const char* line2, int duration) {
        _lcd.clear();
        _lcd.setCursor(0,0); _lcd.print(line1);
        _lcd.setCursor(0,1); _lcd.print(line2);
        delay(duration);
    }

    // Nuevo: Mostrar texto custom directo (para Menú)
    void showCustom(const char* line1, const char* line2) {
        _lcd.clear();
        _lcd.setCursor(0,0); _lcd.print(line1);
        _lcd.setCursor(0,1); _lcd.print(line2);
    }
};

#endif
