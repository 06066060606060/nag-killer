const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>T2CAN Unified</title>
<style>
  :root {
    --bg:#0d1117; --panel:#161b22; --line:#30363d; --txt:#c9d1d9;
    --muted:#8b949e; --accent:#58a6ff; --ok:#3fb950; --warn:#d29922; --bad:#f85149;
  }
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--txt);font:13px/1.5 ui-monospace,Consolas,Menlo,monospace}
  header{padding:12px 16px;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;align-items:center}
  header h1{margin:0;font-size:15px;font-weight:700;color:var(--accent)}
  .pill{font-size:11px;padding:2px 8px;border:1px solid var(--line);border-radius:10px;color:var(--muted)}
  .tabs{display:flex;gap:8px;padding:14px 14px 0;max-width:900px;margin:0 auto}
  .tab-btn{background:var(--panel);border:1px solid var(--line);color:var(--muted);padding:8px 16px;border-radius:6px;cursor:pointer;font:inherit;font-weight:600}
  .tab-btn.active{background:var(--accent);color:#0d1117;border-color:transparent}
  main{max-width:900px;margin:0 auto;padding:14px;display:none;gap:14px}
  main.active{display:grid}
  .panel{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px}
  .panel h2{margin:0 0 10px;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:.08em;color:var(--muted)}
  .row{display:grid;grid-template-columns:repeat(auto-fit,minmax(100px,1fr));gap:8px}
  .stat{background:#0d1117;border:1px solid var(--line);border-radius:6px;padding:7px 10px}
  .stat .k{font-size:10px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
  .stat .v{font-size:17px;font-weight:600;margin-top:2px}
  button{font:inherit;cursor:pointer;border-radius:6px;padding:8px 14px;font-size:13px;font-weight:600;border:1px solid var(--line);background:#21262d;color:var(--txt)}
  button.primary{background:var(--accent);color:#0d1117;border-color:transparent}
  button.danger{color:var(--bad);border-color:#3a1f23}
  button:hover{filter:brightness(1.15)}
  button[disabled]{opacity:.5;cursor:not-allowed}
  .tbar{display:flex;gap:6px;margin-top:8px;flex-wrap:wrap}
  .ok{color:var(--ok)} .warn{color:var(--warn)} .bad{color:var(--bad)}
  .footer{color:var(--muted);font-size:11px;text-align:center;padding:14px 0}
  .footer a{color:var(--muted)}
  .toast{position:fixed;bottom:14px;left:50%;transform:translateX(-50%);background:var(--accent);color:#0d1117;padding:6px 14px;border-radius:14px;font-size:12px;font-weight:600;opacity:0;transition:opacity .25s}
  .toast.show{opacity:1}
  .modes{display:flex;gap:8px;margin-bottom:10px;flex-wrap:wrap}
  label{display:flex;flex-direction:column;gap:4px;color:var(--muted);font-size:12px}
  input[type=text],input[type=number]{background:#0d1117;border:1px solid var(--line);color:var(--txt);border-radius:4px;padding:5px 8px;font:inherit;width:100%}
  input[type=range]{width:100%}
  table{width:100%;border-collapse:collapse;margin-top:6px}
  th,td{text-align:left;padding:5px 6px;border-bottom:1px solid var(--line);font-size:12px}
  th{color:var(--muted);font-weight:600;text-transform:uppercase;letter-spacing:.05em;font-size:10px}
  td input{width:90%}
  .grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  .grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
  details{margin-top:10px}
  summary{cursor:pointer;color:var(--muted);font-size:12px}
  .big{font-size:52px;font-weight:700;text-align:center;padding:16px 0;border-radius:8px;border:2px solid var(--line);letter-spacing:2px;transition:color .2s,border-color .2s}
  .big.on {color:var(--ok);border-color:var(--ok)}
  .big.off{color:var(--bad);border-color:var(--bad)}
  .gate{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px}
  .gbox{border:1px solid var(--line);border-radius:6px;padding:8px 10px;text-align:center}
  .gbox .k{font-size:10px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
  .gbox .v{font-size:15px;font-weight:700;margin-top:3px}
  .gbox.active{border-color:var(--ok);background:#0d2a15}
  .gbox.inactive{border-color:var(--line)}
  .gate-status{text-align:center;font-size:13px;font-weight:600;padding:8px;border-radius:6px;border:1px solid var(--line)}
  .gate-status.open{color:var(--ok);border-color:var(--ok);background:#0d2a15}
  .gate-status.closed{color:var(--bad);border-color:#3a1f23;background:#2a0d0d}
  .disc{display:grid;grid-template-columns:1fr 1fr;gap:8px}
  .dbox{border:1px solid var(--line);border-radius:6px;padding:8px 10px}
  .dbox .k{font-size:10px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
  .dbox .v{font-size:13px;font-weight:600;margin-top:3px}
  .rule{font-size:11px;color:var(--muted);line-height:1.75;margin-top:10px;border-top:1px solid var(--line);padding-top:10px}
  .rule code{color:var(--txt);background:#0d1117;padding:1px 5px;border-radius:3px;font-size:11px}
  .iface-note{font-size:10px;color:var(--muted);text-transform:uppercase;letter-spacing:.06em;margin-left:auto}
</style>
</head>
<body>
<header>
  <h1>T2CAN Unified</h1>
  <span class="pill" id="conn">connecting&hellip;</span>
</header>
<div class="tabs">
  <button class="tab-btn active" onclick="showTab('nag',this)">Nag Echo</button>
  <button class="tab-btn" onclick="showTab('summon',this)">Summon Unlock</button>
</div>

<!-- NAG ECHO TAB -->
<main id="main-nag" class="active">

  <section class="panel">
    <h2>mode <span class="iface-note">CAN A &mdash; MCP2515</span></h2>
    <div class="modes">
      <button id="modeA" class="primary">A &mdash; Simple</button>
      <button id="modeB">B &mdash; TSL6P (burst/pause)</button>
      <button id="modeC">C &mdash; State machine</button>
      <button id="modeR" class="danger" style="margin-left:auto">Reset</button>
    </div>
    <div style="font-size:11px;color:var(--muted);line-height:1.55">
      <b>A</b>: CAN 0x370, fixed +1.80&nbsp;Nm, handsOn=1 always. Proven on MY 2022 HW3 pre-Juniper.<br>
      <b>B</b>: Configurable target CAN ID, default 0x370, 4-value torque cycle, time-bursty (<span id="lbl_burst">1000</span> ms inject / <span id="lbl_pause">1500</span> ms rest).<br>
      <b>C</b>: State machine on DAS_autopilotHandsOnState. Refuses to inject if context CAN frames are stale &gt;1&nbsp;s.
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
      <div class="stat"><div class="k">CAN A state</div><div class="v" id="s_cs">&mdash;</div></div>
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
      Mode C will only inject when both freshness indicators are green (last frame received within 1&nbsp;s).
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
    </div>
  </section>
</main>

<!-- SUMMON UNLOCK TAB -->
<main id="main-summon">
  <section class="panel">
    <h2>Summon Unlock <span class="iface-note">CAN B &mdash; TWAI</span></h2>
    <div class="big" id="sum_big">&mdash;</div>
    <div class="tbar">
      <button class="primary" onclick="postSummon('/api/summon/enable')">Enable</button>
      <button class="danger" onclick="postSummon('/api/summon/disable')">Disable</button>
    </div>
  </section>

  <section class="panel">
    <h2>Injection Gate</h2>
    <div class="gate-status" id="sum_gate_status">&mdash;</div>
    <br>
    <div class="gate">
      <div class="gbox" id="sum_g_pk">
        <div class="k">Parked</div>
        <div class="v" id="sum_g_pk_v">&mdash;</div>
      </div>
      <div class="gbox" id="sum_g_su">
        <div class="k">Summoning</div>
        <div class="v" id="sum_g_su_v">&mdash;</div>
      </div>
    </div>
    <div style="margin-top:8px;font-size:11px;color:var(--muted);display:flex;align-items:center;gap:6px">
      <span>APActive (info only) :</span>
      <span id="sum_g_ap_v" style="font-weight:600">&mdash;</span>
    </div>
    <p style="font-size:11px;color:var(--muted);margin:6px 0 0">
      Gate open if Parked OR Summoning only.<br>
      APActive (AP/TACC) only, does not start injection.
    </p>
  </section>

  <section class="panel">
    <h2>Discrimination Summon / TACC</h2>
    <div class="disc">
      <div class="dbox">
        <div class="k">ACA &mdash; DI_autonomyControlActive</div>
        <div class="v" id="sum_d_aca">&mdash;</div>
        <div style="font-size:10px;color:var(--muted);margin-top:2px">CAN 280 &middot; data[6] bit 2</div>
      </div>
      <div class="dbox">
        <div class="k">SPR &mdash; UI_selfParkRequest</div>
        <div class="v" id="sum_d_spr">&mdash;</div>
        <div style="font-size:10px;color:var(--muted);margin-top:2px">CAN 1016 &middot; data[3] bits 4-7</div>
      </div>
    </div>
    <p style="font-size:11px;color:var(--muted);margin:8px 0 0">
      Summoning = ACA &amp;&amp; SPR. TACC only (ACA=1, SPR=0) does not trigger the injection.
    </p>
  </section>

  <section class="panel">
    <h2>Frames CAN</h2>
    <div class="row">
      <div class="stat"><div class="k">280 (gear/ACA)</div><div class="v" id="sum_s_280">&mdash;</div></div>
      <div class="stat"><div class="k">390 (DIF gear)</div><div class="v" id="sum_s_390">&mdash;</div></div>
      <div class="stat"><div class="k">921 (AP status)</div><div class="v" id="sum_s_921">&mdash;</div></div>
      <div class="stat"><div class="k">1016 (SPR)</div><div class="v" id="sum_s_1016">&mdash;</div></div>
      <div class="stat"><div class="k">1021 mux1 rx</div><div class="v" id="sum_s_rx">&mdash;</div></div>
      <div class="stat"><div class="k">TX ok</div><div class="v ok" id="sum_s_ok">&mdash;</div></div>
      <div class="stat"><div class="k">TX fail</div><div class="v" id="sum_s_fail">&mdash;</div></div>
      <div class="stat"><div class="k">CAN B state</div><div class="v" id="sum_s_can">&mdash;</div></div>
      <div class="stat"><div class="k">Uptime</div><div class="v" id="sum_s_up">&mdash;</div></div>
    </div>
  </section>

  <section class="panel">
    <h2>Rules applied</h2>
    <div class="rule">
      ID <code>0x3FD</code> (1021) &mdash; mux <code>1</code><br>
      &bull; bit <code>19</code> &rarr; <code>0</code> &mdash; Clears the summon EU restriction bit<br>
      &bull; bit <code>47</code> &rarr; <code>1</code> &mdash; Sets the summon enable bit<br><br>
      Injection condition :<br>
      &nbsp;&nbsp;<code>summonEnabled &amp;&amp; (Parked || Summoning)</code><br>
      Summoning :<br>
      &nbsp;&nbsp;<code>lastAca &amp;&amp; sprSeen</code> &mdash; ACA drop clears sprSeen
    </div>
  </section>
</main>

<div class="footer">
  T2CAN Unified &middot;
  <a href="/api/nag/config" target="_blank">/api/nag/config</a> &middot;
  <a href="/api/nag/stats" target="_blank">/api/nag/stats</a> &middot;
  <a href="/api/summon/stats" target="_blank">/api/summon/stats</a><br>
  research / educational only &middot; not for use on public roads
</div>
<div class="toast" id="toast">saved</div>

<script>
const $ = id => document.getElementById(id);

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
  ['modeA','modeB','modeC'].forEach((id,m) => $(id).classList.toggle('primary', nagCfg.mode === m));
  $('modeC_panel').style.opacity = (nagCfg.mode === 2) ? '1' : '0.55';
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
    $('modeC').disabled = false;
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

function freshTag(ms) {
  if (ms == null || ms === 999999) return ['stale', 'bad'];
  if (ms > 5000) return [(ms/1000).toFixed(1)+' s ago', 'bad'];
  if (ms > 1000) return [ms+' ms ago', 'warn'];
  return [ms+' ms ago', 'ok'];
}

async function tickNagStats() {
  if (nagIsLoading) return;
  try {
    nagIsLoading = true;
    const r = await fetch('/api/nag/stats');
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
    $('c_ap').textContent  = s.apState;
    $('c_ho').textContent  = s.handsOnState;
    $('c_st').textContent  = s.steeringDeg + ' °';
    let [t1,c1] = freshTag(s.apStaleMs);
    let [t2,c2] = freshTag(s.stStaleMs);
    $('c_apf').textContent = t1; $('c_apf').className = 'v ' + c1;
    $('c_stf').textContent = t2; $('c_stf').className = 'v ' + c2;
  } catch(e) {
    $('conn').textContent = 'lost';
    $('conn').className   = 'pill bad';
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
$('modeC').onclick  = () => setNagMode(2);
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

function setGbox(id, active) {
  const el = $(id);
  el.className = 'gbox ' + (active ? 'active' : 'inactive');
}

async function fetchSummonStats() {
  try {
    const s = await fetch('/api/summon/stats').then(r => r.json());
    const big = $('sum_big');
    big.textContent = s.enabled ? 'ON' : 'OFF';
    big.className   = 'big ' + (s.enabled ? 'on' : 'off');

    const gate = s.gate;
    const gs = $('sum_gate_status');
    gs.textContent = gate ? 'OPEN — injection allowed' : 'CLOSED — injection blocked';
    gs.className   = 'gate-status ' + (gate ? 'open' : 'closed');

    $('sum_g_ap_v').textContent = s.ap ? 'ON' : 'off';
    $('sum_g_ap_v').style.color = s.ap ? 'var(--ok)' : 'var(--muted)';
    setGbox('sum_g_pk', s.parked); $('sum_g_pk_v').textContent = s.parked ? 'ON' : 'off';
    setGbox('sum_g_su', s.summon); $('sum_g_su_v').textContent = s.summon ? 'ON' : 'off';

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

// ===== STARTUP =====
loadNagConfig().then(() => { tickNagStats(); setInterval(tickNagStats, 500); });
fetchSummonStats();
setInterval(fetchSummonStats, 800);
</script>
</body>
</html>
)HTML";