# CNC Colorimetric Reader

Instrumento de laboratório com dois eixos CNC para leitura automatizada de matrizes de poços usando o sensor espectral AS7341. O sistema posiciona o sensor sobre cada poço, coleta dados espectrais de múltiplos pontos por poço e exibe o status em tempo real via display TFT e interface web.

## Hardware

| Componente | Descrição |
|---|---|
| Adafruit ESP32-S3 Reverse TFT Feather | Microcontrolador principal com display TFT 1.14" embutido |
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

### Limites de curso e origem

Também em `firmware/src/config.h`:

| Parâmetro | Valor padrão | Descrição |
|---|---|---|
| `X_MAX_MM` | 210.0 | Limite máximo do eixo X em mm |
| `Y_MAX_MM` | 290.0 | Limite máximo do eixo Y em mm |
| `X_ORIGIN_MM` | 5.0 | Distância do home até a quina do primeiro poço (X) |
| `Y_ORIGIN_MM` | 5.0 | Distância do home até a quina do primeiro poço (Y) |
| `MAX_SPEED_MM_S` | 25.0 | Velocidade máxima em mm/s |
| `ACCEL_MM_S2` | 150.0 | Aceleração em mm/s² |

## Setup

### Dependências

```bash
# PlatformIO (compilação)
pip install platformio

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

## Fases do projeto

- [x] **Fase 1** — Controle básico dos motores via serial
- [ ] **Fase 2** — Fim de curso, homing e coordenadas absolutas
- [x] **Fase 3** — Integração do sensor AS7341
- [x] **Fase 4** — Rotina de scan da matriz de poços
- [x] **Fase 5** — Interface web (controle e configuração)
- [x] **Fase 6** — Interface web (visualização e exportação de dados)
- [x] **Fase 7** — Display TFT local

## Display TFT

O display exibe em tempo real, sem necessidade de conexão ao PC:

```
CNC Colorimetric
Reader
IP: 192.168.x.xxx
X:12.3  Y:56.7mm
0 / 144
waiting
```

| Campo | Descrição |
|---|---|
| IP | Endereço da interface web |
| X / Y | Posição atual dos eixos em mm |
| Progresso | Poço atual / total de poços |
| Estado | `waiting`, `moving`, `reading` ou `done` |

## Comandos serial

Com o monitor serial aberto (115200 baud):

| Comando | Descrição |
|---|---|
| `x <mm>` | Move o eixo X pelo valor em mm (aceita negativo) |
| `y <mm>` | Move o eixo Y pelo valor em mm (aceita negativo) |
| `e` | Habilita os motores |
| `d` | Desabilita os motores |
| `p` | Imprime a posição atual de X e Y em mm |
| `r` | Lê o sensor AS7341 e imprime os 8 canais |
| `h` | Define a posição atual como home (0, 0) |

## Interface Web

Conecte-se à rede Wi-Fi configurada em `config.h` e acesse o IP exibido no display ou no serial.

- **Configuração da matriz** — linhas, colunas e espaçamento entre poços (padrão: 12×12, 15mm)
- **Seleção de poços** — clique nos poços da grade para selecionar quais serão escaneados
- **Pontos por poço** — número de pontos, margem e tamanho do poço; prévia SVG mostra a distribuição
- **Movimento manual** — jog em X e Y com passos de 0.1, 1, 5 ou 10 mm
- **Scan** — inicia o escaneamento com barra de progresso em tempo real
- **Resultados** — tabela com leituras individuais por ponto dos 8 canais do AS7341, com exportação CSV

## API REST

| Método | Endpoint | Descrição |
|---|---|---|
| `GET` | `/api/status` | Posição atual, estado do scan e progresso |
| `POST` | `/api/cmd` | Envia comando serial (`e`, `d`, `h`, `x <mm>`, `y <mm>`) |
| `POST` | `/api/move` | Move relativamente em X e/ou Y (JSON: `{x, y}` em mm) |
| `POST` | `/api/scan` | Inicia o scan (JSON: `{wells, spacingX, spacingY, points}`) |
| `GET` | `/api/results` | Retorna leituras individuais por ponto de cada poço escaneado |
