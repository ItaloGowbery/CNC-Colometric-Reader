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
//  y=  0  h=36  CNC Colorimetric / Reader  size 2 (2 linhas)
//  y= 38  h=22  IP: 192.168.x.xxx          size 2
//  y= 62  h=22  X:123.4  Y:210.0mm         size 2
//  y= 86  h=22  0 / 144                    size 2
//  y=110  h=22  waiting                    size 2

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
    if (firstRun) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(0xFD20);  // laranja
        tft.setTextSize(2);
        tft.setCursor(4, 2);
        tft.print("CNC Colorimetric");
        tft.setCursor(4, 20);
        tft.print("Reader");
        firstRun = false;
    }
    char buf[40];

    // IP (size 2, y=38)
    char ip[20];
    snprintf(ip, sizeof(ip), "%s", WiFi.localIP().toString().c_str());
    if (strcmp(ip, _dIP) != 0) {
        snprintf(buf, sizeof(buf), "IP: %s", ip);
        _tftRow(38, 22, ST77XX_WHITE, 2, buf);
        strcpy(_dIP, ip);
    }

    // X/Y na mesma linha (size 2, y=62)
    char pos[32];
    snprintf(pos, sizeof(pos), "X:%.1f  Y:%.1fmm", x, y);
    if (strcmp(pos, _dPos) != 0) {
        _tftRow(62, 22, 0x07FF, 2, pos);
        strcpy(_dPos, pos);
    }

    // Scan progress (size 2, y=86)
    if (_dScanIdx != scanWellIdx || _dScanTot != scanTotal) {
        snprintf(buf, sizeof(buf), "%d / %d   ", scanWellIdx, scanTotal);
        _tftRow(86, 22, 0x07E0, 2, buf);
        _dScanIdx = scanWellIdx;
        _dScanTot = scanTotal;
    }

    // Estado (size 2, y=110)
    if (_dState != (int)scanState) {
        snprintf(buf, sizeof(buf), "%s   ", scanStateName());
        _tftRow(110, 22, ST77XX_YELLOW, 2, buf);
        _dState = (int)scanState;
    }
}
