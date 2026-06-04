#include <Arduino.h>
#include <FastAccelStepper.h>
#include "config.h"

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
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) return;

    char cmd = line.charAt(0);
    float val = 0;
    if (line.length() > 1) val = line.substring(2).toFloat();

    switch (cmd) {
        case 'x':
            enableMotors(true);
            stepperX->move(mmToSteps(val));
            Serial.printf("X movendo %.2f mm\n", val);
            break;
        case 'y':
            enableMotors(true);
            stepperY->move(mmToSteps(val));
            Serial.printf("Y movendo %.2f mm\n", val);
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
            Serial.println("Comandos: x <mm>, y <mm>, e, d, p");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("=== CNC Colorimetric Reader - Fase 1 ===");
    Serial.println("Comandos: x <mm>, y <mm>, e, d, p");

    pinMode(ENABLE_PIN, OUTPUT);
    enableMotors(false);

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
}
