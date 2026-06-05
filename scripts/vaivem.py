#!/usr/bin/env python3
"""
Faz o motor X oscilar: uma volta para frente, uma volta para trás, indefinidamente.
Uso: python3 scripts/vaivem.py [porta] [mm_por_volta]
"""
import sys
import time
import serial

PORT    = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
MM      = float(sys.argv[2]) if len(sys.argv) > 2 else 40.0
BAUD    = 115200
TIMEOUT = 30  # segundos max aguardando DONE

def send(ser, cmd):
    ser.write((cmd + "\r\n").encode())
    print(f">>> {cmd}")

def wait_done(ser):
    deadline = time.time() + TIMEOUT
    while time.time() < deadline:
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            print(f"    {line}")
        if line == "DONE":
            return
    raise TimeoutError("Motor não respondeu DONE a tempo")

def main():
    print(f"Conectando em {PORT} @ {BAUD}...")
    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        time.sleep(1.5)  # aguarda ESP32 reiniciar após abrir porta
        ser.reset_input_buffer()

        send(ser, "e")
        time.sleep(0.1)

        volta = 0
        while True:
            volta += 1
            print(f"\n--- Volta {volta} ---")
            send(ser, f"x {MM}")
            wait_done(ser)
            time.sleep(0.3)

            send(ser, f"x -{MM}")
            wait_done(ser)
            time.sleep(0.3)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrompido.")
