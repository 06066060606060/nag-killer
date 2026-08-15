const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>nag_echo v2</title>
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
  main {
    max-width: 420px;
    margin: 0 auto;
    padding: 0 16px 24px;
    display: grid;
    gap: 16px;
  }
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
  .row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
  }
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
  }
  .stat .v {
    font-size: 18px;
    font-weight: 700;
    margin-top: 6px;
    color: var(--txt);
    letter-spacing: -0.02em;
  }
  .stat.full { grid-column: 1 / -1; }
  .modes {
    display: flex;
    gap: 10px;
    margin-bottom: 14px;
    flex-wrap: wrap;
  }
  button {
    font: inherit;
    cursor: pointer;
    background: var(--card);
    color: var(--txt);
    border: 1px solid var(--line);
    border-radius: 12px;
    padding: 10px 16px;
    font-size: 13px;
    font-weight: 600;
    transition: all 0.15s ease;
  }
  button:hover { filter: brightness(1.2); }
  button:active { transform: scale(0.97); }
  button.primary {
    background: var(--txt);
    color: var(--bg);
    border-color: transparent;
  }
  button.danger {
    background: var(--card);
    color: var(--bad);
    border-color: #3a1f23;
    margin-left: auto;
  }
  button[disabled] { opacity: 0.4; cursor: not-allowed; }
  .desc {
    font-size: 12px;
    color: var(--muted);
    line-height: 1.6;
  }
  .desc b { color: var(--txt); font-weight: 600; }
  label {
    display: flex;
    flex-direction: column;
    gap: 5px;
    color: var(--muted);
    font-size: 12px;
    font-weight: 500;
  }
  input[type="text"], input[type="number"] {
    background: var(--card);
    border: 1px solid var(--line);
    color: var(--txt);
    border-radius: 10px;
    padding: 8px 10px;
    font: inherit;
    font-size: 13px;
    width: 100%;
    outline: none;
    transition: border-color 0.15s;
  }
  input[type="text"]:focus, input[type="number"]:focus {
    border-color: var(--accent);
  }
  table {
    width: 100%;
    border-collapse: collapse;
    margin-top: 8px;
  }
  th, td {
    text-align: left;
    padding: 6px 8px;
    border-bottom: 1px solid var(--line);
    font-size: 12px;
  }
  th {
    color: var(--muted);
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    font-size: 10px;
  }
  td input { width: 90%; }
  .tbar {
    display: flex;
    gap: 8px;
    margin-top: 10px;
    flex-wrap: wrap;
    align-items: center;
  }
  .grid3 {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    gap: 10px;
  }
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
  .toast {
    position: fixed;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%);
    background: var(--txt);
    color: var(--bg);
    padding: 10px 20px;
    border-radius: 20px;
    font-size: 13px;
    font-weight: 700;
    opacity: 0;
    transition: opacity 0.25s;
    pointer-events: none;
    z-index: 100;
  }
  .toast.show { opacity: 1; }
  details { margin-top: 12px; }
  summary {
    cursor: pointer;
    color: var(--muted);
    font-size: 13px;
    font-weight: 600;
    list-style: none;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  summary::after {
    content: "›";
    font-size: 16px;
    transition: transform 0.2s;
    margin-left: auto;
  }
  details[open] summary::after { transform: rotate(90deg); }
  .override-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    cursor: pointer;
  }
  .override-header h2 { margin: 0; }
  .override-header::after {
    content: "›";
    font-size: 18px;
    color: var(--muted);
    transition: transform 0.2s;
  }
  .override-header.open::after { transform: rotate(90deg); }
  .override-body {
    display: none;
    margin-top: 14px;
  }
  .override-body.open { display: block; }
</style>
</head>
<body>
<header>
  <h1>Nag Killer</h1>
  <span class="pill" id="state">connecting…</span>
