// Diccionario de Efectos (Debe coincidir con MidiDictionary.h)
const EFFECT_DICT = [
    { name: "DIST (Distortion)", id: 0 },
    { name: "AMP (Amplifier)", id: 1 },
    { name: "MOD (Modulation)", id: 2 },
    { name: "DLY (Delay)", id: 3 },
    { name: "REV (Reverb)", id: 4 },
    { name: "WAH (Wah)", id: 5 },
    { name: "TUNER (Tuner)", id: 6 },
    { name: "LOOP (Looper On/Off)", id: 7 },
    { name: "L.REC (Looper Rec)", id: 8 },
    { name: "L.PLY (Looper Play)", id: 9 },
    { name: "CTRL1", id: 10 },
    { name: "CTRL2", id: 11 },
    { name: "CTRL3", id: 12 },
    { name: "TAP (Tap Tempo)", id: 13 }
];

let port;
let writer;
let isConnected = false;

// Estado local
const NUM_BANKS = 8;
const NUM_PRESETS = 3;

let configs = []; // Matriz [Bank][Preset]
let bankNames = []; // Array de nombres de bancos

// Inicializar matriz vacía
for (let b = 0; b < NUM_BANKS; b++) {
    bankNames[b] = `BANK ${b}`;
    configs[b] = [];
    for (let p = 0; p < NUM_PRESETS; p++) {
        configs[b][p] = { name: "INIT", type: "P", val1: 0, val2: 0 };
    }
}

let currentBank = 0;

document.addEventListener('DOMContentLoaded', () => {
    initUI();
    document.getElementById('btnConnect').addEventListener('click', connectSerial);
    document.getElementById('btnLoad').addEventListener('click', () => sendCommand("GETALL"));

    // Controles de Navegación
    document.getElementById('btnPrevBank').addEventListener('click', () => {
        currentBank--;
        if (currentBank < 0) currentBank = NUM_BANKS - 1;
        renderPedalboard();
    });
    document.getElementById('btnNextBank').addEventListener('click', () => {
        currentBank++;
        if (currentBank >= NUM_BANKS) currentBank = 0;
        renderPedalboard();
    });

    // Guardar Nombre de Banco
    document.getElementById('btnSaveBank').addEventListener('click', () => {
        const name = document.getElementById('txtBankName').value.toUpperCase().substring(0, 8);
        saveBankName(currentBank, name);
    });
});

function initUI() {
    // Poblar selects de efectos
    const selects = document.querySelectorAll('.fs-val1-dict');
    selects.forEach(sel => {
        EFFECT_DICT.forEach(fx => {
            const opt = document.createElement('option');
            opt.value = fx.id;
            opt.textContent = fx.name;
            sel.appendChild(opt);
        });
    });

    // Listeners para Toggle Tipo
    document.querySelectorAll('.fs-type').forEach((sel, idx) => {
        sel.addEventListener('change', (e) => {
            const container = sel.closest('.footswitch');
            const type = e.target.value;
            if (type === 'P') {
                container.querySelector('.fs-options-preset').style.display = 'grid'; // Grid por el cambio css
                container.querySelector('.fs-options-dict').style.display = 'none';
            } else {
                container.querySelector('.fs-options-preset').style.display = 'none';
                container.querySelector('.fs-options-dict').style.display = 'block';
            }
        });
    });

    // Listeners para Guardar Slot
    document.querySelectorAll('.btn-save-slot').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const slotParams = getSlotData(e.target.closest('.footswitch'));
            saveSlot(slotParams);
        });
    });

    // Helper Visual Valeton (01-A)
    document.querySelectorAll('.fs-val1-pc').forEach(input => {
        // Crear elemento de texto debajo
        const helper = document.createElement('div');
        helper.className = 'valeton-helper';
        helper.textContent = getValetonLabel(input.value || 0);
        input.parentNode.appendChild(helper);

        // Update on change
        input.addEventListener('input', (e) => {
            helper.textContent = getValetonLabel(parseInt(e.target.value) || 0);
        });
    });
}

// --- UI UTILS & TOASTS ---
function createToastContainer() {
    if (!document.querySelector('.toast-container')) {
        const container = document.createElement('div');
        container.className = 'toast-container';
        document.body.appendChild(container);
    }
}

