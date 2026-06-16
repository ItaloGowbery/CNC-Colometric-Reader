#pragma once
#include <Arduino.h>

#define AS7341_REAL

struct SensorReading {
    uint16_t ch415;   // violeta
    uint16_t ch445;   // azul
    uint16_t ch480;   // azul-ciano
    uint16_t ch515;   // verde
    uint16_t ch555;   // amarelo-verde
    uint16_t ch590;   // laranja
    uint16_t ch630;   // vermelho
    uint16_t ch680;   // vermelho-escuro
    uint16_t clear;   // luz visível total
    uint16_t nir;     // 910nm infravermelho próximo
    bool     ok;      // false se leitura falhou
};

#ifdef AS7341_REAL
#include <Adafruit_AS7341.h>
static Adafruit_AS7341 _as7341;
#endif

inline void sensorSetLED(uint8_t mA) {
#ifdef AS7341_REAL
    _as7341.setLEDCurrent(mA);
#endif
}

inline void sensorSetConfig(float integrationMs, uint8_t gainIdx) {
#ifdef AS7341_REAL
    static const as7341_gain_t gains[] = {
        AS7341_GAIN_0_5X, AS7341_GAIN_1X,  AS7341_GAIN_2X,   AS7341_GAIN_4X,
        AS7341_GAIN_8X,   AS7341_GAIN_16X, AS7341_GAIN_32X,  AS7341_GAIN_64X,
        AS7341_GAIN_128X, AS7341_GAIN_256X, AS7341_GAIN_512X
    };
    if (gainIdx > 10) gainIdx = 10;
    // ASTEP=5619 → tick=15.6ms; ATIME=0..255 cobre 15ms..4003ms
    uint8_t atime = (uint8_t)constrain((int)round(integrationMs / 15.6f) - 1, 0, 255);
    _as7341.setATIME(atime);
    _as7341.setASTEP(5619);
    _as7341.setGain(gains[gainIdx]);
#endif
}

inline bool sensorBegin() {
#ifdef AS7341_REAL
    if (!_as7341.begin()) return false;
    _as7341.setATIME(4);    // ~78ms com ASTEP=5619
    _as7341.setASTEP(5619);
    _as7341.setGain(AS7341_GAIN_16X);
    _as7341.enableLED(true);
    _as7341.setLEDCurrent(10);
    return true;
#else
    Serial.println("[SENSOR] Modo stub ativo (sem hardware)");
    return true;
#endif
}

inline SensorReading sensorRead() {
#ifdef AS7341_REAL
    if (!_as7341.readAllChannels()) return {0,0,0,0,0,0,0,0,false};
    return {
        _as7341.getChannel(AS7341_CHANNEL_415nm_F1),
        _as7341.getChannel(AS7341_CHANNEL_445nm_F2),
        _as7341.getChannel(AS7341_CHANNEL_480nm_F3),
        _as7341.getChannel(AS7341_CHANNEL_515nm_F4),
        _as7341.getChannel(AS7341_CHANNEL_555nm_F5),
        _as7341.getChannel(AS7341_CHANNEL_590nm_F6),
        _as7341.getChannel(AS7341_CHANNEL_630nm_F7),
        _as7341.getChannel(AS7341_CHANNEL_680nm_F8),
        _as7341.getChannel(AS7341_CHANNEL_CLEAR),
        _as7341.getChannel(AS7341_CHANNEL_NIR),
        true
    };
#else
    // Dados simulados com variação aleatória
    return {
        (uint16_t)(1000 + random(200)),
        (uint16_t)(1500 + random(200)),
        (uint16_t)(2000 + random(200)),
        (uint16_t)(3000 + random(200)),
        (uint16_t)(2800 + random(200)),
        (uint16_t)(2200 + random(200)),
        (uint16_t)(1800 + random(200)),
        (uint16_t)(1200 + random(200)),
        (uint16_t)(5000 + random(500)),
        (uint16_t)(800  + random(100)),
        true
    };
#endif
}
