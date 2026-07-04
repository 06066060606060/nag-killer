const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>nag_echo dashboard</title>
<style>
  :root {
    --bg:#0d1117; --panel:#161b22; --line:#30363d; --txt:#c9d1d9;
    --muted:#8b949e; --accent:#58a6ff; --ok:#3fb950; --warn:#d29922; --bad:#f85149;
  }
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--txt);font:14px/1.45 ui-monospace,Consolas,Menlo,monospace}
  header{padding:14px 18px;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;align-items:center}
  header h1{margin:0;font-size:16px;font-weight:600;color:var(--accent)}
  header .pill{font-size:11px;padding:3px 8px;border:1px solid var(--line);border-radius:10px;color:var(--muted)}
  main{max-width:820px;margin:0 auto;padding:16px;display:grid;gap:14px}
  .panel{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px}
  .panel h2{margin:0 0 10px;font-size:12px;font-weight:600;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
  .row{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px}
  .stat{background:#0d1117;border:1px solid var(--line);border-radius:6px;padding:8px 10px}
  .stat .k{font-size:10px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
  .stat .v{font-size:18px;font-weight:600;margin-top:2px}
  .modes{display:flex;gap:8px;margin-bottom:10px;flex-wrap:wrap}
  button{font:inherit;cursor:pointer;background:#21262d;color:var(--txt);border:1px solid var(--line);border-radius:6px;padding:8px 14px}
  button.primary{background:var(--accent);color:#0d1117;border-color:transparent;font-weight:600}
  button.danger{background:#21262d;color:var(--bad);border-color:#3a1f23}
  button:hover{filter:brightness(1.15)}
  button[disabled]{opacity:.5;cursor:not-allowed}
  label{display:flex;flex-direction:column;gap:4px;color:var(--muted);font-size:12px}
  input[type=text],input[type=number]{background:#0d1117;border:1px solid var(--line);color:var(--txt);border-radius:4px;padding:5px 8px;font:inherit;width:100%}
  input[type=range]{width:100%}
  table{width:100%;border-collapse:collapse;margin-top:6px}
  th,td{text-align:left;padding:5px 6px;border-bottom:1px solid var(--line);font-size:12px}
  th{color:var(--muted);font-weight:600;text-transform:uppercase;letter-spacing:.05em;font-size:10px}
  td input{width:90%}
  .tbar{display:flex;gap:6px;margin-top:8px;flex-wrap:wrap}
  .grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  .grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
  .ok{color:var(--ok)} .warn{color:var(--warn)} .bad{color:var(--bad)}
  .footer{color:var(--muted);font-size:11px;text-align:center;padding:14px 0}
  .footer a{color:var(--muted)}
  .toast{position:fixed;bottom:14px;left:50%;transform:translateX(-50%);background:var(--accent);color:#0d1117;padding:6px 14px;border-radius:14px;font-size:12px;font-weight:600;opacity:0;transition:opacity .25s}
  .toast.show{opacity:1}
  details{margin-top:10px}
  summary{cursor:pointer;color:var(--muted);font-size:12px}
</style>
</head>
<body>
<header>
  <h1>nag_echo v2</h1>
  <span class="pill" id="state">connecting&hellip;</span>
</header>
<main>

  <section class="panel">
    <h2>mode</h2>
    <div class="modes">
      <button id="modeA" class="primary">A &mdash; Simple</button>
      <button id="modeB">B &mdash; TSL6P (burst/pause)</button>
      <button id="modeC">C &mdash; State machine</button>
      <button id="modeR" class="danger" style="margin-left:auto">Reset</button>
    </div>
    <div style="font-size:11px;color:var(--muted);line-height:1.55">
      <b>A</b>: CAN 0x370, fixed +1.80&nbsp;Nm, handsOn=1 always. Proven on MY 2022 HW3 pre-Juniper.<br>
      <b>B</b>: CAN 0x052, 4-value torque cycle, time-bursty (<span id="lbl_burst">1000</span> ms inject / <span id="lbl_pause">1500</span> ms rest). Closer to the actual TSL6P device behaviour from sniff logs.<br>
      <b>C</b>: <a href="#" style="color:var(--muted)">@Linu</a>'s state machine on DAS_autopilotHandsOnState. Refuses to inject if context CAN frames are stale &gt;1&nbsp;s.
    </div>
  </section>

  <section class="panel">
    <h2>live</h2>
    <div class="row">
      <div class="stat"><div class="k">enabled</div><div class="v" id="s_en">&mdash;</div></div>
      <div class="stat"><div class="k">rx frames</div><div class="v" id="s_rx">0</div></div>
      <div class="stat"><div class="k">echo sent</div><div class="v" id="s_echo">0</div></div>
      <div class="stat"><div class="k">tx ok / fail</div><div class="v" id="s_tx">0/0</div></div>
      <div class="stat"><div class="k">last latency</div><div class="v" id="s_lat">&mdash;</div></div>
      <div class="stat"><div class="k">handsOn (real)</div><div class="v" id="s_ho">&mdash;</div></div>
      <div class="stat"><div class="k">torque (real)</div><div class="v" id="s_tq">&mdash;</div></div>
      <div class="stat"><div class="k">last injected</div><div class="v" id="s_inj">&mdash;</div></div>
      <div class="stat"><div class="k">can state</div><div class="v" id="s_cs">&mdash;</div></div>
      <div class="stat"><div class="k">uptime</div><div class="v" id="s_up">&mdash;</div></div>
    </div>
    <div class="tbar">
      <button id="toggle" disabled>Disable</button>
    </div>
  </section>

  <section class="panel" id="modeC_panel">
    <h2>mode C context (DAS state)</h2>
    <div class="row">
      <div class="stat"><div class="k">apState</div><div class="v" id="c_ap">&mdash;</div></div>
      <div class="stat"><div class="k">handsOnState</div><div class="v" id="c_ho">&mdash;</div></div>
      <div class="stat"><div class="k">steering deg</div><div class="v" id="c_st">&mdash;</div></div>
      <div class="stat"><div class="k">apState fresh</div><div class="v" id="c_apf">&mdash;</div></div>
      <div class="stat"><div class="k">steering fresh</div><div class="v" id="c_stf">&mdash;</div></div>
    </div>
    <div style="font-size:11px;color:var(--muted);margin-top:8px">
      Mode C will only inject when both freshness indicators are green
      (last frame received within 1&nbsp;s). If your car uses CAN IDs
      different from the defaults below, edit them here. The Mode C
      gating logic refuses to inject under unknown context — safer than
      guessing.
    </div>
    <div class="grid2" style="margin-top:10px">
      <label>autopilotState CAN ID
        <input type="text" id="f_apId" placeholder="0x399">
      </label>
      <label>steeringAngle CAN ID
        <input type="text" id="f_stId" placeholder="0x129">
      </label>
    </div>
  </section>

  <section class="panel">
    <h2>advanced &mdash; runtime overrides</h2>
    <div class="grid3">
      <label>target CAN ID (hex)
        <input type="text" id="f_id" placeholder="0x370">
      </label>
      <label>burst (ms, mode B)
        <input type="number" id="f_burst" min="50" max="10000" step="50">
      </label>
      <label>pause (ms, mode B)
        <input type="number" id="f_pause" min="0" max="10000" step="50">
      </label>
    </div>
    <details>
      <summary>torque table (used by modes A/B/Custom &mdash; ignored by mode C)</summary>
      <table>
        <thead><tr><th>#</th><th>byte2 (hex)</th><th>byte3 (hex)</th><th>Nm</th></tr></thead>
        <tbody id="tq_tbody"></tbody>
      </table>
      <div class="tbar">
        <button id="tq_add">+ row</button>
        <button id="tq_del">&minus; row</button>
        <label style="flex-direction:row;align-items:center;margin-left:auto">handsOn=1 rate (%)
          <input type="number" id="f_ho" min="0" max="100" step="1" style="width:70px;margin-left:8px">
        </label>
      </div>
    </details>
    <div class="tbar" style="margin-top:14px">
      <button id="apply" class="primary" style="margin-left:auto" disabled>Apply all overrides</button>
    </div>
    <div style="font-size:11px;color:var(--muted);margin-top:8px">
      Hard cap: torque clamped to &plusmn;1.80&nbsp;Nm in firmware.
      Higher values are silently snapped in.
    </div>
  </section>

  <div class="footer">
    nag_echo v2 &middot;
    <a href="/api/config" target="_blank">/api/config</a> &middot;
    <a href="/api/stats" target="_blank">/api/stats</a><br>
    research / educational only &middot; not for use on public roads
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
  $('f_apId').value  = '0x' + cfg.apStateId.toString(16).toUpperCase().padStart(3,'0');
  $('f_stId').value  = '0x' + cfg.steeringId.toString(16).toUpperCase().padStart(3,'0');
  $('f_ho').value    = cfg.hoRatePct;
  $('f_burst').value = cfg.burstMs;
  $('f_pause').value = cfg.pauseMs;
  $('lbl_burst').textContent = cfg.burstMs;
  $('lbl_pause').textContent = cfg.pauseMs;
  $('toggle').textContent = cfg.enabled ? 'Disable' : 'Enable';
  
  ['modeA','modeB','modeC'].forEach((id,m) => $(id).classList.toggle('primary', cfg.mode === m));
  $('modeC_panel').style.opacity = (cfg.mode === 2) ? '1' : '0.55';
  
  renderTorque();
}

async function loadConfig() {
  try {
    const r = await fetch('/api/config');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    // Enable buttons now that config is loaded
    $('toggle').disabled = false;
    $('apply').disabled = false;
    $('modeA').disabled = false;
    $('modeB').disabled = false;
    $('modeC').disabled = false;
    $('modeR').disabled = false;
    $('tq_add').disabled = false;
    $('tq_del').disabled = false;
  } catch(e) {
    console.error('Failed to load config:', e);
    $('state').textContent = 'config error';
    $('state').className = 'pill bad';
    // Retry after 2 seconds
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
  if (isLoading) return; // Prevent overlapping requests
  
  try {
    isLoading = true;
    const r = await fetch('/api/stats');
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const s = await r.json();
    
    $('s_rx').textContent   = s.rx;
    $('s_echo').textContent = s.echo;
    $('s_tx').textContent   = s.txOk + ' / ' + s.txFail;
    $('s_lat').textContent  = s.latUs + ' µs';
    $('s_ho').textContent   = s.ho;
    $('s_tq').textContent   = (s.torque>=0?'+':'') + s.torque.toFixed(2) + ' Nm';
    $('s_inj').textContent  = (s.injNm>=0?'+':'') + s.injNm.toFixed(2) + ' Nm  ho=' + s.injHo;
    
    const cs = ['running','recovering','off','stopped'][s.canState] || s.canState;
    $('s_cs').textContent   = cs;
    $('s_cs').className = 'v ' + (s.canState===0?'ok':s.canState===2?'bad':'warn');
    $('s_up').textContent   = s.uptimeS + ' s';
    $('state').textContent  = 'connected';
    $('state').className    = 'pill ok';
    $('s_en').textContent   = cfg && cfg.enabled ? 'YES' : 'NO';
    $('s_en').className     = 'v ' + (cfg && cfg.enabled ? 'ok' : 'warn');
    
    // Mode C panel
    $('c_ap').textContent  = s.apState;
    $('c_ho').textContent  = s.handsOnState;
    $('c_st').textContent  = s.steeringDeg + ' °';
    let [t1,c1] = freshTag(s.apStaleMs);
    let [t2,c2] = freshTag(s.stStaleMs);
    $('c_apf').textContent = t1; $('c_apf').className = 'v ' + c1;
    $('c_stf').textContent = t2; $('c_stf').className = 'v ' + c2;
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
    const apId = parseInt($('f_apId').value, 16);
    const stId = parseInt($('f_stId').value, 16);
    const ho   = +$('f_ho').value;
    const burst= +$('f_burst').value;
    const pause= +$('f_pause').value;
    
    // Validate inputs
    if (!Number.isFinite(id) || !Number.isFinite(apId) || !Number.isFinite(stId)) {
      showToast('invalid hex ID');
      return;
    }
    
    const params = new URLSearchParams();
    params.set('targetId',  String(id));
    params.set('apStateId', String(apId));
    params.set('steeringId',String(stId));
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

// Initialize button states (disabled until config loads)
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

// Start
loadConfig().then(() => { 
  tickStats(); 
  setInterval(tickStats, 500); 
});
</script>
</body>
</html>
)HTML";