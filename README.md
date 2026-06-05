# CNC Colorimetric Reader

Instrumento de laboratório com dois eixos CNC para leitura automatizada de matrizes de poços usando o sensor espectral AS7341. O sistema posiciona o sensor sobre cada poço e coleta dados espectrais de múltiplos pontos, com controle via interface web.

## Hardware

| Componente | Descrição |
|---|---|
| ESP32-S3 Rev TFT Feather | Microcontrolador principal com display TFT embutido |
| CNC Shield V3 | Porta os drivers de motor |
| DRV8825 (x2) | Drivers dos motores de passo |
| NEMA 17 (x2) | Motores dos eixos X e Y |
| AS7341 | Sensor espectral de 11 canais (I2C) |

## Fiação

O CNC Shield V3 é projetado para Arduino UNO. Como o ESP32-S3 Feather tem pinout diferente, a conexão é feita com jumpers do socket Arduino do shield diretamente ao Feather.

```
Shield (socket Arduino)  →  ESP32-S3 Feather
  D2  (X STEP)           →  GPIO 5
  D5  (X DIR)            →  GPIO 6
  D3  (Y STEP)           →  GPIO 9
  D6  (Y DIR)            →  GPIO 10
  D8  (ENABLE)           →  GPIO 11
```

> Os pinos STEP/DIR/ENABLE do DRV8825 aceitam 3.3V — compatível com o ESP32-S3.

## Configuração dos motores

Edite `firmware/src/config.h` conforme o seu mecanismo de transmissão:

| Mecanismo | Microstepping DRV8825 | STEPS_PER_MM |
|---|---|---|
| Correia GT2 + polia 20 dentes | 1/32 step | 160 |
| Fuso M5 (passo 0.8 mm) | 1/32 step | 6400 |

O jumper de microstepping fica nos pinos M0/M1/M2 de cada driver no CNC Shield.

## Setup

### Dependências

```bash
# PlatformIO (compilação)
pip install platformio

# espflash (upload)
cargo install espflash

# Permissão de porta serial no Linux
sudo usermod -a -G uucp $USER
```

### Upload do firmware

```bash
cd firmware
pio run --target upload
```

Se o ESP32-S3 não entrar em modo de download automaticamente:
1. Segure **BOOT**
2. Pressione e solte **RESET**
3. Solte **BOOT**
4. Rode o comando acima

O `--monitor` é ativado automaticamente após o upload — você verá o prompt de comandos no serial.

## Fases do projeto

- [x] **Fase 1** — Controle básico dos motores via serial
- [ ] **Fase 2** — Fim de curso, homing e coordenadas absolutas
- [ ] **Fase 3** — Integração do sensor AS7341
- [ ] **Fase 4** — Rotina de scan da matriz de poços
- [ ] **Fase 5** — Interface web (controle e configuração)
- [ ] **Fase 6** — Interface web (visualização e exportação de dados)
- [ ] **Fase 7** — Display TFT local

## Comandos serial (Fase 1)

Com o monitor serial aberto (115200 baud):

| Comando | Descrição |
|---|---|
| `x <mm>` | Move o eixo X pelo valor em mm (aceita negativo) |
| `y <mm>` | Move o eixo Y pelo valor em mm (aceita negativo) |
| `e` | Habilita os motores |
| `d` | Desabilita os motores |
| `p` | Imprime a posição atual de X e Y em mm |
