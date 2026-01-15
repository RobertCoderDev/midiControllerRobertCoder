#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include <Arduino.h>

class LedManager {
  private:
    int* _pins;
    int _count;
    bool* _states;

  public:
    LedManager(const int pins[], int count) {
        _count = count;
        _pins = new int[_count];
        _states = new bool[_count];
        for (int i = 0; i < _count; i++) {
            _pins[i] = pins[i];
            pinMode(_pins[i], OUTPUT);
            _states[i] = false;
            digitalWrite(_pins[i], LOW);
        }
    }

    void setLed(int index, bool state) {
      if (index >= 0 && index < _count) {
        _states[index] = state;
        digitalWrite(_pins[index], state ? HIGH : LOW);
      }
    }

    void setAllOff() {
      for (int i = 0; i < _count; i++) {
        setLed(i, false);
      }
    }

    void setExclusive(int index) {
      setAllOff();
      setLed(index, true);
    }
    
    // Efecto de parpadeo simple
    void blink(int index, int duration) {
        if(index >= 0 && index < _count) {
             setLed(index, true);
             delay(duration);
             setLed(index, false);
        }
    }
};

#endif
