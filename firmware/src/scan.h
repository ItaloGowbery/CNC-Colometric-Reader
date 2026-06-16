#pragma once
#include <Arduino.h>
#include "sensor.h"
#include "config.h"

#define MAX_WELLS   250
#define MAX_POINTS  256

struct WellPos    { uint8_t row; uint8_t col; };
struct PointOffset{ float dx; float dy; };  // offset do canto sup-esq do poço (mm)

struct ScanResult {
    WellPos  pos;
    uint8_t  pointIdx;
    uint16_t ch415,ch445,ch480,ch515,ch555,ch590,ch630,ch680,clear,nir;
    bool     ok;
};

enum ScanState { SCAN_IDLE, SCAN_MOVING, SCAN_READING, SCAN_DONE };

extern class FastAccelStepper *stepperX;
extern class FastAccelStepper *stepperY;

static ScanState   scanState       = SCAN_IDLE;
static WellPos     scanQueue[MAX_WELLS];
static ScanResult  scanResults[256];   // buffer para /api/results (export)
static PointOffset scanPoints[MAX_POINTS];
static int         scanTotal       = 0;
static int         scanResultCount = 0;
static int         scanWellIdx     = 0;
static int         scanPointIdx    = 0;
static int         scanNumPts      = 1;
static void (*scanResultCallback)(const ScanResult*) = nullptr;
static void (*scanDoneCallback)()                    = nullptr;
static float       scanSpacingX    = 9.0f;
static float       scanSpacingY    = 9.0f;
static bool        scanAverageMode = false;
static uint32_t    _acc[10];

static void _moveToPoint(int wi, int pi) {
    float wx = constrain(X_ORIGIN_MM + (scanQueue[wi].col - 1) * scanSpacingX + scanPoints[pi].dx, 0.0f, X_MAX_MM);
    float wy = constrain(Y_ORIGIN_MM +  scanQueue[wi].row      * scanSpacingY + scanPoints[pi].dy, 0.0f, Y_MAX_MM);
    stepperX->moveTo((int32_t)(wx * STEPS_PER_MM));
    stepperY->moveTo((int32_t)(wy * STEPS_PER_MM));
}

inline void scanStart(WellPos *wells, int wCount,
                      PointOffset *points, int pCount,
                      float spX, float spY, bool avgMode = false) {
    for (int i = 0; i < wCount && i < MAX_WELLS;  i++) scanQueue[i]  = wells[i];
    for (int i = 0; i < pCount && i < MAX_POINTS; i++) scanPoints[i] = points[i];
    scanTotal       = min(wCount, MAX_WELLS);
    scanNumPts      = min(pCount, MAX_POINTS);
    scanWellIdx     = 0;
    scanPointIdx    = 0;
    scanResultCount = 0;
    scanSpacingX    = spX;
    scanSpacingY    = spY;
    scanAverageMode = avgMode;
    memset(_acc, 0, sizeof(_acc));
    digitalWrite(ENABLE_PIN, LOW);
    _moveToPoint(0, 0);
    scanState = SCAN_MOVING;
    Serial.printf("[SCAN] %d poços x %d pontos (%s)\n", scanTotal, scanNumPts, avgMode ? "média" : "individual");
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
            if (!scanAverageMode) {
                ScanResult res = {
                    scanQueue[scanWellIdx], (uint8_t)scanPointIdx,
                    r.ch415, r.ch445, r.ch480, r.ch515,
                    r.ch555, r.ch590, r.ch630, r.ch680,
                    r.clear, r.nir, r.ok
                };
                if (scanResultCount < 256) scanResults[scanResultCount++] = res;
                if (scanResultCallback) scanResultCallback(&res);
            } else {
                _acc[0]+=r.ch415; _acc[1]+=r.ch445; _acc[2]+=r.ch480; _acc[3]+=r.ch515;
                _acc[4]+=r.ch555; _acc[5]+=r.ch590; _acc[6]+=r.ch630; _acc[7]+=r.ch680;
                _acc[8]+=r.clear; _acc[9]+=r.nir;
            }
            scanPointIdx++;

            if (scanPointIdx >= scanNumPts) {
                if (scanAverageMode) {
                    int n = scanNumPts;
                    ScanResult avg = {
                        scanQueue[scanWellIdx], 0,
                        (uint16_t)(_acc[0]/n), (uint16_t)(_acc[1]/n),
                        (uint16_t)(_acc[2]/n), (uint16_t)(_acc[3]/n),
                        (uint16_t)(_acc[4]/n), (uint16_t)(_acc[5]/n),
                        (uint16_t)(_acc[6]/n), (uint16_t)(_acc[7]/n),
                        (uint16_t)(_acc[8]/n), (uint16_t)(_acc[9]/n),
                        r.ok
                    };
                    if (scanResultCount < 256) scanResults[scanResultCount++] = avg;
                    if (scanResultCallback) scanResultCallback(&avg);
                    memset(_acc, 0, sizeof(_acc));
                }
                Serial.printf("[SCAN] Poço %d/%d OK\n", scanWellIdx+1, scanTotal);
                scanWellIdx++;
                scanPointIdx = 0;
                if (scanWellIdx >= scanTotal) {
                    scanState = SCAN_DONE;
                    Serial.println("[SCAN] Concluído");
                    if (scanDoneCallback) scanDoneCallback();
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
