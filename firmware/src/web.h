#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"

static AsyncWebServer server(80);

// Acesso aos steppers definidos em main.cpp
extern class FastAccelStepper *stepperX;
extern class FastAccelStepper *stepperY;

static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CNC Colorimetric Reader</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:sans-serif;background:#f0f2f5;padding:16px}
    h1{color:#222;margin-bottom:16px;font-size:1.4rem}
    h3{color:#444;margin-bottom:12px;font-size:1rem}
    .card{background:#fff;border-radius:10px;padding:16px;margin-bottom:14px;box-shadow:0 1px 4px rgba(0,0,0,.1)}
    .row{display:flex;gap:12px;flex-wrap:wrap;align-items:flex-end}
    .field{display:flex;flex-direction:column;gap:4px}
    label{font-size:11px;color:#666;font-weight:600;text-transform:uppercase}
    input[type=number]{width:80px;padding:7px 8px;border:1px solid #ddd;border-radius:6px;font-size:14px}
    .btn{padding:8px 18px;border:none;border-radius:6px;cursor:pointer;font-size:13px;font-weight:600}
    .btn-primary{background:#1976d2;color:#fff}.btn-primary:hover{background:#1565c0}
    .btn-secondary{background:#607d8b;color:#fff}.btn-secondary:hover{background:#546e7a}
    .btn-success{background:#388e3c;color:#fff}.btn-success:hover{background:#2e7d32}
    .btn:disabled{background:#bbb;cursor:default}
    .grid-wrap{overflow-x:auto;padding-bottom:4px}
    .grid{display:inline-block}
    .well{width:34px;height:34px;border-radius:50%;background:#e8eaf6;border:2px solid #9fa8da;
          cursor:pointer;font-size:9px;color:#555;display:flex;align-items:center;
          justify-content:center;user-select:none;transition:.12s}
    .well:hover{background:#c5cae9;border-color:#3949ab}
    .well.sel{background:#1976d2;border-color:#0d47a1;color:#fff}
    .clabel,.rlabel{display:flex;align-items:center;justify-content:center;
                    font-size:10px;color:#aaa;font-weight:700}
    .info{font-size:13px;color:#555;margin-top:10px}
    .status-bar{display:flex;gap:20px;flex-wrap:wrap;font-size:13px;color:#444}
    .status-bar span b{color:#1976d2}
    .actions{display:flex;gap:8px;flex-wrap:wrap}
  </style>
</head>
<body>
  <h1>CNC Colorimetric Reader</h1>

  <div class="card">
    <h3>Status</h3>
    <div class="status-bar">
      <span>X: <b id="sx">--</b> mm</span>
      <span>Y: <b id="sy">--</b> mm</span>
      <span>Motor: <b id="sm">--</b></span>
    </div>
  </div>

  <div class="card">
    <h3>Configuração da Placa</h3>
    <div class="row">
      <div class="field"><label>Linhas</label><input type="number" id="rows" value="8" min="1" max="16"></div>
      <div class="field"><label>Colunas</label><input type="number" id="cols" value="12" min="1" max="24"></div>
      <div class="field"><label>Espaç. X (mm)</label><input type="number" id="spX" value="9" step="0.1" min="0.1"></div>
      <div class="field"><label>Espaç. Y (mm)</label><input type="number" id="spY" value="9" step="0.1" min="0.1"></div>
      <button class="btn btn-primary" onclick="buildGrid()">Aplicar</button>
    </div>
  </div>

  <div class="card">
    <h3>Poços</h3>
    <div class="actions" style="margin-bottom:10px">
      <button class="btn btn-secondary" onclick="selAll()">Selecionar Todos</button>
      <button class="btn btn-secondary" onclick="clrAll()">Limpar</button>
    </div>
    <div class="grid-wrap"><div id="grid"></div></div>
    <p class="info" id="info">Nenhum poço selecionado.</p>
  </div>

  <div class="card">
    <h3>Controle</h3>
    <div class="actions">
      <button class="btn btn-secondary" onclick="sendCmd('h')">Definir Home Aqui</button>
      <button class="btn btn-secondary" onclick="sendCmd('e')">Habilitar Motores</button>
      <button class="btn btn-secondary" onclick="sendCmd('d')">Desabilitar Motores</button>
      <button class="btn btn-success" id="scanBtn" disabled>Escanear Selecionados</button>
    </div>
  </div>

  <script>
    const sel = new Set();

    function buildGrid() {
      const R = parseInt(document.getElementById('rows').value);
      const C = parseInt(document.getElementById('cols').value);
      const g = document.getElementById('grid');
      sel.clear();
      g.innerHTML = '';

      // header row (col labels)
      const hrow = document.createElement('div');
      hrow.style.cssText = 'display:flex;gap:5px;margin-bottom:5px';
      const corner = document.createElement('div');
      corner.style.width = '22px';
      hrow.appendChild(corner);
      for(let c=1;c<=C;c++){
        const d=document.createElement('div');
        d.className='clabel';d.style.width='34px';d.textContent=c;
        hrow.appendChild(d);
      }
      g.appendChild(hrow);

      // data rows
      for(let r=0;r<R;r++){
        const row=document.createElement('div');
        row.style.cssText='display:flex;gap:5px;margin-bottom:5px';
        const rl=document.createElement('div');
        rl.className='rlabel';rl.style.width='22px';
        rl.textContent=String.fromCharCode(65+r);
        row.appendChild(rl);
        for(let c=1;c<=C;c++){
          const w=document.createElement('div');w.className='well';
          w.dataset.r=r;w.dataset.c=c;
          w.title=String.fromCharCode(65+r)+c;
          w.onclick=()=>toggle(w);
          row.appendChild(w);
        }
        g.appendChild(row);
      }
      updateInfo();
    }

    function toggle(el){
      const id=el.dataset.r+','+el.dataset.c;
      if(sel.has(id)){sel.delete(id);el.classList.remove('sel');}
      else{sel.add(id);el.classList.add('sel');}
      updateInfo();
    }

    function selAll(){
      document.querySelectorAll('.well').forEach(w=>{sel.add(w.dataset.r+','+w.dataset.c);w.classList.add('sel');});
      updateInfo();
    }

    function clrAll(){
      document.querySelectorAll('.well').forEach(w=>{sel.delete(w.dataset.r+','+w.dataset.c);w.classList.remove('sel');});
      updateInfo();
    }

    function updateInfo(){
      const n=sel.size;
      document.getElementById('info').textContent=n>0?n+' poço(s) selecionado(s).':'Nenhum poço selecionado.';
      document.getElementById('scanBtn').disabled=n===0;
    }

    function sendCmd(cmd){
      fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({cmd})})
        .then(r=>r.json()).then(d=>console.log(d));
    }

    function pollStatus(){
      fetch('/api/status').then(r=>r.json()).then(d=>{
        document.getElementById('sx').textContent=d.x.toFixed(2);
        document.getElementById('sy').textContent=d.y.toFixed(2);
        document.getElementById('sm').textContent=d.running?'Movendo':'Parado';
      }).catch(()=>{});
    }

    buildGrid();
    setInterval(pollStatus, 500);
  </script>
</body>
</html>
)rawhtml";

inline void webBegin() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Conectando WiFi");
    for (uint8_t i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi falhou — continuando sem rede");
        return;
    }
    Serial.printf("\nWiFi OK: http://%s\n", WiFi.localIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
        StaticJsonDocument<128> doc;
        doc["x"] = stepperX->getCurrentPosition() / (float)STEPS_PER_MM;
        doc["y"] = stepperY->getCurrentPosition() / (float)STEPS_PER_MM;
        doc["running"] = stepperX->isRunning() || stepperY->isRunning();
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    server.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest *req){},
        nullptr,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            deserializeJson(doc, data, len);
            const char *cmd = doc["cmd"] | "";
            String resp = "ok";
            if      (strcmp(cmd, "h") == 0) { stepperX->setCurrentPosition(0); stepperY->setCurrentPosition(0); }
            else if (strcmp(cmd, "e") == 0) { digitalWrite(ENABLE_PIN, LOW); }
            else if (strcmp(cmd, "d") == 0) { digitalWrite(ENABLE_PIN, HIGH); }
            else resp = "unknown";
            req->send(200, "application/json", "{\"status\":\""+resp+"\"}");
        }
    );

    server.begin();
    Serial.println("Servidor web iniciado");
}
