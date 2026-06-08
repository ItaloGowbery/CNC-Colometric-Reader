#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "scan.h"

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
    <h3>Pontos por Poço</h3>
    <div class="row" style="margin-bottom:14px">
      <div class="field"><label>Nº de Pontos</label><input type="number" id="numPts" value="1" min="1" max="25" oninput="updatePreview()"></div>
      <div class="field"><label>Margem (mm)</label><input type="number" id="margin" value="1" step="0.1" min="0" max="4.9" oninput="updatePreview()"></div>
      <div class="field"><label>Tamanho poço (mm)</label><input type="number" id="wellSize" value="10" step="0.1" min="1" oninput="updatePreview()"></div>
    </div>
    <div style="display:flex;gap:20px;align-items:flex-start;flex-wrap:wrap">
      <div>
        <div style="font-size:11px;color:#666;font-weight:600;margin-bottom:6px">PRÉVIA DO POÇO</div>
        <svg id="preview" width="180" height="180" style="border:1px solid #ddd;border-radius:4px;background:#fafafa">
          <rect id="pvWell" x="10" y="10" width="160" height="160" fill="#e8eaf6" stroke="#9fa8da" stroke-width="2"/>
          <rect id="pvMargin" x="26" y="26" width="128" height="128" fill="none" stroke="#ef9a9a" stroke-width="1" stroke-dasharray="4,3"/>
          <g id="pvPoints"></g>
        </svg>
      </div>
      <div id="ptsList" style="font-size:12px;color:#555;max-height:180px;overflow-y:auto"></div>
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
    <h3>Movimento Manual</h3>
    <div style="margin-bottom:10px">
      <label style="font-size:11px;color:#666;font-weight:600">PASSO (mm)&nbsp;</label>
      <span id="steps">
        <button class="btn btn-secondary step-btn" onclick="setStep(0.1)">0.1</button>
        <button class="btn btn-primary  step-btn" onclick="setStep(1)">1</button>
        <button class="btn btn-secondary step-btn" onclick="setStep(5)">5</button>
        <button class="btn btn-secondary step-btn" onclick="setStep(10)">10</button>
      </span>
    </div>
    <div style="display:flex;gap:24px;flex-wrap:wrap;align-items:center">
      <div>
        <div style="text-align:center;margin-bottom:4px;font-size:11px;color:#666;font-weight:600">EIXO X</div>
        <div style="display:flex;gap:6px">
          <button class="btn btn-primary" onclick="move(-step,0)">&#8592; X-</button>
          <button class="btn btn-primary" onclick="move(step,0)">X+ &#8594;</button>
        </div>
      </div>
      <div>
        <div style="text-align:center;margin-bottom:4px;font-size:11px;color:#666;font-weight:600">EIXO Y</div>
        <div style="display:flex;gap:6px">
          <button class="btn btn-primary" onclick="move(0,-step)">&#8593; Y-</button>
          <button class="btn btn-primary" onclick="move(0,step)">Y+ &#8595;</button>
        </div>
      </div>
    </div>
  </div>

  <div class="card">
    <h3>Controle</h3>
    <div class="actions">
      <button class="btn btn-secondary" onclick="sendCmd('h')">Definir Home Aqui</button>
      <button class="btn btn-secondary" onclick="sendCmd('e')">Habilitar Motores</button>
      <button class="btn btn-secondary" onclick="sendCmd('d')">Desabilitar Motores</button>
      <button class="btn btn-success" id="scanBtn" disabled onclick="startScan()">Escanear Selecionados</button>
    </div>
    <div id="scanProgress" style="display:none;margin-top:12px">
      <div style="background:#e0e0e0;border-radius:6px;height:10px;overflow:hidden">
        <div id="progressBar" style="background:#388e3c;height:100%;width:0%;transition:.3s"></div>
      </div>
      <p id="progressTxt" style="font-size:13px;color:#555;margin-top:6px"></p>
    </div>
  </div>

  <div class="card" id="resultsCard" style="display:none">
    <h3>Resultados</h3>
    <div style="overflow-x:auto">
      <table id="resultsTable" style="border-collapse:collapse;font-size:12px;width:100%">
        <thead>
          <tr style="background:#e8eaf6">
            <th style="padding:6px 8px;text-align:left">Poço</th>
            <th style="padding:6px 8px">Ponto</th>
            <th style="padding:6px 8px">415nm</th><th style="padding:6px 8px">445nm</th>
            <th style="padding:6px 8px">480nm</th><th style="padding:6px 8px">515nm</th>
            <th style="padding:6px 8px">555nm</th><th style="padding:6px 8px">590nm</th>
            <th style="padding:6px 8px">630nm</th><th style="padding:6px 8px">680nm</th>
          </tr>
        </thead>
        <tbody id="resultsBody"></tbody>
      </table>
    </div>
    <button class="btn btn-secondary" style="margin-top:10px" onclick="exportCSV()">Exportar CSV</button>
  </div>

  <script>
    const sel = new Set();
    let step = 1;

    function computePoints(N, wellSize, margin) {
      const usable = wellSize - 2 * margin;
      if (N === 1 || usable <= 0) return [{x: wellSize/2, y: wellSize/2}];
      let rows = 1, cols = N;
      for (let r = 1; r <= N; r++) {
        const c = Math.ceil(N / r);
        if (r * c >= N && Math.abs(r-c) < Math.abs(rows-cols)) { rows=r; cols=c; }
      }
      const pts = [];
      let count = 0;
      for (let r = 0; r < rows && count < N; r++) {
        for (let c = 0; c < cols && count < N; c++) {
          const x = margin + (cols > 1 ? c*usable/(cols-1) : usable/2);
          const y = margin + (rows > 1 ? r*usable/(rows-1) : usable/2);
          pts.push({x, y}); count++;
        }
      }
      return pts;
    }

    function updatePreview() {
      const N    = parseInt(document.getElementById('numPts').value)   || 1;
      const mg   = parseFloat(document.getElementById('margin').value) || 0;
      const ws   = parseFloat(document.getElementById('wellSize').value)|| 10;
      const scale = 160 / ws;   // pixels per mm (160px usable in 180px SVG)
      const off   = 10;         // SVG padding

      // margin rect
      const mx = off + mg * scale, my = off + mg * scale;
      const ms = (ws - 2*mg) * scale;
      document.getElementById('pvMargin').setAttribute('x', mx);
      document.getElementById('pvMargin').setAttribute('y', my);
      document.getElementById('pvMargin').setAttribute('width',  Math.max(0,ms));
      document.getElementById('pvMargin').setAttribute('height', Math.max(0,ms));

      const pts = computePoints(N, ws, mg);
      const g   = document.getElementById('pvPoints');
      g.innerHTML = '';
      let list = '<b>Pontos (mm):</b><br>';
      pts.forEach((p, i) => {
        const cx = off + p.x * scale, cy = off + p.y * scale;
        const c = document.createElementNS('http://www.w3.org/2000/svg','circle');
        c.setAttribute('cx', cx); c.setAttribute('cy', cy);
        c.setAttribute('r', 5); c.setAttribute('fill','#1976d2');
        c.setAttribute('opacity','0.85');
        const t = document.createElementNS('http://www.w3.org/2000/svg','text');
        t.setAttribute('x', cx); t.setAttribute('y', cy+3.5);
        t.setAttribute('text-anchor','middle');
        t.setAttribute('font-size','7'); t.setAttribute('fill','#fff');
        t.textContent = i+1;
        g.appendChild(c); g.appendChild(t);
        list += (i+1)+': ('+p.x.toFixed(2)+', '+p.y.toFixed(2)+')<br>';
      });
      document.getElementById('ptsList').innerHTML = list;
    }

    function setStep(v) {
      step = v;
      document.querySelectorAll('.step-btn').forEach(b => {
        b.className = 'btn btn-secondary step-btn';
      });
      event.target.className = 'btn btn-primary step-btn';
    }

    function move(x, y) {
      fetch('/api/move', {method:'POST', headers:{'Content-Type':'application/json'},
        body: JSON.stringify({x, y})});
    }

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

    let scanning = false;

    function startScan() {
      const spX = parseFloat(document.getElementById('spX').value);
      const spY = parseFloat(document.getElementById('spY').value);
      const N   = parseInt(document.getElementById('numPts').value) || 1;
      const mg  = parseFloat(document.getElementById('margin').value) || 0;
      const ws  = parseFloat(document.getElementById('wellSize').value) || 10;
      const wells = Array.from(sel).map(id => {
        const [r,c] = id.split(',');
        return {r:parseInt(r), c:parseInt(c)};
      });
      const points = computePoints(N, ws, mg);
      fetch('/api/scan', {method:'POST', headers:{'Content-Type':'application/json'},
        body: JSON.stringify({wells, spacingX:spX, spacingY:spY, points})})
        .then(r=>r.json()).then(()=>{
          scanning = true;
          document.getElementById('scanProgress').style.display='block';
          document.getElementById('resultsCard').style.display='none';
          document.getElementById('resultsBody').innerHTML='';
          document.getElementById('scanBtn').disabled=true;
        });
    }

    function pollStatus(){
      fetch('/api/status').then(r=>r.json()).then(d=>{
        document.getElementById('sx').textContent=d.x.toFixed(2);
        document.getElementById('sy').textContent=d.y.toFixed(2);
        document.getElementById('sm').textContent=d.running?'Movendo':'Parado';
        if (scanning) {
          const pct = d.scanTotal>0 ? (d.scanIdx/d.scanTotal*100) : 0;
          document.getElementById('progressBar').style.width=pct+'%';
          document.getElementById('progressTxt').textContent=
            d.scan==='done'
              ? 'Scan concluído!'
              : 'Poço '+d.scanIdx+' / '+d.scanTotal;
          if (d.scan==='done') {
            scanning=false;
            loadResults();
          }
        }
      }).catch(()=>{});
    }

    function loadResults() {
      fetch('/api/results').then(r=>r.json()).then(data=>{
        const body=document.getElementById('resultsBody');
        const letters='ABCDEFGHIJKLMNOP';
        data.forEach((r,i)=>{
          const tr=document.createElement('tr');
          tr.style.background=i%2?'#f5f5f5':'#fff';
          const label=letters[r.row]+(r.col+1);
          tr.innerHTML='<td style="padding:5px 8px;font-weight:600">'+label+'</td>'+
            '<td style="padding:5px 8px;text-align:center">'+(r.point+1)+'</td>'+
            [r['415'],r['445'],r['480'],r['515'],r['555'],r['590'],r['630'],r['680']]
            .map(v=>'<td style="padding:5px 8px;text-align:center">'+v+'</td>').join('');
          body.appendChild(tr);
        });
        document.getElementById('resultsCard').style.display='block';
        document.getElementById('scanBtn').disabled=sel.size===0;
      });
    }

    function exportCSV() {
      fetch('/api/results').then(r=>r.json()).then(data=>{
        const letters='ABCDEFGHIJKLMNOP';
        let csv='Poco,Ponto,415nm,445nm,480nm,515nm,555nm,590nm,630nm,680nm\n';
        data.forEach(r=>{
          csv+=letters[r.row]+(r.col+1)+','+(r.point+1)+','+
            [r['415'],r['445'],r['480'],r['515'],r['555'],r['590'],r['630'],r['680']].join(',')+'\n';
        });
        const a=document.createElement('a');
        a.href='data:text/csv;charset=utf-8,'+encodeURIComponent(csv);
        a.download='scan_results.csv';a.click();
      });
    }

    buildGrid();
    updatePreview();
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
        StaticJsonDocument<192> doc;
        doc["x"]            = stepperX->getCurrentPosition() / (float)STEPS_PER_MM;
        doc["y"]            = stepperY->getCurrentPosition() / (float)STEPS_PER_MM;
        doc["running"]      = stepperX->isRunning() || stepperY->isRunning();
        doc["scan"]         = scanStateName();
        doc["scanIdx"]      = scanWellIdx;
        doc["scanTotal"]    = scanTotal;
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

    server.on("/api/move", HTTP_POST, [](AsyncWebServerRequest *req){},
        nullptr,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
            StaticJsonDocument<64> doc;
            deserializeJson(doc, data, len);
            float x = doc["x"] | 0.0f;
            float y = doc["y"] | 0.0f;
            digitalWrite(ENABLE_PIN, LOW);
            if (x != 0) {
                float tgt = constrain(stepperX->getCurrentPosition() / (float)STEPS_PER_MM + x, 0.0f, X_MAX_MM);
                stepperX->moveTo((int32_t)(tgt * STEPS_PER_MM));
            }
            if (y != 0) {
                float tgt = constrain(stepperY->getCurrentPosition() / (float)STEPS_PER_MM + y, 0.0f, Y_MAX_MM);
                stepperY->moveTo((int32_t)(tgt * STEPS_PER_MM));
            }
            req->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    server.on("/api/scan", HTTP_POST, [](AsyncWebServerRequest *req){},
        nullptr,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
            DynamicJsonDocument doc(8192);
            deserializeJson(doc, data, len);
            float spX = doc["spacingX"] | 9.0f;
            float spY = doc["spacingY"] | 9.0f;
            static WellPos   wbuf[MAX_WELLS];
            static PointOffset pbuf[MAX_POINTS];
            int nw = 0, np = 0;
            for (JsonObject w : doc["wells"].as<JsonArray>()) {
                if (nw >= MAX_WELLS) break;
                wbuf[nw++] = { (uint8_t)(int)w["r"], (uint8_t)(int)w["c"] };
            }
            for (JsonObject p : doc["points"].as<JsonArray>()) {
                if (np >= MAX_POINTS) break;
                pbuf[np++] = { p["x"] | 5.0f, p["y"] | 5.0f };
            }
            if (np == 0) { pbuf[0] = {5.0f, 5.0f}; np = 1; }  // centro como fallback
            if (nw > 0) scanStart(wbuf, nw, pbuf, np, spX, spY);
            req->send(200, "application/json", "{\"status\":\"started\",\"total\":" + String(nw) + "}");
        }
    );

    server.on("/api/results", HTTP_GET, [](AsyncWebServerRequest *req) {
        DynamicJsonDocument doc(16384);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < scanResultCount; i++) {
            JsonObject o = arr.createNestedObject();
            o["row"]   = scanResults[i].pos.row;
            o["col"]   = scanResults[i].pos.col;
            o["point"] = scanResults[i].pointIdx;
            o["415"]   = scanResults[i].ch415;
            o["445"]   = scanResults[i].ch445;
            o["480"]   = scanResults[i].ch480;
            o["515"]   = scanResults[i].ch515;
            o["555"]   = scanResults[i].ch555;
            o["590"]   = scanResults[i].ch590;
            o["630"]   = scanResults[i].ch630;
            o["680"]   = scanResults[i].ch680;
        }
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    server.begin();
    Serial.println("Servidor web iniciado");
}
