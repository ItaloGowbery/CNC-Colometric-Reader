#pragma once
#include <Arduino.h>

// Descomente quando o sensor estiver conectado fisicamente:
// #define AS7341_REAL

struct SensorReading {
    uint16_t ch415;   // violeta
    uint16_t ch445;   // azul
    uint16_t ch480;   // azul-ciano
    uint16_t ch515;   // verde
    uint16_t ch555;   // amarelo-verde
    uint16_t ch590;   // laranja
    uint16_t ch630;   // vermelho
    uint16_t ch680;   // vermelho-escuro
    bool     ok;      // false se leitura falhou
};

#ifdef AS7341_REAL
#include <Adafruit_AS7341.h>
static Adafruit_AS7341 _as7341;
#endif

inline bool sensorBegin() {
#ifdef AS7341_REAL
    if (!_as7341.begin()) return false;
    _as7341.setATIME(100);
    _as7341.setASTEP(999);
    _as7341.setGain(AS7341_GAIN_256X);
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
        true
    };
#endif
}