function showToast(message, type = 'success') {
    createToastContainer();
    const container = document.querySelector('.toast-container');

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;

    const icon = type === 'success' ? '✅' : '⚠️';

    toast.innerHTML = `
        <span class="toast-icon">${icon}</span>
        <span class="toast-message">${message}</span>
    `;

    container.appendChild(toast);

    // Trigger animation
    requestAnimationFrame(() => {
        toast.classList.add('show');
    });

    // Remove after 3s
    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// --- SERIAL LOGIC ---

// Variable global para el writer
let textEncoder;
let writableStreamClosed;

async function connectSerial() {
    if (!navigator.serial) {
        showToast("WebSerial no soportado. Usa Chrome/Edge.", "error");
        return;
    }

    try {
        port = await navigator.serial.requestPort();
        // IMPORTANTE: 31250 es la velocidad MIDI standard.
        // Si el Arduino usa MIDI.begin(), usa esta velocidad.
        await port.open({ baudRate: 31250 });

        // Configurar Writer una sola vez
        textEncoder = new TextEncoderStream();
        writableStreamClosed = textEncoder.readable.pipeTo(port.writable);
        writer = textEncoder.writable.getWriter();

        isConnected = true;
        document.getElementById('statusText').textContent = "🟢 Conectado";
        document.getElementById('mainPanel').classList.remove('disabled');
        document.getElementById('btnConnect').style.display = 'none';

        showToast("Conexión Establecida");

        // Escuchar
        readLoop();

        // Handshake & Sync
        setTimeout(() => {
            sendCommand("HELLO");
            sendCommand("GETALL");
        }, 500);

    } catch (err) {
        console.error(err);
        showToast("Error al conectar: " + err, "error");
    }
}

async function readLoop() {
    const textDecoder = new TextDecoderStream();
    const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
    const reader = textDecoder.readable.getReader();

    try {
        while (true) {
            const { value, done } = await reader.read();
            if (done) break;
            if (value) parseSerialData(value);
        }
    } catch (error) {
        console.error(error);
        showToast("Error de lectura Serial", "error");
    }
}

// Helper: Convierte PC (0-127) a Formato Banco-Patch (01-A ... 32-D)
function getValetonLabel(pc) {
    if (pc < 0 || pc > 127) return "Inválido";
    const bank = Math.floor(pc / 4) + 1;
    const slotIndex = pc % 4;
    const slots = ['A', 'B', 'C', 'D'];
    const bankStr = bank.toString().padStart(2, '0');
    return `Valeton: ${bankStr}-${slots[slotIndex]}`;
}

let buffer = "";
function parseSerialData(data) {
    buffer += data;
    const lines = buffer.split(/\r?\n/);
    buffer = lines.pop();

    lines.forEach(line => {
        // console.log("RX:", line);
        if (line.trim() === "") return;

        if (line.startsWith("BANK:")) {
            // BANK:ID:NAME
            const parts = line.split(":");
            if (parts.length >= 3) {
                const id = parseInt(parts[1]);
                const name = parts[2] || "";
                if (id >= 0 && id < NUM_BANKS) {
                    bankNames[id] = name;
                }
            }

        } else if (line.startsWith("DATA:")) {
            // DATA:B:P:NAME:TYPE:V1:V2 (NO GUITAR)
            const parts = line.split(":");
            if (parts.length >= 7) {
                const b = parseInt(parts[1]);
                const p = parseInt(parts[2]);

                if (!configs[b]) configs[b] = [];

                configs[b][p] = {
                    name: parts[3],
                    type: parts[4],
                    val1: parseInt(parts[5]),
                    val2: parseInt(parts[6])
                };
            }
        } else if (line.startsWith("END:CONFIG")) {
            renderPedalboard();
            showToast("Configuración Sincronizada");
        } else if (line.startsWith("OK:SAVED")) {
            showToast("Botón Guardado");
        } else if (line.startsWith("OK:BANK_RENAMED")) {
            showToast("Banco Renombrado");
            bankNames[currentBank] = document.getElementById('txtBankName').value.toUpperCase();
        } else if (line.startsWith("READY:")) {
            console.log("Pedal Ready");
            showToast("Pedal Listo ✅");
        }
    });
}

async function sendCommand(cmd) {
    if (!writer) {
        showToast("No conectado", "error");
        return;
    }
    try {
        await writer.write(cmd + "\n");
    } catch (err) {
        console.error("Error writing:", err);
        showToast("Error de escritura", "error");
    }
}

function renderPedalboard() {
    document.getElementById('lblBankIndex').textContent = `BANK ${currentBank}`;
    const nameInput = document.getElementById('txtBankName');
    if (nameInput) nameInput.value = bankNames[currentBank] || "";

    // Update Slots
    for (let i = 0; i < 3; i++) {
        const data = configs[currentBank][i];
        const el = document.querySelector(`.footswitch[data-index="${i}"]`);

        if (!data) continue;

        el.querySelector('.fs-name').value = data.name;
        el.querySelector('.fs-type').value = data.type;

        // Disparar evento change
        el.querySelector('.fs-type').dispatchEvent(new Event('change'));

        if (data.type === 'P') {
            const inputPC = el.querySelector('.fs-val1-pc');
            inputPC.value = data.val1;
            el.querySelector('.fs-val2-pc').value = data.val2;

            // Update Helper manually
            const helper = inputPC.parentNode.querySelector('.valeton-helper');
            if (helper) helper.textContent = getValetonLabel(data.val1);

        } else {
            el.querySelector('.fs-val1-dict').value = data.val1;
        }
    }
}

function getSlotData(el) {
    const idx = parseInt(el.dataset.index);
    const type = el.querySelector('.fs-type').value;
    const name = el.querySelector('.fs-name').value.padEnd(4, ' ').substring(0, 4).toUpperCase();

    let v1 = 0;
    let v2 = 0;

    if (type === 'P') {
        v1 = el.querySelector('.fs-val1-pc').value;
        v2 = el.querySelector('.fs-val2-pc').value;
    } else {
        v1 = el.querySelector('.fs-val1-dict').value;
    }

    return {
        b: currentBank,
        p: idx,
        name: name,
        type: type,
        v1: v1,
        v2: v2
    };
}

function saveSlot(data) {
    // SAVE:B:P:NAME:TYPE:V1:V2
    const cmd = `SAVE:${data.b}:${data.p}:${data.name}:${data.type}:${data.v1}:${data.v2}`;
    console.log("TX:", cmd);
    sendCommand(cmd);

    // Update local config cache
    configs[data.b][data.p] = {
        name: data.name,
        type: data.type,
        val1: parseInt(data.v1),
        val2: parseInt(data.v2)
    };
}

function saveBankName(bankIdx, name) {
    // SAVEBANK:B:NAME
    const cmd = `SAVEBANK:${bankIdx}:${name}`;
    console.log("TX:", cmd);
    sendCommand(cmd);
}
