# CNC Colorimetric Reader

A two-axis CNC laboratory instrument for automated spectral reading of well plates using the AS7341 sensor. The system positions the sensor over each well, collects spectral data from multiple points per well, and displays real-time status via TFT display and web interface.

## Hardware

| Component | Description |
|---|---|
| Adafruit ESP32-S3 Reverse TFT Feather | Main microcontroller with built-in 1.14" TFT display |
| CNC Shield V3 | Stepper driver carrier |
| DRV8825 (x2) | Stepper motor drivers |
| NEMA 17 (x2) | X and Y axis stepper motors |
| AS7341 | 11-channel spectral sensor (I2C) |

## Wiring

The CNC Shield V3 is designed for Arduino UNO. Since the ESP32-S3 Feather has a different pinout, connections are made with jumpers from the shield's Arduino socket directly to the Feather.

```
Shield (Arduino socket)  →  ESP32-S3 Feather
  5V                     →  USB
  GND                    →  GND
  D2  (X STEP)           →  GPIO 5
  D5  (X DIR)            →  GPIO 6
  D3  (Y STEP)           →  GPIO 9
  D6  (Y DIR)            →  GPIO 10
  D8  (ENABLE)           →  GPIO 11
```

> The DRV8825 STEP/DIR/ENABLE pins accept 3.3V — compatible with the ESP32-S3.

## Motor Configuration

Edit `firmware/src/config.h` according to your drive mechanism:

| Mechanism | DRV8825 Microstepping | STEPS_PER_MM |
|---|---|---|
| GT2 belt + 20T pulley | 1/32 step | 160 |
| M5 leadscrew (0.8 mm pitch) | 1/32 step | 6400 |

The microstepping jumper is on the M0/M1/M2 pins of each driver on the CNC Shield.

### Travel Limits and Origin

Also in `firmware/src/config.h`:

| Parameter | Default | Description |
|---|---|---|
| `X_MAX_MM` | 210.0 | Maximum X axis travel in mm |
| `Y_MAX_MM` | 290.0 | Maximum Y axis travel in mm |
| `X_ORIGIN_MM` | 5.0 | Distance from home to the first well corner (X) |
| `Y_ORIGIN_MM` | 5.0 | Distance from home to the first well corner (Y) |
| `MAX_SPEED_MM_S` | 25.0 | Maximum speed in mm/s |
| `ACCEL_MM_S2` | 150.0 | Acceleration in mm/s² |

## Setup

### Dependencies

```bash
# PlatformIO
pip install platformio

# Serial port permission (Linux)
sudo usermod -a -G uucp $USER
```

### Uploading the Firmware

```bash
cd firmware
pio run --target upload
```

If the ESP32-S3 does not enter download mode automatically:
1. Hold **BOOT**
2. Press and release **RESET**
3. Release **BOOT**
4. Run the command above

## Project Phases

- [x] **Phase 1** — Basic motor control via serial
- [x] **Phase 2** — AS7341 sensor integration
- [x] **Phase 3** — Well plate scan routine
- [x] **Phase 4** — Web interface (control and configuration)
- [x] **Phase 5** — Web interface (data visualization and export)
- [x] **Phase 6** — Local TFT display
- [ ] **Phase 7** — Z-axis control with servo motor
- [ ] **Phase 8** — Endstops, homing and absolute coordinates

## TFT Display

The display shows real-time status without requiring a PC connection:

```
CNC Colorimetric
Reader
IP: 192.168.x.xxx
X:12.3  Y:56.7mm
0 / 144
waiting
```

| Field | Description |
|---|---|
| IP | Web interface address |
| X / Y | Current axis position in mm |
| Progress | Current well / total wells |
| State | `waiting`, `moving`, `reading` or `done` |

## Serial Commands

With the serial monitor open at 115200 baud:

| Command | Description |
|---|---|
| `x <mm>` | Move X axis by the given value in mm (accepts negative) |
| `y <mm>` | Move Y axis by the given value in mm (accepts negative) |
| `e` | Enable motors |
| `d` | Disable motors |
| `p` | Print current X and Y position in mm |
| `r` | Read AS7341 sensor and print all 8 channels |
| `h` | Set current position as home (0, 0) |

## Web Interface

Connect to the Wi-Fi network configured in `config.h` and open the IP shown on the display or serial monitor.

- **Plate configuration** — set rows, columns and well spacing (default: 12×12, 15 mm)
- **Well selection** — click wells on the grid to select which ones will be scanned
- **Points per well** — number of sampling points, margin and well size; SVG preview shows point distribution
- **Manual jog** — move X and Y in steps of 0.1, 1, 5 or 10 mm
- **Scan** — start scanning selected wells with a real-time progress bar
- **Results** — table with individual readings per point for all 8 AS7341 channels, with CSV export

## REST API

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/api/status` | Current position, scan state and progress |
| `POST` | `/api/cmd` | Send serial command (`e`, `d`, `h`, `x <mm>`, `y <mm>`) |
| `POST` | `/api/move` | Move relatively in X and/or Y (JSON: `{x, y}` in mm) |
| `POST` | `/api/scan` | Start scan (JSON: `{wells, spacingX, spacingY, points}`) |
| `GET` | `/api/results` | Return individual point readings for each scanned well |
