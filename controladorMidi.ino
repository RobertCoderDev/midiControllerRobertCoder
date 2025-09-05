//================================================================
//      Controlador MIDI Personalizado para Valeton GP-200
//================================================================
// Por: ROBERT CODER
// Versión: 3.1.0
//================================================================

// --- INCLUIR BIBLIOTECAS ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MIDI.h>

// --- CONFIGURACIÓN DE PANTALLA E INICIALIZACIÓN MIDI ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
MIDI_CREATE_DEFAULT_INSTANCE();

// --- DEFINICIÓN DE SÍMBOLOS PERSONALIZADOS ---
byte notaMusical[8] = { B00110, B00101, B00101, B00100, B01100, B01100, B00000, B00000 };
byte notaMusical2[8] = { 0b00001, 0b00011, 0b00101, 0b01001, 0b01001, 0b01011, 0b11011, 0b11000 };

//****************************************************************
//              <<<<< ÁREA DE PERSONALIZACIÓN FÁCIL >>>>>
//****************************************************************
const int NUM_GUITARS = 2;
const int NUM_BANKS = 4;

const char* guitarNames[NUM_GUITARS] = {"IBANEZ", "EPIPHONE"};
const char* bankNames[NUM_BANKS] = {"LEAD", "AMP", "EFECTOS", "TABLET"};

const char* presetNames[NUM_GUITARS][NUM_BANKS][3] = {
  // GUITARRA 0: IBANEZ
  {
    {"SOLO", "AMBF", "DLYP"},
    {"VOX", "FEND", "MESA"},
    {"WAHA", "CTRL", "DLY"},
    {"ANT", "NOTA", "SIG"}
  },
  // GUITARRA 1: EPIPHONE
  {
    {"SOLO", "AMBF", "DLYP"},
    {"VOX", "FEND", "DR.Z"},
    {"WAHA", "CTRL", "DLY"},
    {"ANT", "NOTA", "SIG"}
  }
};

struct MidiCommand { char type; int bank; int number; };

MidiCommand midiCommands[NUM_GUITARS][NUM_BANKS][3] = {
  // GUITARRA 0: IBANEZ
  {
    {{'P', 0, 4},  {'P', 0, 1},  {'P', 0, 2}},     // Banco LEAD
    {{'P', 0, 9},  {'P', 0, 8},  {'P', 0, 11}},    // Banco AMP
    {{'C', 0, 57}, {'C', 0, 71}, {'C', 0, 55}},    // Banco EFECTOS
    {{'T', 0, 103},{'T', 0, 104},{'T', 0, 102}}    // Banco TABLET
  },
  // GUITARRA 1: EPIPHONE
  {
    {{'P', 0, 20}, {'P', 0, 21}, {'P', 0, 22}},    // Banco LEAD
    {{'P', 1, 34}, {'P', 1, 11}, {'P', 1, 12}},    // Banco AMP
    {{'C', 0, 57}, {'C', 0, 71}, {'C', 0, 55}},    // Banco EFECTOS
    {{'T', 0, 103},{'T', 0, 104},{'T', 0, 102}}    // Banco TABLET
  }
};
//****************************************************************
//            <<<<< FIN DEL ÁREA DE PERSONALIZACIÓN >>>>>
//****************************************************************

// --- PINES DE HARDWARE ---
const int BTN_BANK_DOWN = 2, BTN_TOGGLE = 12, BTN_BANK_UP = 4;
const int BTN_PRESET_1 = 5, BTN_PRESET_2 = 6, BTN_PRESET_3 = 7;
const int LED_PRESET_1 = 8, LED_PRESET_2 = 9, LED_PRESET_3 = 10;
const int BTN_GUITAR_CHANGE = 11;
const int BTN_CTRL_2 = 3;

const int buttonPins[] = {BTN_BANK_DOWN, BTN_TOGGLE, BTN_BANK_UP, BTN_PRESET_1, BTN_PRESET_2, BTN_PRESET_3, BTN_GUITAR_CHANGE, BTN_CTRL_2};
const int NUM_BUTTONS = 8; // Eran 7, ahora son 8

// --- VARIABLES GLOBALES ---
int currentGuitar = 0, previousGuitar = 0;
int currentBank = 0, previousBank = 0;
int currentPresetIndex = -1, previousPresetIndex = -1;
int lastPresetBank = -1;
bool ledStatus[3] = {false, false, false};
bool previousLedStatus[3] = {false, false, false};
bool ctrl2Status = false;

bool buttonStates[NUM_BUTTONS], lastButtonStates[NUM_BUTTONS];
const long debounceDelay = 200;
bool inToggleView = false;


// --- v3.1.0 AÑADIDO: Nueva función para manejar exclusivamente el botón CTRL 2 ---
void handleCtrl2() {
  // 1. Invertimos el estado del control
  ctrl2Status = !ctrl2Status;
  int value = ctrl2Status ? 127 : 0;
  MIDI.sendControlChange(70, value, 1);

}

void setup() {
  MIDI.begin(1);
  lcd.init();
  lcd.backlight();

  lcd.createChar(0, notaMusical);
  lcd.createChar(1, notaMusical2);

  // --- Secuencia de Bienvenida ---
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("MIDI Controller");
  lcd.setCursor(0, 1); lcd.print("Valeton GP-200");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("BY");
  lcd.setCursor(0, 1); lcd.print("ROBERT CODER");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("HI ROBERT ");
  delay(700); lcd.write((byte)0); lcd.print(" ");
  delay(700); lcd.write((byte)1); lcd.print(" ");
  delay(700); lcd.write((byte)0);
  delay(2500);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = HIGH;
  }
  pinMode(LED_PRESET_1, OUTPUT); pinMode(LED_PRESET_2, OUTPUT); pinMode(LED_PRESET_3, OUTPUT);
  updateDisplay(); updateLeds();
}

