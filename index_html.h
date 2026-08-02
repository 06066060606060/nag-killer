const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>T2CAN Unified</title>
<style>
  :root {
    --bg: #0d0d0d;
    --panel: #161618;
    --card: #1c1c1e;
    --line: #2c2c2e;
    --txt: #f5f5f7;
    --muted: #8e8e93;
    --accent: #0a84ff;
    --ok: #30d158;
    --warn: #ff9f0a;
    --bad: #ff453a;
  }
  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
  body {
    margin: 0;
    background: var(--bg);
    color: var(--txt);
    font: 15px/1.45 -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
  }
  header {
    padding: 18px 20px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 10px;
  }
  header h1 {
    margin: 0;
    font-size: 18px;
    font-weight: 700;
    color: var(--txt);
    display: flex;
    align-items: center;
    gap: 8px;
  }
  header h1::before {
    content: "⚡";
    font-size: 14px;
    opacity: 0.7;
  }
  header .pill {
    font-size: 12px;
    padding: 5px 12px;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 20px;
    color: var(--muted);
    display: flex;
    align-items: center;
    gap: 6px;
    font-weight: 500;
  }
  header .pill::before {
    content: "";
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--warn);
    display: inline-block;
  }
  header .pill.ok::before { background: var(--ok); }
  header .pill.bad::before { background: var(--bad); }
  .tabs {
    display: flex;
    gap: 8px;
    padding: 0 20px 14px;
    max-width: 460px;
    margin: 0 auto;
    flex-wrap: wrap;
  }
  .tab-btn {
    background: var(--card);
    border: 1px solid var(--line);
    color: var(--muted);
    padding: 9px 16px;
    border-radius: 12px;
    cursor: pointer;
    font: inherit;
    font-size: 13px;
    font-weight: 600;
    transition: all 0.15s ease;
  }
  .tab-btn:hover { filter: brightness(1.2); }
  .tab-btn.active { background: var(--txt); color: var(--bg); border-color: transparent; }
  main {
    max-width: 460px;
    margin: 0 auto;
    padding: 0 16px 24px;
    display: none;
    gap: 16px;
  }
  main.active { display: grid; }
  .panel {
    background: var(--panel);
    border: 1px solid var(--line);
    border-radius: 20px;
    padding: 18px;
  }
  .panel h2 {
    margin: 0 0 14px;
    font-size: 14px;
    font-weight: 700;
    color: var(--txt);
  }
  .panel h2 .iface-note {
    font-size: 10px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: .06em;
    color: var(--muted);
    margin-left: 8px;
  }
  .row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
  }
  .row.row3 { grid-template-columns: 1fr 1fr 1fr; }
  .stat {
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 14px;
    padding: 14px 12px;
    min-height: 80px;
    display: flex;
    flex-direction: column;
    justify-content: center;
  }
  .stat .k {
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--muted);
    font-weight: 600;
    line-height: 1.3;
  }
  .stat .v {
    font-size: 20px;
    font-weight: 700;
    margin-top: 6px;
    color: var(--txt);
    letter-spacing: -0.02em;
    overflow-wrap: anywhere;
  }
  .stat.full { grid-column: 1 / -1; }
  .big-state {
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 14px;
    padding: 18px 14px;
    font-size: 28px;
    font-weight: 700;
    letter-spacing: -0.02em;
    text-align: center;
  }
  .big-state.off  { color: var(--txt); }
  .big-state.on   { color: var(--ok); }
  .big-state.warn { color: var(--warn); }
  .big-state.bad  { color: var(--bad); }
  .tbar {
    display: flex;
    gap: 10px;
    margin-top: 14px;
    flex-wrap: wrap;
  }
  button {
    font: inherit;
    cursor: pointer;
    background: var(--card);
    color: var(--txt);
    border: 1px solid var(--line);
    border-radius: 12px;
    padding: 10px 20px;
    font-size: 13px;
    font-weight: 600;
    transition: all 0.15s ease;
  }
  button:hover { filter: brightness(1.2); }
  button:active { transform: scale(0.97); }
  button[disabled] { opacity: .5; cursor: not-allowed; }
  button.primary {
    background: var(--txt);
    color: var(--bg);
    border-color: transparent;
  }
  button.danger {
    background: var(--card);
    color: var(--bad);
    border-color: #3a1f23;
  }
  button.warn {
    background: var(--warn);
    color: #000;
    border-color: transparent;
  }
  .desc {
    font-size: 12px;
    color: var(--muted);
    line-height: 1.6;
    margin-top: 8px;
  }
  .desc b { color: var(--txt); font-weight: 600; }
  .ok { color: var(--ok); }
  .warn { color: var(--warn); }
  .bad { color: var(--bad); }
  .footer {
    color: var(--muted);
    font-size: 11px;
    text-align: center;
    padding: 14px 0;
    line-height: 1.6;
  }
  .footer a { color: var(--muted); text-decoration: none; }
  label {
    display: flex;
    flex-direction: column;
    gap: 5px;
    color: var(--muted);
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 600;
  }
  input[type=text], input[type=number] {
    background: var(--card);
    border: 1px solid var(--line);
    color: var(--txt);
    border-radius: 10px;
    padding: 9px 10px;
    font: 13px/1.4 -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    width: 100%;
  }
  input[type=file] {
    width: 100%;
    font-size: 12px;
    color: var(--muted);
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 12px;
    padding: 10px 12px;
  }
  table { width: 100%; border-collapse: collapse; margin-top: 8px; }
  th, td { text-align: left; padding: 7px 6px; border-bottom: 1px solid var(--line); font-size: 12px; }
  th { color: var(--muted); font-weight: 600; text-transform: uppercase; letter-spacing: .05em; font-size: 10px; }
  td input { width: 90%; }
  details { margin-top: 14px; }
  summary { cursor: pointer; color: var(--muted); font-size: 12px; font-weight: 600; }
  .progress {
    width: 100%;
    height: 8px;
    background: var(--card);
    border: 1px solid var(--line);
    border-radius: 6px;
    overflow: hidden;
    margin-top: 12px;
    display: none;
  }
  .progress.show { display: block; }
  .progress-bar {
    height: 100%;
    width: 0%;
    background: var(--accent);
    transition: width 0.15s ease;
  }
  .ota-msg {
    font-size: 12px;
    margin-top: 10px;
    color: var(--muted);
  }
  .ota-msg.ok  { color: var(--ok); }
  .ota-msg.bad { color: var(--bad); }
  .gate-status {
    text-align: center;
    font-size: 13px;
    font-weight: 700;
    padding: 10px;
    border-radius: 12px;
    border: 1px solid var(--line);
    background: var(--card);
  }
  .gate-status.open   { color: var(--ok);  border-color: var(--ok); }
  .gate-status.closed { color: var(--bad); border-color: #3a1f23; }
  .gbox.active .v { color: var(--ok); }
  .toast {
    position: fixed;
    bottom: 18px;
    left: 50%;
    transform: translateX(-50%);
    background: var(--txt);
    color: var(--bg);
    padding: 8px 16px;
    border-radius: 14px;
    font-size: 12px;
    font-weight: 700;
    opacity: 0;
    transition: opacity .25s;
    z-index: 10;
  }
  .toast.show { opacity: 1; }
</style>
</head>
<body>
<header>
  <h1>T2CAN Unified <span id="hdr_ver" style="color:var(--muted);font-weight:600;font-size:13px;">—</span></h1>
  <span class="pill" id="conn">connecting…</span>
</header>
<div class="tabs">
  <button class="tab-btn active" onclick="showTab('nag',this)">Nag Killer</button>
  <button class="tab-btn" onclick="showTab('summon',this)">Summon Unlock</button>
  <button class="tab-btn" onclick="showTab('fw',this)">Firmware</button>
</div>

<!-- NAG ECHO TAB -->
<main id="main-nag" class="active">

  <section class="panel">
    <h2>Mode <span class="iface-note">CAN A — MCP2515</span></h2>
    <div class="tbar" style="margin-top:0">
      <button id="modeA" class="primary">A — Simple</button>
      <button id="modeB">B — TSL6P (burst/pause)</button>
      <button id="modeR" class="danger" style="margin-left:auto">Reset</button>
    </div>
    <div class="desc">
      <b>A</b>: CAN 0x370, fixed +1.80 Nm, handsOn=1 always.<br>
      <b>B</b>: Configurable target CAN ID, default 0x370, torque cycle, time-bursty (<span id="lbl_burst">1000</span> ms inject / <span id="lbl_pause">1500</span> ms rest).
    </div>
  </section>

  <section class="panel">
    <h2>Live</h2>
    <div class="row">
      <div class="stat"><div class="k">Enabled</div><div class="v" id="s_en">—</div></div>
      <div class="stat"><div class="k">Rx frames</div><div class="v" id="s_rx">0</div></div>
      <div class="stat"><div class="k">Echo sent</div><div class="v" id="s_echo">0</div></div>
      <div class="stat"><div class="k">Tx ok / fail</div><div class="v" id="s_tx">0/0</div></div>
      <div class="stat"><div class="k">Last latency</div><div class="v" id="s_lat">—</div></div>
      <div class="stat"><div class="k">HandsOn (real)</div><div class="v" id="s_ho">—</div></div>
      <div class="stat"><div class="k">Torque (real)</div><div class="v" id="s_tq">—</div></div>
      <div class="stat"><div class="k">Last injected</div><div class="v" id="s_inj">—</div></div>
      <div class="stat"><div class="k">CAN A state</div><div class="v" id="s_cs">—</div></div>
      <div class="stat full"><div class="k">Uptime</div><div class="v" id="s_up">—</div></div>
    </div>
    <div class="tbar">
      <button id="toggle" disabled>Disable</button>
    </div>
  </section>

  <section class="panel">
    <h2>Advanced — runtime overrides</h2>
    <div class="row row3">
      <label>Target CAN ID (hex)
        <input type="text" id="f_id" placeholder="0x370">
      </label>
      <label>AP state ID (hex)
        <input type="text" id="f_apId" placeholder="0x399">
      </label>
      <label>Steering ID (hex)
        <input type="text" id="f_stId" placeholder="0x129">
      </label>
    </div>
    <div class="row row3" style="margin-top:10px">
      <label>Burst (ms, mode B)
        <input type="number" id="f_burst" min="50" max="10000" step="50">
      </label>
      <label>Pause (ms, mode B)
        <input type="number" id="f_pause" min="0" max="10000" step="50">
      </label>
      <label>HandsOn=1 rate (%)
        <input type="number" id="f_ho" min="0" max="100" step="1">
      </label>
    </div>
    <details>
      <summary>Torque table (used by modes A/B)</summary>
      <table>
        <thead><tr><th>#</th><th>byte2 (hex)</th><th>byte3 (hex)</th><th>Nm</th></tr></thead>
        <tbody id="tq_tbody"></tbody>
      </table>
      <div class="tbar">
        <button id="tq_add">+ row</button>
        <button id="tq_del">− row</button>
      </div>
    </details>
    <div class="tbar" style="margin-top:14px">
      <button id="apply" class="primary" style="margin-left:auto" disabled>Apply all overrides</button>
    </div>
    <div class="desc">Hard cap: torque clamped to ±1.80 Nm in firmware.</div>
  </section>
</main>

<!-- SUMMON UNLOCK TAB -->
<main id="main-summon">
  <section class="panel">
    <h2>Summon Unlock <span class="iface-note">CAN B — TWAI</span></h2>
    <div class="stat full" style="min-height:auto;padding:16px 14px;">
      <div class="k">State</div>
      <div class="big-state off" id="sum_big">—</div>
    </div>
    <div class="tbar">
      <button class="primary" onclick="postSummon('/api/summon/enable')">Enable</button>
      <button class="danger" onclick="postSummon('/api/summon/disable')">Disable</button>
      <button class="warn" id="btnForceMode">AP injection</button>
    </div>
  </section>

  <section class="panel">
    <h2>Injection Gate</h2>
    <div class="gate-status" id="sum_gate_status">—</div>
    <div class="row" style="margin-top:12px">
      <div class="stat" id="sum_g_pk"><div class="k">Parked</div><div class="v" id="sum_g_pk_v">—</div></div>
      <div class="stat" id="sum_g_su"><div class="k">Summoning</div><div class="v" id="sum_g_su_v">—</div></div>
      <div class="stat"><div class="k">ACA</div><div class="v" id="sum_d_aca">—</div></div>
      <div class="stat"><div class="k">SPR</div><div class="v" id="sum_d_spr">—</div></div>
    </div>
    <div class="desc">
      Gate open if Parked OR Summoning only.<br>
      APActive : <span id="sum_g_ap_v" style="font-weight:600">—</span> (info only, doesn't start injection).
    </div>
  </section>

  <section class="panel">
    <h2>Frames CAN</h2>
    <div class="row">
      <div class="stat"><div class="k">280 (gear/ACA)</div><div class="v" id="sum_s_280">—</div></div>
      <div class="stat"><div class="k">390 (DIF gear)</div><div class="v" id="sum_s_390">—</div></div>
      <div class="stat"><div class="k">921 (AP status)</div><div class="v" id="sum_s_921">—</div></div>
      <div class="stat"><div class="k">1016 (SPR)</div><div class="v" id="sum_s_1016">—</div></div>
      <div class="stat"><div class="k">1021 mux1 rx</div><div class="v" id="sum_s_rx">—</div></div>
      <div class="stat"><div class="k">TX ok</div><div class="v ok" id="sum_s_ok">—</div></div>
      <div class="stat"><div class="k">TX fail</div><div class="v" id="sum_s_fail">—</div></div>
      <div class="stat"><div class="k">CAN B state</div><div class="v" id="sum_s_can">—</div></div>
      <div class="stat full"><div class="k">Uptime</div><div class="v" id="sum_s_up">—</div></div>
    </div>
  </section>
</main>

<!-- FIRMWARE / OTA TAB -->
<main id="main-fw">
  <section class="panel">
    <h2>Firmware / OTA Update</h2>
    <div class="row" style="margin-bottom:12px;">
      <div class="stat"><div class="k">Version</div><div class="v" id="fw_ver">—</div></div>
      <div class="stat"><div class="k">Free heap</div><div class="v" id="fw_free">—</div></div>
      <div class="stat"><div class="k">MCP2515 (CAN A)</div><div class="v" id="fw_mcp">—</div></div>
      <div class="stat"><div class="k">TWAI (CAN B)</div><div class="v" id="fw_twai">—</div></div>
      <div class="stat full"><div class="k">Boot count</div><div class="v" id="fw_boot">—</div></div>
    </div>
    <input type="file" id="otaFile" accept=".bin">
    <div class="tbar">
      <button class="primary" id="btnOtaUpload" onclick="uploadOta()">Upload &amp; Flash</button>
    </div>
    <div class="progress" id="otaProgressWrap">
      <div class="progress-bar" id="otaProgressBar"></div>
    </div>
    <div class="ota-msg" id="otaMsg">Select a compiled .bin firmware file, then upload. The device reboots automatically after a successful flash.</div>
  </section>
</main>

<div class="footer">
  T2CAN Unified ·
  <a href="/api/nag/config" target="_blank">/api/nag/config</a> ·
  <a href="/api/nag/stats" target="_blank">/api/nag/stats</a> ·
  <a href="/api/summon/stats" target="_blank">/api/summon/stats</a> ·
  <a href="/api/system/stats" target="_blank">/api/system/stats</a><br>
  research / educational only · not for use on public roads
</div>
<div class="toast" id="toast">saved</div>

<script>
const $ = id => document.getElementById(id);
let otaUploading = false;

function showTab(tab, btn) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('main').forEach(m => m.classList.remove('active'));
  btn.classList.add('active');
  $('main-' + tab).classList.add('active');
}

function showToast(msg) {
  const t = $('toast');
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(showToast._h);
  showToast._h = setTimeout(() => t.classList.remove('show'), 1500);
}

// ===== NAG ECHO JS =====
let nagCfg = null;
let nagIsLoading = false;

function nmFromBytes(b2, b3) {
  const raw = ((b2 & 0x0F) << 8) | (b3 & 0xFF);
  return (raw * 0.01 - 20.5);
}

function renderNagTorque() {
  if (!nagCfg || !nagCfg.torque) return;
  const tb = $('tq_tbody');
  tb.innerHTML = '';
  nagCfg.torque.forEach((t, i) => {
    const tr = document.createElement('tr');
    const b2Hex = '0x' + t.b2.toString(16).padStart(2,'0').toUpperCase();
    const b3Hex = '0x' + t.b3.toString(16).padStart(2,'0').toUpperCase();
    tr.innerHTML = `<td>${i}</td>
      <td><input type="text" data-i="${i}" data-k="b2" value="${b2Hex}"></td>
      <td><input type="text" data-i="${i}" data-k="b3" value="${b3Hex}"></td>
      <td id="nm_${i}">${nmFromBytes(t.b2,t.b3).toFixed(2)}</td>`;
    tb.appendChild(tr);
  });
  tb.querySelectorAll('input').forEach(inp => inp.addEventListener('input', e => {
    const i = +e.target.dataset.i, k = e.target.dataset.k;
    const v = parseInt(e.target.value, 16);
    if (Number.isFinite(v) && nagCfg && nagCfg.torque[i]) {
      nagCfg.torque[i][k] = v & 0xFF;
      $(`nm_${i}`).textContent = nmFromBytes(nagCfg.torque[i].b2, nagCfg.torque[i].b3).toFixed(2);
    }
  }));
}

function renderNagConfig() {
  if (!nagCfg) return;
  $('f_id').value    = '0x' + nagCfg.targetId.toString(16).toUpperCase().padStart(3,'0');
  $('f_apId').value  = '0x' + nagCfg.apStateId.toString(16).toUpperCase().padStart(3,'0');
  $('f_stId').value  = '0x' + nagCfg.steeringId.toString(16).toUpperCase().padStart(3,'0');
  $('f_ho').value    = nagCfg.hoRatePct;
  $('f_burst').value = nagCfg.burstMs;
  $('f_pause').value = nagCfg.pauseMs;
  $('lbl_burst').textContent = nagCfg.burstMs;
  $('lbl_pause').textContent = nagCfg.pauseMs;
  $('toggle').textContent = nagCfg.enabled ? 'Disable' : 'Enable';
  [['modeA',0],['modeB',1]].forEach(([id,m]) => { const el = $(id); if (el) el.classList.toggle('primary', nagCfg.mode === m); });
  renderNagTorque();
}

async function loadNagConfig() {
  try {
    const r = await fetch('/api/nag/config');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    nagCfg = await r.json();
    renderNagConfig();
    $('toggle').disabled = false;
    $('apply').disabled = false;
    $('modeA').disabled = false;
    $('modeB').disabled = false;
    $('modeR').disabled = false;
    $('tq_add').disabled = false;
    $('tq_del').disabled = false;
  } catch(e) {
    console.error('Failed to load nag config:', e);
    $('conn').textContent = 'nag cfg error';
    $('conn').className = 'pill bad';
    setTimeout(loadNagConfig, 2000);
  }
}

let nagLastOkMs = 0;
async function tickNagStats() {
  if (nagIsLoading) return;
  try {
    nagIsLoading = true;
    const ctrl = new AbortController();
    const to = setTimeout(() => ctrl.abort(), 1200);
    const r = await fetch('/api/nag/stats', { cache:'no-store', signal: ctrl.signal });
    clearTimeout(to);
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const s = await r.json();
    $('s_rx').textContent   = s.rx;
    $('s_echo').textContent = s.echo;
    $('s_tx').textContent   = s.txOk + ' / ' + s.txFail;
    $('s_lat').textContent  = s.latUs + ' µs';
    $('s_ho').textContent   = s.ho;
    $('s_tq').textContent   = (s.torque>=0?'+':'') + s.torque.toFixed(2) + ' Nm';
    $('s_inj').textContent  = (s.injNm>=0?'+':'') + s.injNm.toFixed(2) + ' Nm  ho=' + s.injHo;
    const cs = ['OK','running','bus-off','error'][s.canAState] || String(s.canAState);
    $('s_cs').textContent = cs;
    $('s_cs').className = 'v ' + (s.canAState===0?'ok':s.canAState===1?'ok':'bad');
    $('s_up').textContent   = s.uptimeS + ' s';
    $('conn').textContent  = 'connected';
    $('conn').className    = 'pill ok';
    $('s_en').textContent   = nagCfg && nagCfg.enabled ? 'YES' : 'NO';
    $('s_en').className     = 'v ' + (nagCfg && nagCfg.enabled ? 'ok' : 'warn');
    nagLastOkMs = Date.now();
  } catch(e) {
    if (Date.now() - nagLastOkMs > 3000) {
      $('conn').textContent = 'lost';
      $('conn').className   = 'pill bad';
    }
  } finally {
    nagIsLoading = false;
  }
}

async function setNagMode(m) {
  if (!nagCfg) { showToast('not ready'); return; }
  try {
    const r = await fetch('/api/nag/mode?m=' + m, { method: 'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    nagCfg = await r.json();
    renderNagConfig();
    showToast('mode applied');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

async function applyNagOverrides() {
  if (!nagCfg || !nagCfg.torque) { showToast('not ready'); return; }
  try {
    const id   = parseInt($('f_id').value, 16);
    const apId = parseInt($('f_apId').value, 16);
    const stId = parseInt($('f_stId').value, 16);
    const ho   = +$('f_ho').value;
    const burst= +$('f_burst').value;
    const pause= +$('f_pause').value;
    if (!Number.isFinite(id) || !Number.isFinite(apId) || !Number.isFinite(stId)) {
      showToast('invalid hex ID'); return;
    }
    const params = new URLSearchParams();
    params.set('targetId',  String(id));
    params.set('apStateId', String(apId));
    params.set('steeringId',String(stId));
    params.set('hoRatePct', String(ho));
    params.set('burstMs',   String(burst));
    params.set('pauseMs',   String(pause));
    params.set('count', String(nagCfg.torque.length));
    nagCfg.torque.forEach((t, i) => {
      params.set('b2_' + i, String(t.b2));
      params.set('b3_' + i, String(t.b3));
    });
    const r = await fetch('/api/nag/update?' + params, { method: 'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    nagCfg = await r.json();
    renderNagConfig();
    showToast('saved');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

$('modeA').onclick  = () => setNagMode(0);
$('modeB').onclick  = () => setNagMode(1);
$('modeR').onclick  = async () => {
  if (!nagCfg) { showToast('not ready'); return; }
  if (!confirm('Reset all settings to Mode A defaults?')) return;
  try {
    const r = await fetch('/api/nag/reset', { method:'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    nagCfg = await r.json();
    renderNagConfig();
    showToast('reset');
  } catch(e) {
    showToast('error: ' + e.message);
  }
};
$('apply').onclick  = applyNagOverrides;
$('toggle').onclick = () => {
  if (!nagCfg) { showToast('not ready'); return; }
  const params = new URLSearchParams({ enabled: nagCfg.enabled ? '0' : '1' });
  fetch('/api/nag/update?' + params, { method:'POST' })
    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); return r.json(); })
    .then(c => { nagCfg = c; renderNagConfig(); showToast(nagCfg.enabled ? 'enabled' : 'disabled'); })
    .catch(e => showToast('error: ' + e.message));
};
$('tq_add').onclick = () => {
  if (!nagCfg || !nagCfg.torque) return;
  if (nagCfg.torque.length >= 8) { showToast('max 8 entries'); return; }
  nagCfg.torque.push({ b2: 0x08, b3: 0xB6 });
  renderNagTorque();
};
$('tq_del').onclick = () => {
  if (!nagCfg || !nagCfg.torque) return;
  if (nagCfg.torque.length <= 1) { showToast('min 1 entry'); return; }
  nagCfg.torque.pop();
  renderNagTorque();
};

// ===== SUMMON UNLOCK JS =====
const SUMMON_CAN_STATES = ['running','recovering','bus-off','stopped'];

async function fetchSummonStats() {
  try {
    const s = await fetch('/api/summon/stats').then(r => r.json());
    const big = $('sum_big');
    if (s.forceMode) {
      big.textContent = 'FORCE';
      big.className   = 'big-state warn';
    } else {
      big.textContent = s.enabled ? 'ON' : 'OFF';
      big.className   = 'big-state ' + (s.enabled ? 'on' : 'off');
    }

    const btnForceMode = $('btnForceMode');
    if (btnForceMode) {
      btnForceMode.textContent = s.forceMode ? 'AP Injection: ON' : 'AP Injection: OFF';
      btnForceMode.style.opacity = s.forceMode ? '1' : '0.7';
    }

    const gate = s.gate;
    const gs = $('sum_gate_status');
    gs.textContent = gate ? 'OPEN — injection allowed' : 'CLOSED — injection blocked';
    gs.className   = 'gate-status ' + (gate ? 'open' : 'closed');

    $('sum_g_ap_v').textContent = s.ap ? 'ON' : 'OFF';
    $('sum_g_ap_v').style.color = s.ap ? 'var(--ok)' : 'var(--muted)';
    $('sum_g_pk').classList.toggle('active', !!s.parked);
    $('sum_g_pk_v').textContent = s.parked ? 'ON' : 'OFF';
    $('sum_g_su').classList.toggle('active', !!s.summon);
    $('sum_g_su_v').textContent = s.summon ? 'ON' : 'OFF';

    $('sum_d_aca').textContent = s.aca ? 'ACTIVE' : 'inactive';
    $('sum_d_aca').className = 'v ' + (s.aca ? 'ok' : '');
    $('sum_d_spr').textContent = s.spr ? 'SEEN' : 'not seen';
    $('sum_d_spr').className = 'v ' + (s.spr ? 'ok' : '');

    $('sum_s_280').textContent  = s.rx280;
    $('sum_s_390').textContent  = s.rx390;
    $('sum_s_921').textContent  = s.rx921;
    $('sum_s_1016').textContent = s.rx1016;
    $('sum_s_rx').textContent   = s.rxMux1;
    $('sum_s_ok').textContent   = s.txOk;
    $('sum_s_fail').textContent = s.txFail;
    $('sum_s_fail').className   = 'v ' + (s.txFail > 0 ? 'warn' : '');

    const cs = SUMMON_CAN_STATES[s.canState] ?? String(s.canState);
    $('sum_s_can').textContent = cs;
    $('sum_s_can').className   = 'v ' + (s.canState === 0 ? 'ok' : s.canState === 2 ? 'bad' : 'warn');

    const u = s.uptimeS;
    $('sum_s_up').textContent = u < 60 ? u + 's' : Math.floor(u/60) + 'm' + (u%60) + 's';

    $('conn').textContent = 'connected';
    $('conn').className   = 'pill ok';
  } catch {
    $('conn').textContent = 'lost';
    $('conn').className   = 'pill bad';
  }
}

async function postSummon(url) {
  await fetch(url, { method: 'POST' });
  fetchSummonStats();
}

// ===== FIRMWARE / SYSTEM JS =====
async function fetchSystemStats() {
  try {
    const s = await fetch('/api/system/stats').then(r => r.json());
    if (s.fwVersion) {
      $('fw_ver').textContent = s.fwVersion;
      $('hdr_ver').textContent = s.fwVersion;
    }
    if (s.freeHeap !== undefined) $('fw_free').textContent = Math.round(s.freeHeap/1024) + ' KB';
    $('fw_mcp').textContent  = s.mcpReady  ? 'ready' : 'not ready';
    $('fw_mcp').className    = 'v ' + (s.mcpReady  ? 'ok' : 'bad');
    $('fw_twai').textContent = s.twaiReady ? 'ready' : 'not ready';
    $('fw_twai').className   = 'v ' + (s.twaiReady ? 'ok' : 'bad');
    if (s.rtcBootCount !== undefined) $('fw_boot').textContent = s.rtcBootCount;
  } catch(e) {
    // keep last known values
  }
}

// ── OTA upload ───────────────────────────────────────────────
function uploadOta() {
  const input = $('otaFile');
  const file = input.files[0];
  const msg = $('otaMsg');
  const wrap = $('otaProgressWrap');
  const bar = $('otaProgressBar');
  const btn = $('btnOtaUpload');

  if (!file) {
    msg.textContent = 'Please choose a .bin file first.';
    msg.className = 'ota-msg bad';
    return;
  }

  const form = new FormData();
  form.append('update', file, file.name);

  otaUploading = true;
  btn.disabled = true;
  input.disabled = true;
  wrap.className = 'progress show';
  bar.style.width = '0%';
  msg.textContent = 'Uploading ' + file.name + '…';
  msg.className = 'ota-msg';

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      bar.style.width = pct + '%';
      msg.textContent = 'Uploading… ' + pct + '%';
    }
  };

  xhr.onload = () => {
    let ok = xhr.status === 200;
    let errText = '';
    try {
      const r = JSON.parse(xhr.responseText);
      ok = ok && r.ok;
      errText = r.error || '';
    } catch {}

    if (ok) {
      bar.style.width = '100%';
      msg.textContent = 'Flash successful — rebooting…';
      msg.className = 'ota-msg ok';
      setTimeout(() => location.reload(), 6000);
    } else {
      msg.textContent = 'OTA failed' + (errText ? ': ' + errText : '');
      msg.className = 'ota-msg bad';
      btn.disabled = false;
      input.disabled = false;
      otaUploading = false;
    }
  };

  xhr.onerror = () => {
    msg.textContent = 'Upload error — device likely rebooted or connection lost.';
    msg.className = 'ota-msg bad';
    btn.disabled = false;
    input.disabled = false;
    otaUploading = false;
  };

  xhr.send(form);
}

// ===== STARTUP =====
loadNagConfig().then(() => { tickNagStats(); setInterval(() => { if (!otaUploading) tickNagStats(); }, 500); });
fetchSummonStats();
setInterval(() => { if (!otaUploading) fetchSummonStats(); }, 800);
fetchSystemStats();
setInterval(() => { if (!otaUploading) fetchSystemStats(); }, 3000);
</script>
</body>
</html>
)HTML";
