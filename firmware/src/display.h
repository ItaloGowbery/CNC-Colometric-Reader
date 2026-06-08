#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include "config.h"
#include "scan.h"

#define TFT_CS        7
#define TFT_DC       39
#define TFT_RST      40
#define TFT_BACKLITE 45

static Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

// estado anterior para redesenhar só o que mudou
static char _dIP[20]  = "";
static char _dPos[32] = "";
static bool _dRunning = false, _dRunInit = false;
static int  _dScanIdx = -1, _dScanTot = -1, _dState = -1;

static void _tftRow(int y, int h, uint16_t color, uint8_t sz, const char* text) {
    tft.fillRect(0, y, 240, h, ST77XX_BLACK);
    tft.setTextColor(color);
    tft.setTextSize(sz);
    tft.setCursor(4, y + 1);
    tft.print(text);
}

inline void displayBegin() {
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
    tft.init(135, 240);
    tft.setRotation(3);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(false);
}

// Chamar periodicamente no loop (ex: a cada 300ms)
inline void displayUpdate(float x, float y, bool running) {
    char buf[40];

    // Linha 1 — IP (size 1, y=2)
    char ip[20];
    snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
    if (strcmp(ip, _dIP) != 0) {
        snprintf(buf, sizeof(buf), "IP: %s", ip);
        _tftRow(2, 10, ST77XX_WHITE, 1, buf);
        strcpy(_dIP, ip);
    }

    // Linha 2 — Posição X/Y (size 2, y=14)
    char pos[32];
    snprintf(pos, sizeof(pos), "X:%-6.1f Y:%-5.1fmm", x, y);
    if (strcmp(pos, _dPos) != 0) {
        _tftRow(14, 18, 0x07FF, 2, pos);
        strcpy(_dPos, pos);
    }

    // Linha 3 — Motor (size 1, y=34)
    if (!_dRunInit || _dRunning != running) {
        snprintf(buf, sizeof(buf), "Motor: %s", running ? "Movendo" : "Parado ");
        _tftRow(34, 10, 0xFFE0, 1, buf);
        _dRunning = running;
        _dRunInit = true;
    }

    // Linha 4 — Progresso do scan (size 2, y=46)
    if (_dScanIdx != scanWellIdx || _dScanTot != scanTotal) {
        snprintf(buf, sizeof(buf), "Scan: %d / %d   ", scanWellIdx, scanTotal);
        _tftRow(46, 18, 0x07E0, 2, buf);
        _dScanIdx = scanWellIdx;
        _dScanTot = scanTotal;
    }

    // Linha 5 — Estado do scan (size 1, y=66)
    if (_dState != (int)scanState) {
        snprintf(buf, sizeof(buf), "Estado: %s   ", scanStateName());
        _tftRow(66, 10, ST77XX_WHITE, 1, buf);
        _dState = (int)scanState;
    }
}
