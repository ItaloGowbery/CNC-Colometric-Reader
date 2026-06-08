#include <Arduino.h>
#include <FastAccelStepper.h>
#include "config.h"
#include "sensor.h"
#include "scan.h"
#include "web.h"
#include "display.h"

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepperX = nullptr;
FastAccelStepper *stepperY = nullptr;

static int32_t mmToSteps(float mm) {
    return (int32_t)(mm * STEPS_PER_MM);
}

static void enableMotors(bool on) {
    digitalWrite(ENABLE_PIN, on ? LOW : HIGH);
}

static void waitMotors() {
    while (stepperX->isRunning() || stepperY->isRunning()) {
        delay(10);
    }
}

// ---------------------------------------------------------------------------
// Comandos via Serial:
//   x <mm>   — move eixo X relativo (+ ou -)
//   y <mm>   — move eixo Y relativo (+ ou -)
//   e        — habilita motores
//   d        — desabilita motores
//   p        — imprime posição atual em mm
// ---------------------------------------------------------------------------
static void handleSerial() {
    static String buffer = "";

    bool lineReady = false;
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            buffer.trim();
            if (!buffer.isEmpty()) { lineReady = true; break; }
        } else if (c == '\b' || c == 0x7f) {  // backspace / DEL
            if (buffer.length() > 0) buffer.remove(buffer.length() - 1);
        } else {
            buffer += c;
        }
    }

    if (!lineReady) return;
    String line = buffer;
    buffer = "";
    while (Serial.available()) Serial.read();  // descarta sobras (\n, lixo)

    char cmd = line.charAt(0);
    int sp = line.indexOf(' ');
    float val = (sp >= 0) ? line.substring(sp + 1).toFloat() : 0;

    switch (cmd) {
        case 'x': {
            enableMotors(true);
            float curX = stepperX->getCurrentPosition() / (float)STEPS_PER_MM;
            float tgtX = constrain(curX + val, 0.0f, X_MAX_MM);
            stepperX->moveTo(mmToSteps(tgtX));
            Serial.printf("X -> %.2f mm\n", tgtX);
            break;
        }
        case 'y': {
            enableMotors(true);
            float curY = stepperY->getCurrentPosition() / (float)STEPS_PER_MM;
            float tgtY = constrain(curY + val, 0.0f, Y_MAX_MM);
            stepperY->moveTo(mmToSteps(tgtY));
            Serial.printf("Y -> %.2f mm\n", tgtY);
            break;
        }
        case 'r': {
            SensorReading s = sensorRead();
            if (s.ok)
                Serial.printf("LEITURA 415:%u 445:%u 480:%u 515:%u 555:%u 590:%u 630:%u 680:%u\n",
                    s.ch415, s.ch445, s.ch480, s.ch515, s.ch555, s.ch590, s.ch630, s.ch680);
            else
                Serial.println("SENSOR ERRO");
            break;
        }
        case 'h':
            stepperX->setCurrentPosition(0);
            stepperY->setCurrentPosition(0);
            Serial.println("Home definido: X=0 Y=0");
            break;
        case 'e':
            enableMotors(true);
            Serial.println("Motores habilitados");
            break;
        case 'd':
            waitMotors();
            enableMotors(false);
            Serial.println("Motores desabilitados");
            break;
        case 'p': {
            float xMm = stepperX->getCurrentPosition() / (float)STEPS_PER_MM;
            float yMm = stepperY->getCurrentPosition() / (float)STEPS_PER_MM;
            Serial.printf("Posicao: X=%.2f mm  Y=%.2f mm\n", xMm, yMm);
            break;
        }
        default:
            Serial.println("Comandos: x <mm>, y <mm>, r, h, e, d, p");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    Serial.println("=== CNC Colorimetric Reader ===");
    Serial.println("Comandos: x <mm>, y <mm>, e, d, p");

    pinMode(ENABLE_PIN, OUTPUT);
    enableMotors(false);

    displayBegin();
    sensorBegin();
    webBegin();
    engine.init();

    stepperX = engine.stepperConnectToPin(X_STEP_PIN);
    stepperY = engine.stepperConnectToPin(Y_STEP_PIN);

    if (!stepperX || !stepperY) {
        Serial.println("ERRO: falha ao inicializar steppers");
        while (true) delay(1000);
    }

    stepperX->setDirectionPin(X_DIR_PIN);
    stepperY->setDirectionPin(Y_DIR_PIN);

    stepperX->setSpeedInHz(mmToSteps(MAX_SPEED_MM_S));
    stepperX->setAcceleration(mmToSteps(ACCEL_MM_S2));

    stepperY->setSpeedInHz(mmToSteps(MAX_SPEED_MM_S));
    stepperY->setAcceleration(mmToSteps(ACCEL_MM_S2));

    Serial.println("Pronto.");
}

void loop() {
    handleSerial();
    scanLoop();

    static bool wasRunning = false;
    bool running = stepperX->isRunning() || stepperY->isRunning();
    if (wasRunning && !running && scanState == SCAN_IDLE) Serial.println("DONE");
    wasRunning = running;

    static uint32_t lastDisplay = 0;
    if (millis() - lastDisplay >= 300) {
        float x = stepperX->getCurrentPosition() / (float)STEPS_PER_MM;
        float y = stepperY->getCurrentPosition() / (float)STEPS_PER_MM;
        displayUpdate(x, y, running);
        lastDisplay = millis();
    }
}
