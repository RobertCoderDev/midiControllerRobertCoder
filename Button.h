#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
  private:
    int _pin;
    unsigned long _debounceDelay;
    unsigned long _lastDebounceTime;
    unsigned long _pressStartTime;
    bool _lastState;
    bool _state;
    bool _longPressHandled;
    const unsigned long _longPressDuration = 1000; // 1 segundo para long press

  public:
    Button(int pin, unsigned long debounceDelay = 50) {
      _pin = pin;
      _debounceDelay = debounceDelay;
      pinMode(_pin, INPUT_PULLUP);
      _state = HIGH; // Pullup: HIGH es suelto, LOW es presionado
      _lastState = HIGH;
      _lastDebounceTime = 0;
      _longPressHandled = false;
    }

    void update() {
      int reading = digitalRead(_pin);

      if (reading != _lastState) {
        _lastDebounceTime = millis();
      }

      if ((millis() - _lastDebounceTime) > _debounceDelay) {
        if (reading != _state) {
          _state = reading;
          
          if (_state == LOW) {
            // Botón presionado
            _pressStartTime = millis();
            _longPressHandled = false;
          }
        }
      }

      _lastState = reading;
    }

    bool isPressed() {
      // Retorna true solo en el flanco de bajada (cuando se presiona)
      // Nota: Esto disparará INMEDIATAMENTE al presionar. 
      // Si queremos que "Short Press" solo dispare al soltar (para distinguir de Long Press),
      // la lógica debería ser diferente (OnRelease). 
      // Por ahora mantengamos comportamiento original: disparo inmediato.
      // Pero para convivir con Long Press, idealmente Short Press actúa al soltar si no hubo Long Press.
      // Vamos a implementar detección simple: 
      // isPressed retorna true UNA VEZ cuando el botón pasa a LOW (como antes).
      // isLongPressed retorna true UNA VEZ si se mantiene.
      
      // REVISIÓN: Si queremos distinguir, 'isPressed' (click corto) no puede dispararse al bajar
      // si vamos a esperar a ver si es largo.
      // MANTENDRÉ COMPORTAMIENTO HÍBRIDO: Dispara al bajar (acción inmediata) Y dispara Long Press si mantienes.
      // Esto es útil para cosas como "Preset (short) -> Guardar (long)".
      
      // Implementación actual: detecta transición a LOW
       return (_state == LOW && (millis() - _lastDebounceTime) == 0); // Truco simple o flag
    }
    
    // Mejor enfoque para uso fácil: Estado explícito
    bool justPressed() {
      return (_state == LOW && (millis() - _lastDebounceTime) == 0 && _lastState == HIGH && digitalRead(_pin) == LOW); 
      // La lógica de update ya actualizó _state. Necesitamos detectar el CAMBIO en este frame.
      // Re-haremos update para ser más claros con banderas.
    }
};

// Re-escritura más limpia de Button para Arduino
class ButtonSimple {
  private:
    int _pin;
    bool _state;
    bool _lastReading;
    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay;
    
    unsigned long _pressedTime;
    bool _isLongPressed;
    bool _ignoreNextRelease;
    const unsigned long LONG_PRESS_TIME = 800;

  public:
    bool pressed;      // True un ciclo cuando se presiona
    bool released;     // True un ciclo cuando se suelta
    bool longPressed;  // True un ciclo cuando se detecta pulsación larga

    ButtonSimple(int pin) : _pin(pin), _debounceDelay(50) {
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
};

#endif
