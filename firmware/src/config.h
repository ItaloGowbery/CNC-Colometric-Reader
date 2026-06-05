#pragma once

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
#define WIFI_SSID  "WiFi_MZNET_2G"
#define WIFI_PASS  "rede142536"

// ---------------------------------------------------------------------------
// Fiação: conectar jumpers do socket Arduino do CNC Shield ao Feather
//
//   Shield (socket)  →  ESP32-S3 Feather
//   D2 (X STEP)      →  GPIO 5
//   D5 (X DIR)       →  GPIO 6
//   D3 (Y STEP)      →  GPIO 9
//   D6 (Y DIR)       →  GPIO 10
//   D8 (ENABLE)      →  GPIO 11   (compartilhado, ativo em LOW)
// ---------------------------------------------------------------------------
#define X_STEP_PIN   5
#define X_DIR_PIN    6
#define Y_STEP_PIN   9
#define Y_DIR_PIN    10
#define ENABLE_PIN   11

// ---------------------------------------------------------------------------
// Parâmetros — ajustar conforme o mecanismo de transmissão
//
//   Correia GT2, polia 20 dentes, DRV8825 1/8 step:
//     passos/volta = 200 * 8 = 1600
//     mm/volta     = 20 * 2mm = 40mm   →   STEPS_PER_MM = 40
//
//   Fuso M5 (passo 0.8mm), DRV8825 1/8 step:
//     passos/volta = 200 * 8 = 1600
//     mm/volta     = 0.8mm              →   STEPS_PER_MM = 2000
// ---------------------------------------------------------------------------
#define STEPS_PER_MM      160     // GT2 20T + DRV8825 1/32 step: 200*32/40mm
#define MAX_SPEED_MM_S    50.0f
#define ACCEL_MM_S2       150.0f
