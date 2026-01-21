# 🎸 GP-200 Smart MIDI Controller
**Professional Advanced Foot Controller System by Robert Coder**

Un controlador MIDI de alto rendimiento diseñado específicamente para liberar todo el potencial de la pedalera **Valeton GP-200**. Este sistema combina la robustez de un hardware dedicado basado en Arduino con la flexibilidad de una interfaz de configuración web moderna e intuitiva.

---

## 🚀 Características Principales

### 🧠 Firmware Inteligente
*   **Arquitectura Híbrida de Conectividad**: Soporte simultáneo para USB (MIDI Standard @ 31250 baudios) y Bluetooth (High Speed @ 38400 baudios).
*   **Gestión Dinámica de Bancos**: Sistema de almacenamiento en EEPROM que permite crear y eliminar bancos de memoria en tiempo real (hasta 10 bancos), optimizando la navegación según el setlist.
*   **Personalización Total**: Cada footswitch es configurable individualmente para enviar cambios de programa (Presets) o mensajes de cambio de control (Efectos/Toggles).
*   **Autoconfiguración HC-06**: El firmware detecta y configura automáticamente el módulo Bluetooth con el nombre `MidiController` y baud rate optimizado.

### 💻 WebApp de Configuración (Next-Gen UI)
*   **Diseño Premium**: Interfaz visual basada en **Glassmorphism** y acentos neón, optimizada para tablets y pantallas táctiles.
*   **Tecnología WebSerial**: Conexión directa desde el navegador (Chrome/Edge) sin necesidad de instalar drivers o software adicional.
*   **UX Avanzada**:
    *   Modal inteligente de selección de conexión (USB vs Bluetooth).
    *   Sistema de reintento automático (`Auto-Retry`) para lecturas de datos.
    *   Visualización en tiempo real de los parámetros (Nombres, Tipos, Valores).

---

## 🛠️ Especificaciones Técnicas

### Estructura del Proyecto
```bash
midiControllerRobertCoder/
├── firmware/
│   └── controladorMidi/
│       ├── controladorMidi.ino  # Core Logic & Loop
│       ├── ConfigManager.h      # EEPROM & Bank Management
│       ├── SerialCommander.h    # Protocolo de Comunicación (TX/RX)
│       ├── Button.h             # Debounce & Event Handling
│       ├── DisplayManager.h     # I2C LCD Control
│       ├── LedManager.h         # Visual Feedback
│       └── MidiDictionary.h     # Mapeo de Efectos Valeton
└── webapp/
    ├── index.html               # Semantic HTML5 Structure
    ├── style.css                # CSS3 Variables & Responsive Grid
    └── app.js                   # Serial API logic & UI Controller
```

### Protocolo de Comunicación
El sistema utiliza un protocolo de texto ASII optimizado para comandos seriales:
- **Lectura**: `GETALL` (Recupera toda la configuración activa).
- **Escritura**: `SAVE:B:P:NAME:TYPE:V1:V2` (Guarda un slot específico).
- **Gestión**: `ADDBANK`, `DELBANK` (Modificación estructural de la memoria).

---

## 🔌 Guía de Instalación y Uso

### 1. Firmware
1.  Abrir `firmware/controladorMidi/controladorMidi.ino` en Arduino IDE.
2.  Instalar librerías requeridas: `LiquidCrystal_I2C`, `SoftwareSerial`.
3.  Seleccionar placa (ej. Arduino Nano/Uno) y subir el código.
    *   *Nota: La primera ejecución formateará la EEPROM automáticamente.*

### 2. Configuración Bluetooth
El módulo HC-06 se autoconfigurará al encenderse conectado a los pines definidos.
*   **Nombre**: `MidiController`
*   **PIN**: `1234`

### 3. Editor Web
1.  Abrir la carpeta `webapp` en un navegador compatible con WebSerial (Chrome, Edge, Opera).
    *   *Recomendado usar extensiones de Live Server para evitar bloqueos CORS locales.*
2.  Clic en **"🔌 Conectar USB/Serial"**.
3.  Seleccionar el modo deseado:
    *   **Cable USB**: Para uso cableado estándar.
    *   **Bluetooth**: Para configuración inalámbrica desde PC/Tablet soportadas.

---

### 🎮 Manual de Operación

| Control | Acción Corta (Click) | Acción Larga (Hold > 800ms) |
| :--- | :--- | :--- |
| **Bank Up / Down** | Cambia 1 Banco | **Scroll Rápido** (Sube/Baja bancos continuamente) |
| **Toggle** | Preset Anterior (Swap) | **Afinador** (Envía CC #68 Value 127) |
| **Presets 1-3** | Acción Principal (PC/Efecto) | **Acción Secundaria** (Configurable en App: PC/CC/Fx) |


---

## 📄 Licencia

Desarrollado por **Robert Coder**.
Software de código abierto para propósitos educativos y de desarrollo comunitario.
