#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
  private:
    int _pin;
    bool _state;
    bool _lastReading;
    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay;
    
    unsigned long _pressedTime;
    bool _isLongPressed;
    bool _ignoreNextRelease;
    const unsigned long LONG_PRESS_TIME = 1000;

  public:
    bool pressed;      // True un ciclo cuando se presiona
    bool released;     // True un ciclo cuando se suelta
    bool longPressed;  // True un ciclo cuando se detecta pulsación larga

    Button(int pin) : _pin(pin), _debounceDelay(50) {
      pinMode(_pin, INPUT_PULLUP);
      _state = HIGH;
      _lastReading = HIGH;
      _ignoreNextRelease = false;
    }

    void update() {
      bool reading = digitalRead(_pin);
      pressed = false;
      released = false;
      longPressed = false;

      if (reading != _lastReading) {
        _lastDebounceTime = millis();
      }

      if ((millis() - _lastDebounceTime) > _debounceDelay) {
        if (reading != _state) {
          _state = reading;

          if (_state == LOW) {
            // Flanco descendente (Presionado)
            _pressedTime = millis();
            _isLongPressed = false;
            _ignoreNextRelease = false;
            // Opcional: Si queremos acción inmediata al pulsar (sin esperar a soltar)
            // pressed = true; 
          } else {
            // Flanco ascendente (Soltado)
            if (!_ignoreNextRelease) {
               pressed = true; // Consideramos "Click" al soltar si no fue Long Press
            }
            released = true;
          }
        }
      }
      
      // Chequeo Long Press continuo mientras está presionado
      if (_state == LOW && !_isLongPressed && (millis() - _pressedTime > LONG_PRESS_TIME)) {
        _isLongPressed = true;
        longPressed = true;
        _ignoreNextRelease = true; // Para no disparar 'pressed' (click corto) al soltar
      }

      _lastReading = reading;
    }

    // Nuevo método para verificar si el botón está mantenido pulsado (sin debounce complex)
    // Útil para scroll continuo
    bool isDown() {
        return _state == LOW;
    }

    bool isLongPressedState() {
        return _isLongPressed;
    }
};

#endif
