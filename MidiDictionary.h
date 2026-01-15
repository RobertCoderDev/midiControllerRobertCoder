#ifndef MIDIDICTIONARY_H
#define MIDIDICTIONARY_H

#include <Arduino.h>

struct MidiDefinition {
  const char* label;
  byte cc;
};

// Diccionario de funciones comunes para Valeton GP-200
const int DICT_SIZE = 14; 
const MidiDefinition midiDictionary[DICT_SIZE] = {
  {"DIST",  49}, // Distortion Module
  {"AMP",   50}, // Amp Module
  {"MOD",   54}, // Modulation Module
  {"DLY",   55}, // Delay Module
  {"REV",   56}, // Reverb Module
  {"WAH",   57}, // Wah Module
  {"TUNER", 58}, // Tuner
  {"LOOP",  59}, // Looper On/Off
  {"L.REC", 60}, // Looper Record
  {"L.PLY", 62}, // Looper Play/Stop
  {"CTRL1", 69}, // CTRL 1
  {"CTRL2", 70}, // CTRL 2
  {"CTRL3", 71}, // CTRL 3
  {"TAP",   75}  // Tap Tempo
};

// Función helper para obtener CC por índice o nombre (opcional)
int getCCFromDict(int index) {
  if (index >= 0 && index < DICT_SIZE) {
    return midiDictionary[index].cc;
  }
  return 0;
}

const char* getNameFromDict(int index) {
  if (index >= 0 && index < DICT_SIZE) {
    return midiDictionary[index].label;
  }
  return "???";
}

#endif
