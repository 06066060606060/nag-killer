const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#151515">
<title>Nag Killer</title>
<style>
:root{--bg:#f2f2f0;--card:#fff;--ink:#171717;--muted:#777;--line:#dededb;--green:#2b8a55;--amber:#b27a19;--red:#c9403a;--shadow:0 8px 30px rgba(0,0,0,.07)}
@media(prefers-color-scheme:dark){:root{--bg:#151515;--card:#202020;--ink:#f2f2f2;--muted:#999;--line:#343434;--shadow:none}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}html,body{margin:0;background:var(--bg);color:var(--ink);font-family:-apple-system,BlinkMacSystemFont,"Helvetica Neue",Arial,sans-serif}button,input{font:inherit}button{cursor:pointer}.wrap{max-width:520px;margin:auto;min-height:100vh;padding:calc(12px + env(safe-area-inset-top)) 14px calc(26px + env(safe-area-inset-bottom))}.top{height:48px;display:flex;align-items:center;justify-content:space-between}.logo{font-size:15px;font-weight:700;letter-spacing:-.02em}.live{display:flex;align-items:center;gap:7px;font-size:11px;color:var(--muted)}.live:before{content:"";width:7px;height:7px;border-radius:50%;background:var(--red)}.live.ok:before{background:var(--green)}
.drive{padding:24px 8px 18px;text-align:center}.small{font-size:10px;color:var(--muted);font-weight:650;letter-spacing:.06em}.mode{font-size:46px;line-height:1.03;font-weight:510;letter-spacing:-.055em;margin:8px 0 20px}.mode.good{color:var(--green)}.mode.warn{color:var(--amber)}.mode.bad{color:var(--red)}.mode.compact{font-size:30px;letter-spacing:-.04em}
.metrics{display:grid;grid-template-columns:repeat(2,1fr);gap:1px;background:var(--line);border:1px solid var(--line);border-radius:18px;overflow:hidden;box-shadow:var(--shadow)}.metric{background:var(--card);padding:14px 7px;text-align:center}.metric .k{font-size:9px;color:var(--muted);font-weight:650}.metric .v{font-size:17px;margin-top:5px;font-weight:580;letter-spacing:-.03em}.good{color:var(--green)!important}.warn{color:var(--amber)!important}.bad{color:var(--red)!important}
.quick{margin-top:14px;background:var(--card);border-radius:20px;overflow:hidden;border:1px solid var(--line);box-shadow:var(--shadow)}.qrow{min-height:60px;padding:0 16px;display:flex;align-items:center;justify-content:space-between;gap:10px}.qrow+.qrow{border-top:1px solid var(--line)}.ql{display:flex;align-items:center;gap:12px;min-width:0}.ico{width:28px;height:28px;border-radius:50%;background:var(--bg);display:grid;place-items:center;font-size:12px;font-weight:700;flex:0 0 auto}.qt{font-size:14px;font-weight:560}.qs{font-size:10px;color:var(--muted);margin-top:2px;white-space:nowrap}.rightctl{display:flex;align-items:center;gap:9px;min-width:0}.prio{font-size:9.5px;font-weight:680;color:var(--green);white-space:nowrap;text-align:right}.prio b{display:block;font-size:7.5px;color:var(--muted);font-weight:620;margin-bottom:2px;letter-spacing:.05em}.toggle{position:relative;width:43px;height:25px;flex:0 0 auto}.toggle input{display:none}.track{position:absolute;inset:0;border-radius:99px;background:#b8b8b3;transition:.18s}.track:after{content:"";position:absolute;width:19px;height:19px;left:3px;top:3px;border-radius:50%;background:#fff;transition:.18s;box-shadow:0 1px 3px rgba(0,0,0,.25)}.toggle input:checked+.track{background:var(--green)}.toggle input:checked+.track:after{transform:translateX(18px)}
.gatecard{margin-top:14px;background:var(--card);border:1px solid var(--line);border-radius:20px;padding:15px;box-shadow:var(--shadow)}.gatehead{display:flex;align-items:center;justify-content:space-between;gap:12px}.gatetitle{font-size:13px;font-weight:590}.gatepill{border:1px solid var(--green);color:var(--green);border-radius:999px;padding:7px 11px;font-size:11px;font-weight:700;white-space:nowrap}.gatepill.closed{border-color:var(--muted);color:var(--muted)}.gategrid{display:grid;grid-template-columns:repeat(4,1fr);gap:1px;background:var(--line);border:1px solid var(--line);border-radius:14px;overflow:hidden;margin-top:12px}.gateitem{background:var(--card);padding:11px 5px;text-align:center}.gk{font-size:8px;color:var(--muted);font-weight:650;letter-spacing:.04em}.gv{font-size:11px;margin-top:4px;font-weight:650;white-space:nowrap}
.canline{margin-top:14px;display:grid;grid-template-columns:1fr 1fr;gap:8px}.can{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:15px;box-shadow:var(--shadow)}.canTop{display:flex;align-items:center;justify-content:space-between}.canName{font-size:12px;font-weight:590}.badge{font-size:9px;color:var(--green);font-weight:700}.canVal{font-size:22px;margin-top:10px;font-weight:520;letter-spacing:-.04em}.canSub{font-size:9px;color:var(--muted);margin-top:4px}
.drawer{margin-top:14px;background:var(--card);border:1px solid var(--line);border-radius:20px;overflow:hidden;box-shadow:var(--shadow)}details+details{border-top:1px solid var(--line)}summary{list-style:none;cursor:pointer;padding:17px 16px;display:flex;justify-content:space-between;align-items:center;font-size:13px;font-weight:570}summary::-webkit-details-marker{display:none}.arrow{font-size:15px;color:var(--muted);transition:.18s}details[open] .arrow{transform:rotate(90deg)}.body{padding:0 16px 15px}.r{display:flex;justify-content:space-between;gap:16px;padding:10px 0;font-size:11px;border-top:1px solid var(--line)}.rk{color:var(--muted)}.rv{text-align:right;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}.subhead{font-size:9px;color:var(--muted);font-weight:700;letter-spacing:.08em;margin:13px 0 7px}.controlgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;padding-top:6px}.btn{border:0;border-radius:13px;padding:12px;background:var(--bg);color:var(--ink);font-size:11px;font-weight:600}.btn.primary{background:var(--ink);color:var(--bg)}.btn.danger{color:var(--red)}.fieldgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px}.field{background:var(--bg);border-radius:13px;padding:9px 10px}.field label{display:block;font-size:8px;color:var(--muted);font-weight:650;margin-bottom:5px}.field input{width:100%;border:0;outline:0;background:transparent;color:var(--ink);font-size:13px}.field.full{grid-column:1/-1}.ota{margin-top:8px}.ota input[type=file]{width:100%;font-size:10px;color:var(--muted);margin:6px 0 8px}.progress{height:4px;background:var(--line);border-radius:99px;overflow:hidden;display:none}.progress.show{display:block}.progress>i{display:block;height:100%;width:0;background:var(--green)}.otaMsg{font-size:9px;color:var(--muted);margin-top:7px}.linkbtn{display:flex;align-items:center;justify-content:center;text-decoration:none}
.toast{position:fixed;left:50%;bottom:calc(24px + env(safe-area-inset-bottom));transform:translate(-50%,20px);background:#111;color:#fff;padding:10px 14px;border-radius:999px;font-size:11px;opacity:0;pointer-events:none;transition:.2s;z-index:20}.toast.show{opacity:.94;transform:translate(-50%,0)}.bottom{text-align:center;margin-top:18px;font-size:9px;color:var(--muted)}
@media(max-width:380px){.mode{font-size:40px}.mode.compact{font-size:27px}.qrow{padding:0 13px}.qs{max-width:140px;overflow:hidden;text-overflow:ellipsis}.prio{font-size:8.5px}.gategrid{grid-template-columns:1fr 1fr}.canline{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="logo">NAG KILLER</div>
    <div class="live" id="conn">Connecting</div>
  </div>

  <section class="drive">
    <div class="small">STEERING / TORQUE CONTROL</div>
    <div class="mode" id="nagMode">—</div>
    <div class="metrics">
      <div class="metric"><div class="k">HANDS ON</div><div class="v" id="s_ho">—</div></div>
      <div class="metric"><div class="k">TORQUE</div><div class="v" id="s_tq">—</div></div>
    </div>
  </section>

  <section class="quick">
    <div class="qrow">
      <div class="ql"><div class="ico">A</div><div><div class="qt">A-Simple</div><div class="qs">Fixed +1.80 Nm · HandsOn=1</div></div></div>
      <button class="btn" id="modeA">Select</button>
    </div>
    <div class="qrow">
      <div class="ql"><div class="ico">B</div><div><div class="qt">B-TSL6P burst/pause</div><div class="qs">Configurable torque cycle</div></div></div>
      <button class="btn" id="modeB">Select</button>
    </div>
    <div class="qrow">
      <div class="ql"><div class="ico">C</div><div><div class="qt">C-Random Walk</div><div class="qs">Random walk variation by wewe9v9v</div></div></div>
      <button class="btn" id="modeC">Select</button>
    </div>
  </section>

  <section class="canline">
    <section class="can">
      <div class="canTop"><div class="canName">TARGET CAN</div><div class="badge" id="canBadge">—</div></div>
      <div class="canVal" id="s_id">0x370</div>
      <div class="canSub">RX + TX · TWAI</div>
    </section>
    <section class="can">
      <div class="canTop"><div class="canName">CAN STATE</div><div class="badge" id="s_cs_badge">—</div></div>
      <div class="canVal" id="s_cs">—</div>
      <div class="canSub">TWAI automatic recovery</div>
    </section>
  </section>

  <section class="drawer">
    <details>
      <summary><span>Live details</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">RX Frames</span><span class="rv" id="s_rx">0</span></div>
        <div class="r"><span class="rk">Echo Sent</span><span class="rv" id="s_echo">0</span></div>
        <div class="r"><span class="rk">TX OK / FAIL</span><span class="rv" id="s_tx">0 / 0</span></div>
        <div class="r"><span class="rk">Last latency</span><span class="rv" id="s_lat">—</span></div>
        <div class="r"><span class="rk">Last injected</span><span class="rv" id="s_inj">—</span></div>
        <div class="r"><span class="rk">Uptime</span><span class="rv" id="s_up">—</span></div>
      </div>
    </details>

    <details>
      <summary><span>Mode description</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="subhead">MODE A · SIMPLE</div>
        <div class="r"><span class="rk">CAN ID</span><span class="rv">0x370</span></div>
        <div class="r"><span class="rk">Torque</span><span class="rv">Fixed +1.80 Nm</span></div>
        <div class="r"><span class="rk">HandsOn</span><span class="rv">1 always</span></div>

        <div class="subhead">MODE B · TSL6P BURST / PAUSE</div>
        <div class="r"><span class="rk">Target</span><span class="rv">Configurable CAN ID</span></div>
        <div class="r"><span class="rk">Torque</span><span class="rv">Configurable 4-value cycle</span></div>
        <div class="r"><span class="rk">Timing</span><span class="rv">Burst + pause</span></div>

        <div class="subhead">MODE C · RANDOM WALK</div>
        <div class="r"><span class="rk">Byte 2</span><span class="rv">0x08 fixed</span></div>
        <div class="r"><span class="rk">Byte 3 range</span><span class="rv">0x98 → 0xB6</span></div>
        <div class="r"><span class="rk">Update interval</span><span class="rv">200 ms</span></div>
        <div class="r"><span class="rk">Maximum step</span><span class="rv">±15 counts</span></div>
        <div class="r"><span class="rk">HandsOn</span><span class="rv">1</span></div>
      </div>
    </details>

    <details>
      <summary><span>Runtime Overrides</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="fieldgrid">
          <div class="field"><label>TARGET CAN ID (HEX)</label><input type="text" id="f_id" placeholder="0x370"></div>
          <div class="field"><label>HANDS ON RATE (%)</label><input type="number" id="f_ho" min="0" max="100" step="1"></div>
          <div class="field"><label>BURST (MS)</label><input type="number" id="f_burst" min="50" max="10000" step="50"></div>
          <div class="field"><label>PAUSE (MS)</label><input type="number" id="f_pause" min="0" max="10000" step="50"></div>
        </div>

        <details>
          <summary><span>Torque table</span><span class="arrow">›</span></summary>
          <div class="body">
            <table style="width:100%;font-size:10px;border-collapse:collapse">
              <thead><tr><th>#</th><th>b2</th><th>b3</th><th>Nm</th></tr></thead>
              <tbody id="tq_tbody"></tbody>
            </table>
            <div class="controlgrid">
              <button class="btn" id="tq_add">+ row</button>
              <button class="btn" id="tq_del">− row</button>
            </div>
          </div>
        </details>

        <div class="controlgrid">
          <button class="btn" id="toggle">Disable</button>
          <button class="btn primary" id="apply">Apply overrides</button>
          <button class="btn danger" id="modeR">Reset</button>
        </div>
        <div class="r"><span class="rk">Firmware torque cap</span><span class="rv">±1.80 Nm</span></div>
      </div>
    </details>

    <details>
      <summary><span>Firmware OTA</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="subhead">FIRMWARE UPDATE</div>
        <div class="ota">
          <input id="otaFile" type="file" accept=".bin,application/octet-stream">
          <button class="btn primary" id="otaUpload" style="width:100%">Upload Firmware</button>
          <div class="progress" id="otaProgress"><i id="otaBar"></i></div>
          <div class="otaMsg" id="otaMsg">Ready</div>
        </div>
      </div>
    </details>

    <details>
      <summary><span>System</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">CAN recovery</span><span class="rv">TWAI automatic recovery</span></div>
        <div class="r"><span class="rk">CAN state</span><span class="rv" id="sysCanState">—</span></div>
        <div class="r"><span class="rk">Firmware</span><span class="rv">Nag Killer %FIRMWARE_VERSION%</span></div>
        <div class="controlgrid">
          <a class="btn linkbtn" href="/api/config" target="_blank">Config JSON</a>
          <a class="btn linkbtn" href="/api/stats" target="_blank">Stats JSON</a>
        </div>
      </div>
    </details>
  </section>

  <div class="bottom">NAG KILLER · %FIRMWARE_VERSION% · TWAI · OTA</div>
</div>
<div class="toast" id="toast"></div>
<script>
const $ = id => document.getElementById(id);
let cfg = null;
let isLoading = false;

function showToast(msg) {
  const t = $('toast');
  if (!t) return;
  t.textContent = msg;
  t.classList.add('show');
  clearTimeout(showToast._h);
  showToast._h = setTimeout(() => t.classList.remove('show'), 1500);
}

function setWifiStatus(online, detail) {
  const e = $('conn');
  if (!e) return;
  e.textContent = online ? (detail || 'WiFi connected') : (detail || 'WiFi lost');
  e.className = online ? 'live ok' : 'live';
}

function updateNagModeDisplay(mode) {
  const e = $('nagMode');
  if (!e) return;
  const n = Number(mode);
  const names = ['A-Simple', 'B-TSL6P', 'C-Random Walk'];
  e.textContent = names[n] || '—';
  e.className = 'mode ' + (n === 2 ? 'warn' : n === 0 ? 'good' : '');
}

function nmFromBytes(b2, b3) {
  const raw = ((b2 & 0x0F) << 8) | (b3 & 0xFF);
  return raw * 0.01 - 20.5;
}

function renderTorque() {
  if (!cfg || !Array.isArray(cfg.torque)) return;
  const tb = $('tq_tbody');
  if (!tb) return;
  tb.innerHTML = '';

  cfg.torque.forEach((t, i) => {
    const tr = document.createElement('tr');
    const b2Hex = '0x' + Number(t.b2).toString(16).padStart(2,'0').toUpperCase();
    const b3Hex = '0x' + Number(t.b3).toString(16).padStart(2,'0').toUpperCase();

    tr.innerHTML = `<td>${i}</td>
      <td><input type="text" data-i="${i}" data-k="b2" value="${b2Hex}"></td>
      <td><input type="text" data-i="${i}" data-k="b3" value="${b3Hex}"></td>
      <td id="nm_${i}">${nmFromBytes(t.b2,t.b3).toFixed(2)}</td>`;
    tb.appendChild(tr);
  });

  tb.querySelectorAll('input').forEach(inp => inp.addEventListener('input', e => {
    const i = Number(e.target.dataset.i);
    const k = e.target.dataset.k;
    const v = parseInt(e.target.value, 16);
    if (Number.isFinite(v) && cfg && cfg.torque && cfg.torque[i]) {
      cfg.torque[i][k] = v & 0xFF;
      const nm = $('nm_' + i);
      if (nm) nm.textContent = nmFromBytes(cfg.torque[i].b2, cfg.torque[i].b3).toFixed(2);
    }
  }));
}

function renderConfig() {
  if (!cfg) return;

  if ($('f_id')) $('f_id').value =
    '0x' + Number(cfg.targetId).toString(16).toUpperCase().padStart(3,'0');
  if ($('f_ho')) $('f_ho').value = cfg.hoRatePct;
  if ($('f_burst')) $('f_burst').value = cfg.burstMs;
  if ($('f_pause')) $('f_pause').value = cfg.pauseMs;

  if ($('toggle')) $('toggle').textContent = cfg.enabled ? 'Disable' : 'Enable';

  const mode = Number(cfg.mode);
  ['modeA','modeB','modeC'].forEach((id, m) => {
    const b = $(id);
    if (!b) return;
    b.classList.toggle('primary', mode === m);
    b.textContent = mode === m ? 'Selected' : 'Select';
    b.disabled = false;
  });

  updateNagModeDisplay(mode);
  renderTorque();
}

async function loadConfig() {
  try {
    const r = await fetch('/api/config?_=' + Date.now(), {
      method: 'GET',
      cache: 'no-store'
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);

    const data = await r.json();
    if (!data || typeof data.mode === 'undefined') {
      throw new Error('invalid config');
    }

    cfg = data;
    renderConfig();

    ['toggle','apply','modeA','modeB','modeC','modeR','tq_add','tq_del'].forEach(id => {
      if ($(id)) $(id).disabled = false;
    });

    setWifiStatus(true, 'WiFi connected');
    return true;
  } catch (e) {
    setWifiStatus(false, 'WiFi lost');
    return false;
  }
}

async function tickStats() {
  if (isLoading) return;

  try {
    isLoading = true;
    const r = await fetch('/api/stats?_=' + Date.now(), {
      method: 'GET',
      cache: 'no-store'
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);

    const s = await r.json();

    if ($('s_rx'))   $('s_rx').textContent = s.rx ?? 0;
    if ($('s_echo')) $('s_echo').textContent = s.echo ?? 0;
    if ($('s_tx'))   $('s_tx').textContent = (s.txOk ?? 0) + ' / ' + (s.txFail ?? 0);
    if ($('s_lat'))  $('s_lat').textContent = s.latUs ? (s.latUs + ' µs') : '—';
    if ($('s_ho'))   $('s_ho').textContent = s.ho != null ? s.ho : '—';
    if ($('s_tq'))   $('s_tq').textContent =
      s.torque != null ? ((s.torque >= 0 ? '+' : '') + Number(s.torque).toFixed(2) + ' Nm') : '—';
    if ($('s_inj'))  $('s_inj').textContent =
      s.injNm != null ? ((s.injNm >= 0 ? '+' : '') + Number(s.injNm).toFixed(2) + ' Nm  ho=' + s.injHo) : '—';
    if ($('s_up'))   $('s_up').textContent = s.uptimeS ? (s.uptimeS + ' s') : '—';

    const cs = ['stopped','running','bus-off','recovering'][Number(s.canState)] || 'unknown';
    if ($('s_cs')) {
      $('s_cs').textContent = cs;
      $('s_cs').className = 'canVal ' + (Number(s.canState) === 1 ? 'good' : Number(s.canState) === 2 ? 'bad' : 'warn');
    }
    if ($('s_cs_badge')) $('s_cs_badge').textContent = cs;
    if ($('sysCanState')) $('sysCanState').textContent = cs;
    if ($('s_id') && cfg) $('s_id').textContent =
      '0x' + Number(cfg.targetId).toString(16).toUpperCase().padStart(3,'0');

    setWifiStatus(true, 'WiFi connected');
  } catch (e) {
    setWifiStatus(false, 'WiFi lost');
  } finally {
    isLoading = false;
  }
}

async function setMode(m) {
  const mode = Number(m);
  if (![0,1,2].includes(mode)) return;

  // Disable only the three mode buttons while the request is in progress.
  ['modeA','modeB','modeC'].forEach(id => { if ($(id)) $(id).disabled = true; });

  try {
    const r = await fetch('/api/mode?m=' + mode, {
      method: 'POST',
      cache: 'no-store'
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);

    const data = await r.json();
    if (!data || Number(data.mode) !== mode) {
      throw new Error('mode not confirmed');
    }

    cfg = data;
    renderConfig();
    setWifiStatus(true, 'WiFi connected');
    showToast('Mode ' + ['A','B','C'][mode] + ' applied');
  } catch(e) {
    showToast('Mode error: ' + e.message);
    await loadConfig();
  } finally {
    ['modeA','modeB','modeC'].forEach(id => { if ($(id)) $(id).disabled = false; });
  }
}

async function applyOverrides() {
  if (!cfg || !Array.isArray(cfg.torque)) {
    showToast('not ready');
    return;
  }

  try {
    const id = parseInt($('f_id').value, 16);
    const ho = Number($('f_ho').value);
    const burst = Number($('f_burst').value);
    const pause = Number($('f_pause').value);

    if (!Number.isFinite(id)) {
      showToast('invalid hex ID');
      return;
    }

    const params = new URLSearchParams();
    params.set('targetId', String(id));
    params.set('hoRatePct', String(ho));
    params.set('burstMs', String(burst));
    params.set('pauseMs', String(pause));
    params.set('count', String(cfg.torque.length));

    cfg.torque.forEach((t, i) => {
      params.set('b2_' + i, String(t.b2));
      params.set('b3_' + i, String(t.b3));
    });

    const r = await fetch('/api/update?' + params.toString(), {
      method: 'POST',
      cache: 'no-store'
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);

    cfg = await r.json();
    renderConfig();
    showToast('saved');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

async function resetConfig() {
  if (!cfg) { showToast('not ready'); return; }
  if (!confirm('Reset all settings to Mode A defaults?')) return;

  try {
    const r = await fetch('/api/reset', { method:'POST', cache:'no-store' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    cfg = await r.json();
    renderConfig();
    showToast('reset');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

async function toggleEnabled() {
  if (!cfg) { showToast('not ready'); return; }

  try {
    const params = new URLSearchParams({ enabled: cfg.enabled ? '0' : '1' });
    const r = await fetch('/api/update?' + params.toString(), {
      method:'POST',
      cache:'no-store'
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);

    cfg = await r.json();
    renderConfig();
    showToast(cfg.enabled ? 'enabled' : 'disabled');
  } catch(e) {
    showToast('error: ' + e.message);
  }
}

function initDashboard() {
  ['modeA','modeB','modeC','modeR','tq_add','tq_del'].forEach(id => {
    if ($(id)) $(id).disabled = true;
  });

  if ($('modeA')) $('modeA').onclick = () => setMode(0);
  if ($('modeB')) $('modeB').onclick = () => setMode(1);
  if ($('modeC')) $('modeC').onclick = () => setMode(2);
  if ($('modeR')) $('modeR').onclick = resetConfig;
  if ($('apply')) $('apply').onclick = applyOverrides;
  if ($('toggle')) $('toggle').onclick = toggleEnabled;

  if ($('tq_add')) $('tq_add').onclick = () => {
    if (!cfg || !cfg.torque) return;
    if (cfg.torque.length >= 8) { showToast('max 8 entries'); return; }
    cfg.torque.push({ b2: 0x08, b3: 0xB6 });
    renderTorque();
  };

  if ($('tq_del')) $('tq_del').onclick = () => {
    if (!cfg || !cfg.torque) return;
    if (cfg.torque.length <= 1) { showToast('min 1 entry'); return; }
    cfg.torque.pop();
    renderTorque();
  };

  // OTA
  if ($('otaUpload')) $('otaUpload').onclick = async () => {
    const file = $('otaFile')?.files?.[0];
    if (!file) { showToast('select a .bin file'); return; }
    if (!confirm('Flash this firmware and reboot the ESP32?')) return;

    const status = $('otaMsg');
    const bar = $('otaBar');
    const progress = $('otaProgress');
    const btn = $('otaUpload');

    btn.disabled = true;
    if (progress) progress.classList.add('show');
    if (bar) bar.style.width = '0%';
    if (status) status.textContent = 'Uploading…';

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update', true);

    xhr.upload.onprogress = e => {
      if (e.lengthComputable) {
        const pct = (e.loaded / e.total) * 100;
        if (bar) bar.style.width = pct.toFixed(1) + '%';
        if (status) status.textContent = 'Uploading ' + pct.toFixed(0) + '%';
      }
    };

    xhr.onload = () => {
      try {
        const r = JSON.parse(xhr.responseText);
        if (xhr.status === 200 && r.ok) {
          if (bar) bar.style.width = '100%';
          if (status) status.textContent = 'Update successful — rebooting…';
          showToast('OTA successful');
          setTimeout(() => location.reload(), 5000);
        } else {
          throw new Error(r.error || ('HTTP ' + xhr.status));
        }
      } catch (e) {
        if (status) status.textContent = 'OTA error: ' + e.message;
        showToast('OTA error');
        btn.disabled = false;
      }
    };

    xhr.onerror = () => {
      if (status) status.textContent = 'Connection lost — the ESP32 may be rebooting.';
      btn.disabled = false;
    };

    xhr.send(file);
  };

  setWifiStatus(false, 'Connecting…');

  loadConfig().then(() => {
    tickStats();
    setInterval(tickStats, 500);
  });
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initDashboard);
} else {
  initDashboard();
}
</script>
</body></html>
)HTML";
