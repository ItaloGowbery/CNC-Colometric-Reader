#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include "config.h"
#include "scan.h"

#define TFT_CS        42
#define TFT_DC        40
#define TFT_RST       41
#define TFT_BACKLITE  45

static Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

// Layout (240x135, rotation 1):
//  y=  0  h=22  IP: 192.168.x.xxx   size 2
//  y= 24  h=22  X:123.4  Y:210.0mm  size 2
//  y= 48  h=30  0 / 144             size 3
//  y= 80  h=30  idle                size 3

static char _dIP[20]  = "";
static char _dPos[32] = "";
static int  _dScanIdx = -1, _dScanTot = -1, _dState = -1;

static void _tftRow(int y, int h, uint16_t color, uint8_t sz, const char* text) {
    tft.fillRect(0, y, 240, h, ST77XX_BLACK);
    tft.setTextColor(color);
    tft.setTextSize(sz);
    tft.setCursor(4, y + 2);
    tft.print(text);
}

inline void displayBegin() {
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
    tft.init(135, 240);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(false);
}

inline void displayUpdate(float x, float y) {
    static bool firstRun = true;
    if (firstRun) { tft.fillScreen(ST77XX_BLACK); firstRun = false; }
    char buf[40];

    // IP (size 2, y=0)
    char ip[20];
    snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
    if (strcmp(ip, _dIP) != 0) {
        snprintf(buf, sizeof(buf), "IP: %s", ip);
        _tftRow(0, 22, ST77XX_WHITE, 2, buf);
        strcpy(_dIP, ip);
    }

    // X/Y na mesma linha (size 2, y=24)
    char pos[32];
    snprintf(pos, sizeof(pos), "X:%.1f  Y:%.1fmm", x, y);
    if (strcmp(pos, _dPos) != 0) {
        _tftRow(24, 22, 0x07FF, 2, pos);
        strcpy(_dPos, pos);
    }

    // Scan progress (size 3, y=48)
    if (_dScanIdx != scanWellIdx || _dScanTot != scanTotal) {
        snprintf(buf, sizeof(buf), "%d / %d   ", scanWellIdx, scanTotal);
        _tftRow(48, 30, 0x07E0, 3, buf);
        _dScanIdx = scanWellIdx;
        _dScanTot = scanTotal;
    }

    // Estado (size 3, y=80)
    if (_dState != (int)scanState) {
        snprintf(buf, sizeof(buf), "%s   ", scanStateName());
        _tftRow(80, 30, ST77XX_YELLOW, 3, buf);
        _dState = (int)scanState;
    }
}
