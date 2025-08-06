//================================================================
//     Controlador MIDI Personalizado para Valeton GP-200
//================================================================
// Por: ROBERT CODER
// Versión: 2.2.0
//================================================================

// --- INCLUIR BIBLIOTECAS ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MIDI.h>

// --- CONFIGURACIÓN DE PANTALLA ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- INICIALIZACIÓN MIDI ---
MIDI_CREATE_DEFAULT_INSTANCE();


// --- DEFINICIÓN DE SÍMBOLOS PERSONALIZADOS ---
byte notaMusical[8] = {
  B00110,
  B00101,
  B00101,
  B00100,
  B01100,
  B01100,
  B00000,
  B00000
};

byte notaMusical2[8] = {
0b00001,
0b00011,
0b00101,
0b01001,
0b01001,
0b01011,
0b11011,
0b11000
};




//****************************************************************
//           <<<<< ÁREA DE PERSONALIZACIÓN FÁCIL >>>>>
//****************************************************************

// 1. DEFINE LOS NOMBRES DE GUITARRAS, BANCOS Y PRESETS
const int NUM_GUITARS = 2;
const int NUM_BANKS = 4;

const char* guitarNames[NUM_GUITARS] = {"IBANEZ", "EPIPHONE"};
const char* bankNames[NUM_BANKS] = {"SOLOS", "AMP", "EFECTOS", "TABLET"};

// Nombres de los presets (máximo 4 caracteres)
const char* presetNames[NUM_GUITARS][NUM_BANKS][3] = {
  // GUITARRA 0: IBANEZ
  {
    {"SOLO", "AMBF", "DLYP"}, // Banco "SOLOS"
    {"VOX", "FEND", "DR.Z"}, // Banco "AMP"
    {"WAHA", "CTRL", "DLY"},    // Banco "EFECTOS"
    {"ANT", "NOTA", "SIG"} // Banco "TABLET"
  },
  // GUITARRA 1: EPIPHONE
  {
    {"SOLO", "AMBF", "DLYP"}, // Banco "SOLOS"
    {"VOX", "FEND", "DR.Z"}, // Banco "AMP"
    {"WAHA", "CTRL", "DLY"},    // Banco "EFECTOS"
    {"ANT", "NOTA", "SIG"} // Banco "TABLET"
  }
};

// 2. DEFINE LOS COMANDOS MIDI
//    Usaremos una estructura para definir qué hace cada botón.
struct MidiCommand {
  char type; // 'P' para Program Change (PC), 'C' para Control Change (CC)
  int bank;     // El valor del Bank Select (0 o 1)
  int number; // El número de PC o CC
};

// Matriz 3D con los comandos MIDI
MidiCommand midiCommands[NUM_GUITARS][NUM_BANKS][3] = {
  // GUITARRA 0: IBANEZ
  {
    // Banco "SOLOS" - Cambian a patches completos (PC)
    {{'P', 0, 3}, {'P', 0, 1}, {'P', 0, 2}},
    // Banco "AMP" - Cambian a patches completos (PC)
    {{'P', 0, 9}, {'P', 0, 8}, {'P', 0, 10}},
    // Banco "EFECTOS" - Encienden/apagan módulos (CC)
    // CC#57=WAH, CC#70=CTRL2, CC#55=DLY
    {{'C', 0, 57}, {'C', 0, 70}, {'C', 0, 55}},
    // Banco "TABLET"
    {{'T', 0, 103}, {'T', 0, 104}, {'T', 0, 102}}  
  },
  // GUITARRA 1: EPIPHONE
  {
    // Banco "SOLOS" - Cambian a patches completos (PC) 
    {{'P', 0, 20}, {'P', 0, 21}, {'P', 0, 22}},
    // Banco "AMP" - Cambian a patches completos (PC)
    {{'P', 1, 34}, {'P', 1, 11}, {'P', 1, 12}},
    // Banco "EFECTOS" - Encienden/apagan módulos (CC)
    // CC#57=WAH, CC#70=CTRL2, CC#55=DLY
    {{'C', 0, 57}, {'C', 0, 70}, {'C', 0, 55}},
    // Banco "TABLET"
    {{'T', 0, 103}, {'T', 0, 104}, {'T', 0, 102}} 
  }
};
//****************************************************************
//           <<<<< FIN DEL ÁREA DE PERSONALIZACIÓN >>>>>
//****************************************************************

