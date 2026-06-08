#pragma once
#include <Arduino.h>
#include "sensor.h"
#include "config.h"

#define MAX_WELLS  384
#define MAX_POINTS  25

struct WellPos    { uint8_t row; uint8_t col; };
struct PointOffset{ float dx; float dy; };  // offset do canto sup-esq do poço (mm)

struct ScanResult {
    WellPos  pos;
    uint8_t  pointIdx;
    uint16_t ch415,ch445,ch480,ch515,ch555,ch590,ch630,ch680;
    bool     ok;
};

enum ScanState { SCAN_IDLE, SCAN_MOVING, SCAN_READING, SCAN_DONE };

extern class FastAccelStepper *stepperX;
extern class FastAccelStepper *stepperY;

static ScanState   scanState       = SCAN_IDLE;
static WellPos     scanQueue[MAX_WELLS];
static ScanResult  scanResults[MAX_WELLS * MAX_POINTS];
static PointOffset scanPoints[MAX_POINTS];
static int         scanTotal       = 0;
static int         scanResultCount = 0;
static int         scanWellIdx     = 0;
static int         scanPointIdx    = 0;
static int         scanNumPts      = 1;
static float       scanSpacingX    = 9.0f;
static float       scanSpacingY    = 9.0f;

static void _moveToPoint(int wi, int pi) {
    float wx = constrain(X_ORIGIN_MM + (scanQueue[wi].col - 1) * scanSpacingX + scanPoints[pi].dx, 0.0f, X_MAX_MM);
    float wy = constrain(Y_ORIGIN_MM +  scanQueue[wi].row      * scanSpacingY + scanPoints[pi].dy, 0.0f, Y_MAX_MM);
    stepperX->moveTo((int32_t)(wx * STEPS_PER_MM));
    stepperY->moveTo((int32_t)(wy * STEPS_PER_MM));
}

inline void scanStart(WellPos *wells, int wCount,
                      PointOffset *points, int pCount,
                      float spX, float spY) {
    for (int i = 0; i < wCount && i < MAX_WELLS;  i++) scanQueue[i]  = wells[i];
    for (int i = 0; i < pCount && i < MAX_POINTS; i++) scanPoints[i] = points[i];
    scanTotal    = min(wCount, MAX_WELLS);
    scanNumPts   = min(pCount, MAX_POINTS);
    scanWellIdx     = 0;
    scanPointIdx    = 0;
    scanResultCount = 0;
    scanSpacingX    = spX;
    scanSpacingY    = spY;
    digitalWrite(ENABLE_PIN, LOW);
    _moveToPoint(0, 0);
    scanState = SCAN_MOVING;
    Serial.printf("[SCAN] %d poços x %d pontos\n", scanTotal, scanNumPts);
}

inline void scanLoop() {
    switch (scanState) {
        case SCAN_IDLE:
        case SCAN_DONE:
            break;

        case SCAN_MOVING:
            if (!stepperX->isRunning() && !stepperY->isRunning())
                scanState = SCAN_READING;
            break;

        case SCAN_READING: {
            SensorReading r = sensorRead();
            scanResults[scanResultCount++] = {
                scanQueue[scanWellIdx], (uint8_t)scanPointIdx,
                r.ch415, r.ch445, r.ch480, r.ch515,
                r.ch555, r.ch590, r.ch630, r.ch680,
                r.ok
            };
            scanPointIdx++;

            if (scanPointIdx >= scanNumPts) {
                Serial.printf("[SCAN] Poço %d/%d OK\n", scanWellIdx+1, scanTotal);
                scanWellIdx++;
                scanPointIdx = 0;
                if (scanWellIdx >= scanTotal) {
                    scanState = SCAN_DONE;
                    Serial.println("[SCAN] Concluído");
                } else {
                    _moveToPoint(scanWellIdx, 0);
                    scanState = SCAN_MOVING;
                }
            } else {
                _moveToPoint(scanWellIdx, scanPointIdx);
                scanState = SCAN_MOVING;
            }
            break;
        }
    }
}

inline const char* scanStateName() {
    switch (scanState) {
        case SCAN_IDLE:    return "waiting";
        case SCAN_MOVING:  return "moving";
        case SCAN_READING: return "reading";
        case SCAN_DONE:    return "done";
    }
    return "idle";
}