void loop() {
  readButtons();
}

void readButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonStates[i] = digitalRead(buttonPins[i]);
    if (buttonStates[i] == LOW && lastButtonStates[i] == HIGH) {
      delay(debounceDelay);
      switch (buttonPins[i]) {
        case BTN_TOGGLE:          performToggle(); break;
        case BTN_GUITAR_CHANGE:   inToggleView = false; currentGuitar = (currentGuitar + 1) % NUM_GUITARS; currentBank = 0; resetLedStatus(); updateDisplay(); break;
        case BTN_BANK_UP:         inToggleView = false; currentBank = (currentBank + 1) % NUM_BANKS; resetLedStatus(); updateDisplay(); break;
        case BTN_BANK_DOWN:       inToggleView = false; currentBank = (currentBank - 1 + NUM_BANKS) % NUM_BANKS; resetLedStatus(); updateDisplay(); break;
        case BTN_PRESET_1:        handlePreset(0); break;
        case BTN_PRESET_2:        handlePreset(1); break;
        case BTN_PRESET_3:        handlePreset(2); break;
        case BTN_CTRL_2:          handleCtrl2(); break;
      }
    }
    lastButtonStates[i] = buttonStates[i];
  }
}

void handlePreset(int presetIndex) {
  inToggleView = false;
  MidiCommand cmd = midiCommands[currentGuitar][currentBank][presetIndex];

  if (cmd.type == 'P') {
    if (currentPresetIndex != -1) {
      previousGuitar = currentGuitar;
      previousBank = lastPresetBank;
      previousPresetIndex = currentPresetIndex;
    } else {
      previousPresetIndex = -1;
    }
    currentPresetIndex = presetIndex;
    lastPresetBank = currentBank;
    MIDI.sendControlChange(0, cmd.bank, 1);
    MIDI.sendProgramChange(cmd.number, 1);
    for (int i = 0; i < 3; i++) { ledStatus[i] = false; }
    ledStatus[presetIndex] = true;

  } else {
    currentPresetIndex = -1;
    previousPresetIndex = -1;
    lastPresetBank = -1;

    if (cmd.type == 'C') {
      ledStatus[presetIndex] = !ledStatus[presetIndex];
      int value = ledStatus[presetIndex] ? 127 : 0;
      MIDI.sendControlChange(cmd.number, value, 1);
    } else if (cmd.type == 'T') {
      MIDI.sendControlChange(cmd.number, 127, 1);
      digitalWrite(LED_PRESET_1 + presetIndex, HIGH);
      delay(100);
      digitalWrite(LED_PRESET_1 + presetIndex, LOW);
    }
  }

  updateLeds();
  updateDisplay();
}

void performToggle() {
  if (previousPresetIndex == -1) return;

  int tempGuitar = currentGuitar;
  int tempBank = lastPresetBank;
  int tempPresetIndex = currentPresetIndex;

  currentGuitar = previousGuitar;
  currentBank = previousBank;
  currentPresetIndex = previousPresetIndex;
  lastPresetBank = previousBank;

  previousGuitar = tempGuitar;
  previousBank = tempBank;
  previousPresetIndex = tempPresetIndex;

  MidiCommand cmd = midiCommands[currentGuitar][currentBank][currentPresetIndex];
  if (cmd.type == 'P') {
    MIDI.sendControlChange(0, cmd.bank, 1);
    MIDI.sendProgramChange(cmd.number, 1);
  }

  inToggleView = true;
  updateLeds();
  updateDisplay();
}

void updateLeds() {
    if (inToggleView) {
        for (int i=0; i<3; i++) { ledStatus[i] = false; }
        if (currentGuitar == previousGuitar && currentBank == previousBank) {
            ledStatus[currentPresetIndex] = true;
        } else {
        }
    }
    digitalWrite(LED_PRESET_1, ledStatus[0]);
    digitalWrite(LED_PRESET_2, ledStatus[1]);
    digitalWrite(LED_PRESET_3, ledStatus[2]);
}

void resetLedStatus() {
  for (int i = 0; i < 3; i++) { ledStatus[i] = false; }
  updateLeds();
}

void updateDisplay() {
  lcd.clear();
  if (inToggleView && previousPresetIndex != -1) {
    const char* currentName = presetNames[currentGuitar][currentBank][currentPresetIndex];
    const char* previousName = presetNames[previousGuitar][previousBank][previousPresetIndex];

    lcd.setCursor(0, 0); lcd.print("[");
    lcd.setCursor(1, 0); lcd.print(currentName);
    lcd.setCursor(5, 0); lcd.print("]");
    lcd.setCursor(7, 0); lcd.print("<=>");
    lcd.setCursor(11, 0); lcd.print(previousName);

  } else {
    lcd.setCursor(0, 0); lcd.print(guitarNames[currentGuitar]); lcd.print(": "); lcd.print(bankNames[currentBank]);
    lcd.setCursor(0, 1); lcd.print(presetNames[currentGuitar][currentBank][0]);
    lcd.setCursor(6, 1); lcd.print(presetNames[currentGuitar][currentBank][1]);
    lcd.setCursor(12, 1); lcd.print(presetNames[currentGuitar][currentBank][2]);
  }
}