// --- PINES DE HARDWARE ---
const int BTN_BANK_DOWN = 2;
const int BTN_GUITAR = 3;
const int BTN_BANK_UP = 4;
const int BTN_PRESET_1 = 5;
const int BTN_PRESET_2 = 6;
const int BTN_PRESET_3 = 7;
const int LED_PRESET_1 = 8;
const int LED_PRESET_2 = 9;
const int LED_PRESET_3 = 10;

// --- VARIABLES GLOBALES ---
int currentGuitar = 0;
int currentBank = 0;
bool ledStatus[3] = {false, false, false}; 

unsigned long lastDebounceTime = 0;
const long debounceDelay = 400;

// --- NUEVAS VARIABLES PARA EL PARPADEO DEL LCD ---
bool isBlinking = false;
int blinkingPresetIndex = -1; // -1 significa que ninguno parpadea
unsigned long blinkStartTime = 0;
unsigned long lastBlinkTime = 0;
bool blinkState = false;
const int blinkInterval = 333; // Parpadea cada 333ms
const int blinkDuration = 2000; // Duración total del parpadeo (2 segundos)

void setup() {
  MIDI.begin(1);
  lcd.init();
  lcd.backlight();


 // Cargar los símbolos que quieres usar en la memoria del LCD
  lcd.createChar(0, notaMusical); // La nota musical queda en la posición 0
  lcd.createChar(1, notaMusical2); // La nota musical queda en la posición 1

  // --- Secuencia de Bienvenida ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MIDI Controller");
  lcd.setCursor(0, 1);
  lcd.print("Valeton GP-200");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BY");
  lcd.setCursor(0, 1);
  lcd.print("ROBERT CODER");
  delay(2000); 

  // --- Mensaje Final de Bienvenida ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HI ROBERT ");
  delay(700);
  lcd.write((byte)0); // Imprime el símbolo en la posición 0 (nota musical)
  lcd.print(" ");
  delay(700);
  lcd.write((byte)1); // Imprime el símbolo en la posición 1 (nota musical2)
  lcd.print(" ");
  delay(700);
  lcd.write((byte)0); // Imprime el símbolo en la posición 1 (guitarra)
  delay(2500); // Muestra el mensaje por 2.5 segundos

  
  
  pinMode(BTN_BANK_DOWN, INPUT_PULLUP);
  pinMode(BTN_GUITAR, INPUT_PULLUP);
  pinMode(BTN_BANK_UP, INPUT_PULLUP);
  pinMode(BTN_PRESET_1, INPUT_PULLUP);
  pinMode(BTN_PRESET_2, INPUT_PULLUP);
  pinMode(BTN_PRESET_3, INPUT_PULLUP);

  pinMode(LED_PRESET_1, OUTPUT);
  pinMode(LED_PRESET_2, OUTPUT);
  pinMode(LED_PRESET_3, OUTPUT);

  updateDisplay();
  updateLeds();
}

void loop() {
  readButtons();
  handleBlinking(); // función para controlar el parpadeo
}

void handleBlinking() {
  // Solo ejecuta esto si el parpadeo está activado
  if (!isBlinking) {
    return;
  }

  // Comprueba si ya pasaron los 3 segundos
  if (millis() - blinkStartTime > blinkDuration) {
    isBlinking = false;
    blinkingPresetIndex = -1;
    updateDisplay(); // Restaura la pantalla a su estado normal
    return;
  }

  // Comprueba si es momento de cambiar el estado (visible/invisible)
  if (millis() - lastBlinkTime > blinkInterval) {
    blinkState = !blinkState; // Invierte el estado
    lastBlinkTime = millis();

    // Calcula la posición del texto que debe parpadear
    int cursorPosition = blinkingPresetIndex * 6;
    lcd.setCursor(cursorPosition, 1);

    if (blinkState) {
      // Si está visible, escribe el nombre del preset
      lcd.print(presetNames[currentGuitar][currentBank][blinkingPresetIndex]);
    } else {
      // Si está invisible, escribe espacios en blanco
      lcd.print("    "); 
    }
  }
}