</header>
<main>

  <section class="panel">
    <h2>Live</h2>
    <div class="row">
      <div class="stat"><div class="k">RX Frames</div><div class="v" id="s_rx">0</div></div>
      <div class="stat"><div class="k">Echo Sent</div><div class="v" id="s_echo">0</div></div>
      <div class="stat"><div class="k">TX OK / Fail</div><div class="v" id="s_tx">0/0</div></div>
      <div class="stat"><div class="k">Last Latency</div><div class="v" id="s_lat">—</div></div>
      <div class="stat"><div class="k">Torque (real)</div><div class="v" id="s_tq">—</div></div>
      <div class="stat"><div class="k">HandsOn (real)</div><div class="v" id="s_ho">—</div></div>
      <div class="stat"><div class="k">Last Injected</div><div class="v" id="s_inj">—</div></div>
      <div class="stat"><div class="k">CAN State</div><div class="v" id="s_cs">—</div></div>
      <div class="stat full"><div class="k">Uptime</div><div class="v" id="s_up">—</div></div>
    </div>
  </section>

  <section class="panel">
    <h2>Mode</h2>
    <div class="modes">
      <button id="modeA" class="primary">A - Simple</button>
      <button id="modeB">B - TSL6P burst/pause</button>
      <button id="modeC">C - Random walk burst/pause</button>
      <button id="modeR" class="danger">Reset</button>
    </div>
    <div class="desc">
      <b>A</b>: CAN 0x370, fixed +1.80 Nm, handsOn=1 always. Simple, works on HW3 v12.<br>
      <b>B</b>: Configurable target CAN ID, 8-value torque cycle, burst injection for <span id="lbl_burst">1000</span> ms followed by a <span id="lbl_pause">1500</span> ms pause. Emulates TSL6P.<br>
      <b>C</b>: Random walk between +1.48 and +1.78 Nm, Configurable CAN ID, burst injection for <span id="lbl_burst2">1000</span> ms followed by a <span id="lbl_pause2">1500</span> ms pause. Random Nm to obscure usage.
    </div>
  </section>

  <section class="panel">
    <div class="override-header" id="ovToggle">
      <h2>Runtime Overrides</h2>
    </div>
    <div class="override-body" id="ovBody">
      <div class="grid3">
        <label>Target CAN ID (hex)
          <input type="text" id="f_id" placeholder="0x370">
        </label>
        <label>Burst (ms, mode B/C)
          <input type="number" id="f_burst" min="50" max="10000" step="50">
        </label>
        <label>Pause (ms, mode B/C)
          <input type="number" id="f_pause" min="0" max="10000" step="50">
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
          <label style="flex-direction:row;align-items:center;margin-left:auto">handsOn=1 rate (%)
            <input type="number" id="f_ho" min="0" max="100" step="1" style="width:70px;margin-left:8px">
          </label>
        </div>
      </details>
      <div class="tbar" style="margin-top:14px">
        <button id="toggle" disabled style="margin-right:auto">Disable</button>
        <button id="apply" class="primary" disabled>Apply all overrides</button>
      </div>
      <div class="desc" style="margin-top:8px">
        Hard cap: torque clamped to ±1.80 Nm in firmware. Higher values are silently snapped in.
      </div>
    </div>
  </section>

  <div class="footer">
    nag_echo v2 · <a href="/api/config" target="_blank">/api/config</a> · <a href="/api/stats" target="_blank">/api/stats</a><br>
    research / educational only · not for use on public roads
  </div>
</main>

<div class="toast" id="toast">saved</div>

<script>
const $ = id => document.getElementById(id);
let cfg = null;
let isLoading = false;

function showToast(msg) {
  const t = $('toast');
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(showToast._h);
  showToast._h = setTimeout(() => t.classList.remove('show'), 1500);
}

function nmFromBytes(b2, b3) {
  const raw = ((b2 & 0x0F) << 8) | (b3 & 0xFF);
  return (raw * 0.01 - 20.5);
}

function renderTorque() {
  if (!cfg || !cfg.torque) return;
  const tb = $('tq_tbody');
  tb.innerHTML = '';
  cfg.torque.forEach((t, i) => {
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
    if (Number.isFinite(v) && cfg && cfg.torque[i]) {
      cfg.torque[i][k] = v & 0xFF;
      $(`nm_${i}`).textContent = nmFromBytes(cfg.torque[i].b2, cfg.torque[i].b3).toFixed(2);
    }
  }));
}

function renderConfig() {
  if (!cfg) return;
  $('f_id').value    = '0x' + cfg.targetId.toString(16).toUpperCase().padStart(3,'0');
  $('f_ho').value    = cfg.hoRatePct;
  $('f_burst').value = cfg.burstMs;
  $('f_pause').value = cfg.pauseMs;
  $('lbl_burst').textContent = cfg.burstMs;
  $('lbl_burst2').textContent = cfg.burstMs;
  $('lbl_pause').textContent = cfg.pauseMs;
  $('lbl_pause2').textContent = cfg.pauseMs;
  $('toggle').textContent = cfg.enabled ? 'Disable' : 'Enable';
  ['modeA','modeB','modeC'].forEach((id,m) => $(id).classList.toggle('primary', Number(cfg.mode) === m));
  renderTorque();
}

async function loadConfig() {
  try {
    const r = await fetch('/api/config');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    $('toggle').disabled = false;
    $('apply').disabled = false;
    $('modeA').disabled = false;
    $('modeB').disabled = false;
    $('modeC').disabled = false;
    $('modeR').disabled = false;
    $('tq_add').disabled = false;
    $('tq_del').disabled = false;
  } catch(e) {
    $('state').textContent = 'config error';
    $('state').className = 'pill bad';
    setTimeout(loadConfig, 2000);
  }
}