void readButtons() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Selector de Guitarra
    if (digitalRead(BTN_GUITAR) == LOW) {
      currentGuitar = (currentGuitar + 1) % NUM_GUITARS;
      currentBank = 0;
      resetLedStatus();
      updateDisplay();
      lastDebounceTime = millis();
    }
    // Banco Arriba
    else if (digitalRead(BTN_BANK_UP) == LOW) {
      currentBank = (currentBank + 1) % NUM_BANKS;
      resetLedStatus();
      updateDisplay();
      lastDebounceTime = millis();
    }
    // Banco Abajo
    else if (digitalRead(BTN_BANK_DOWN) == LOW) {
      currentBank = (currentBank - 1 + NUM_BANKS) % NUM_BANKS;
      resetLedStatus();
      updateDisplay();
      lastDebounceTime = millis();
    }
    // Botones de Preset
    else if (digitalRead(BTN_PRESET_1) == LOW) {
      handlePreset(0);
      lastDebounceTime = millis();
    } else if (digitalRead(BTN_PRESET_2) == LOW) {
      handlePreset(1);
      lastDebounceTime = millis();
    } else if (digitalRead(BTN_PRESET_3) == LOW) {
      handlePreset(2);
      lastDebounceTime = millis();
    }
  }
}

void handlePreset(int presetIndex) {
  // Inicia la secuencia de parpadeo para el preset presionado
  isBlinking = true;
  blinkingPresetIndex = presetIndex;
  blinkStartTime = millis();
  lastBlinkTime = millis();
  blinkState = false; // Prepara el estado para que la próxima acción sea "encender"

  // --- LÍNEAS NUEVAS AÑADIDAS ---
  // Apaga el texto INMEDIATAMENTE para una respuesta visual instantánea
  int cursorPosition = presetIndex * 6;
  lcd.setCursor(cursorPosition, 1);
  lcd.print("    "); 
  // --- FIN DE LAS LÍNEAS NUEVAS ---

  // La lógica MIDI no cambia
  MidiCommand cmd = midiCommands[currentGuitar][currentBank][presetIndex];

  if (cmd.type == 'P') { 
    MIDI.sendControlChange(0, cmd.bank, 1); 
    MIDI.sendProgramChange(cmd.number, 1);
    
    for (int i = 0; i < 3; i++) { ledStatus[i] = false; }
    ledStatus[presetIndex] = true;
    updateLeds();
  } 
  else if (cmd.type == 'C') {
    ledStatus[presetIndex] = !ledStatus[presetIndex]; 
    int value = ledStatus[presetIndex] ? 127 : 0;
    MIDI.sendControlChange(cmd.number, value, 1);
    updateLeds();
  }
  else if (cmd.type == 'T') { 
    MIDI.sendControlChange(cmd.number, 127, 1);
    
    digitalWrite(LED_PRESET_1 + presetIndex, HIGH);
    delay(300); 
    digitalWrite(LED_PRESET_1 + presetIndex, LOW);
  }
}

void updateLeds() {
  digitalWrite(LED_PRESET_1, ledStatus[0] ? HIGH : LOW);
  digitalWrite(LED_PRESET_2, ledStatus[1] ? HIGH : LOW);
  digitalWrite(LED_PRESET_3, ledStatus[2] ? HIGH : LOW);
}

void resetLedStatus() {
  for(int i=0; i<3; i++) {
    ledStatus[i] = false;
  }
  updateLeds();
}

void updateDisplay() {
  lcd.clear();
  // Fila superior: "GUITARRA: BANCO"
  lcd.setCursor(0, 0);
  lcd.print(guitarNames[currentGuitar]);
  lcd.print(": ");
  lcd.print(bankNames[currentBank]);

  // Fila inferior: "PRE1  PRE2  PRE3"
  lcd.setCursor(0, 1);
  lcd.print(presetNames[currentGuitar][currentBank][0]);
  lcd.setCursor(6, 1);
  lcd.print(presetNames[currentGuitar][currentBank][1]);
  lcd.setCursor(12, 1);
  lcd.print(presetNames[currentGuitar][currentBank][2]);
}