function freshTag(ms) {
  if (ms == null || ms === 999999) return ['stale', 'bad'];
  if (ms > 5000) return [(ms/1000).toFixed(1)+' s ago', 'bad'];
  if (ms > 1000) return [ms+' ms ago', 'warn'];
  return [ms+' ms ago', 'ok'];
}

async function tickStats() {
  if (isLoading) return;
  try {
    isLoading = true;
    const r = await fetch('/api/stats');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const s = await r.json();
    $('s_rx').textContent   = s.rx;
    $('s_echo').textContent = s.echo;
    $('s_tx').textContent   = s.txOk + ' / ' + s.txFail;
    $('s_lat').textContent  = s.latUs ? (s.latUs + ' µs') : '—';
    $('s_ho').textContent   = s.ho != null ? s.ho : '—';
    $('s_tq').textContent   = s.torque != null ? ((s.torque>=0?'+':'') + s.torque.toFixed(2) + ' Nm') : '—';
    $('s_inj').innerHTML    = s.injNm != null ? ((s.injNm>=0?'+':'') + s.injNm.toFixed(2) + ' Nm<br>ho=' + s.injHo) : '—';
    const cs = ['stopped','running','bus-off','recovering'][s.canState] || s.canState;
    $('s_cs').textContent = cs;
    $('s_cs').className = 'v ' + (s.canState===1?'ok':s.canState===2?'bad':'warn');
    $('s_up').textContent   = s.uptimeS ? (s.uptimeS + ' s') : '—';
    $('state').textContent  = 'connected';
    $('state').className    = 'pill ok';
  } catch(e) {
    $('state').textContent = 'lost';
    $('state').className   = 'pill bad';
  } finally {
    isLoading = false;
  }
}

async function setMode(m) {
  if (!cfg) { showToast('not ready'); return; }
  try {
    const r = await fetch('/api/mode?m=' + m, { method: 'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    showToast('mode applied');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

async function applyOverrides() {
  if (!cfg || !cfg.torque) { showToast('not ready'); return; }
  try {
    const id   = parseInt($('f_id').value, 16);
    const ho   = +$('f_ho').value;
    const burst= +$('f_burst').value;
    const pause= +$('f_pause').value;
    if (!Number.isFinite(id)) { showToast('invalid hex ID'); return; }
    const params = new URLSearchParams();
    params.set('targetId',  String(id));
    params.set('hoRatePct', String(ho));
    params.set('burstMs',   String(burst));
    params.set('pauseMs',   String(pause));
    params.set('count', String(cfg.torque.length));
    cfg.torque.forEach((t, i) => {
      params.set('b2_' + i, String(t.b2));
      params.set('b3_' + i, String(t.b3));
    });
    const r = await fetch('/api/update?' + params, { method: 'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    showToast('saved');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

$('modeA').disabled = true;
$('modeB').disabled = true;
$('modeC').disabled = true;
$('modeR').disabled = true;
$('tq_add').disabled = true;
$('tq_del').disabled = true;

$('modeA').onclick  = () => setMode(0);
$('modeB').onclick  = () => setMode(1);
$('modeC').onclick  = () => setMode(2);
$('modeR').onclick  = async () => {
  if (!cfg) { showToast('not ready'); return; }
  if (!confirm('Reset all settings to Mode A defaults?')) return;
  try {
    const r = await fetch('/api/reset', { method:'POST' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    showToast('reset');
  } catch(e) {
    showToast('error: ' + e.message);
  }
};

$('apply').onclick  = applyOverrides;
$('toggle').onclick = () => {
  if (!cfg) { showToast('not ready'); return; }
  const params = new URLSearchParams({ enabled: cfg.enabled ? '0' : '1' });
  fetch('/api/update?' + params, { method:'POST' })
    .then(r => { if (!r.ok) throw new Error('HTTP ' + r.status); return r.json(); })
    .then(c => { cfg = c; renderConfig(); showToast(cfg.enabled ? 'enabled' : 'disabled'); })
    .catch(e => showToast('error: ' + e.message));
};

$('tq_add').onclick = () => {
  if (!cfg || !cfg.torque) return;
  if (cfg.torque.length >= 8) { showToast('max 8 entries'); return; }
  cfg.torque.push({ b2: 0x08, b3: 0xB6 });
  renderTorque();
};

$('tq_del').onclick = () => {
  if (!cfg || !cfg.torque) return;
  if (cfg.torque.length <= 1) { showToast('min 1 entry'); return; }
  cfg.torque.pop();
  renderTorque();
};

$('ovToggle').onclick = () => {
  $('ovBody').classList.toggle('open');
  $('ovToggle').classList.toggle('open');
};

loadConfig().then(() => {
  tickStats();
  setInterval(tickStats, 500);
});
</script>
</body>
</html>
)HTML";
