// portal_html.h - HTML for the captive portal config page
// Dark themed, single page, no external resources (works offline on the AP)
#pragma once

const char PORTAL_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Memento Mori Setup</title>
<style>
  :root {
    --bg: #0a0a0b;
    --panel: #15161a;
    --border: #2a2c33;
    --text: #e8e8ea;
    --muted: #888a92;
    --accent: #00d563;
    --danger: #ff4d4d;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 24px;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: linear-gradient(180deg, #0a0a0b 0%, #0f1014 100%);
    color: var(--text);
    min-height: 100vh;
  }
  .wrap { max-width: 480px; margin: 0 auto; }
  h1 {
    font-size: 22px; font-weight: 600;
    margin: 0 0 4px;
    letter-spacing: -0.01em;
  }
  .sub {
    color: var(--muted); font-size: 13px;
    margin-bottom: 24px;
  }
  .card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 20px;
    margin-bottom: 16px;
  }
  .card h2 {
    font-size: 13px; font-weight: 600;
    text-transform: uppercase; letter-spacing: 0.08em;
    color: var(--muted);
    margin: 0 0 16px;
  }
  label {
    display: block;
    font-size: 12px;
    color: var(--muted);
    margin-bottom: 6px;
    margin-top: 12px;
  }
  label:first-of-type { margin-top: 0; }
  input, select {
    width: 100%;
    background: #0a0a0b;
    border: 1px solid var(--border);
    color: var(--text);
    border-radius: 8px;
    padding: 10px 12px;
    font-size: 15px;
    font-family: inherit;
    transition: border-color 0.15s;
  }
  input:focus, select:focus {
    outline: none;
    border-color: var(--accent);
  }
  .row { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px; }
  button {
    width: 100%;
    background: var(--accent);
    color: #000;
    border: none;
    border-radius: 8px;
    padding: 14px;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
    margin-top: 8px;
    transition: opacity 0.15s;
  }
  button:hover { opacity: 0.85; }
  .scan-list {
    max-height: 180px; overflow-y: auto;
    margin-top: 12px;
    border: 1px solid var(--border);
    border-radius: 8px;
  }
  .scan-list .net {
    padding: 10px 12px;
    font-size: 14px;
    cursor: pointer;
    border-bottom: 1px solid var(--border);
    display: flex; justify-content: space-between;
  }
  .scan-list .net:last-child { border-bottom: none; }
  .scan-list .net:hover { background: #1c1d22; }
  .rssi { color: var(--muted); font-size: 12px; }
  .help {
    font-size: 11px; color: var(--muted);
    margin-top: 6px;
  }
  #status {
    text-align: center;
    padding: 12px;
    border-radius: 8px;
    font-size: 13px;
    margin-top: 12px;
    display: none;
  }
  #status.ok { background: rgba(0, 213, 99, 0.1); color: var(--accent); display: block; }
  #status.err { background: rgba(255, 77, 77, 0.1); color: var(--danger); display: block; }
  .theme-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 8px;
  }
  .theme-opt {
    display: flex; align-items: center; gap: 10px;
    padding: 10px;
    border: 1px solid var(--border);
    border-radius: 8px;
    cursor: pointer;
    transition: border-color 0.15s, background 0.15s;
    background: #0a0a0b;
  }
  .theme-opt:hover { border-color: var(--muted); }
  .theme-opt.sel  { border-color: var(--accent); background: rgba(0,213,99,0.08); }
  .theme-swatch {
    width: 32px; height: 32px;
    border-radius: 6px;
    flex-shrink: 0;
    display: flex; align-items: center; justify-content: center;
    font-size: 14px; font-weight: 700;
    border: 1px solid rgba(255,255,255,0.08);
  }
  .theme-name {
    font-size: 13px;
    color: var(--text);
  }
</style>
</head>
<body>
<div class="wrap">
  <h1>Memento Mori Setup</h1>
  <div class="sub">memento mori, configured properly</div>

  <div class="card">
    <h2>WiFi</h2>
    <label>Network</label>
    <input id="ssid" type="text" placeholder="SSID" autocomplete="off">
    <button type="button" onclick="scan()" style="background:#2a2c33;color:var(--text);margin-top:8px;font-size:13px;padding:8px;">Scan nearby</button>
    <div id="scanlist" class="scan-list" style="display:none"></div>
    <label>Password</label>
    <input id="pass" type="password" placeholder="WiFi password" autocomplete="off">
  </div>

  <div class="card">
    <h2>You</h2>
    <label>Date of birth</label>
    <div class="row">
      <input id="dy" type="number" placeholder="YYYY" min="1900" max="2025">
      <input id="dm" type="number" placeholder="MM" min="1" max="12">
      <input id="dd" type="number" placeholder="DD" min="1" max="31">
    </div>
    <label>Sex (for actuarial table)</label>
    <select id="sex">
      <option value="M">Male</option>
      <option value="F">Female</option>
    </select>
    <div class="help">Uses UK ONS National Life Tables 2020-2022 period life expectancy.</div>
  </div>

  <div class="card">
    <h2>Theme</h2>
    <div id="themes" class="theme-grid">Loading...</div>
    <input id="theme" type="hidden" value="0">
  </div>

  <div class="card">
    <h2>Timezone</h2>
    <select id="tz"></select>
    <div class="help">DST handled automatically via POSIX TZ string.</div>
  </div>

  <button onclick="save()">Save & Reboot</button>
  <div id="status"></div>
</div>

<script>
async function scan() {
  const list = document.getElementById('scanlist');
  list.style.display = 'block';
  list.innerHTML = '<div class="net">Scanning...</div>';
  try {
    const r = await fetch('/scan');
    const nets = await r.json();
    if (!nets.length) { list.innerHTML = '<div class="net">No networks found</div>'; return; }
    list.innerHTML = nets.map(n =>
      `<div class="net" onclick="document.getElementById('ssid').value='${n.ssid.replace(/'/g,"\\'")}'">
        <span>${n.ssid}</span><span class="rssi">${n.rssi} dBm${n.lock?' &#x1F512;':''}</span>
      </div>`
    ).join('');
  } catch(e) {
    list.innerHTML = '<div class="net">Scan failed</div>';
  }
}

async function loadThemes() {
  const grid = document.getElementById('themes');
  try {
    const r = await fetch('/themes');
    const themes = await r.json();
    grid.innerHTML = themes.map(t =>
      `<div class="theme-opt" data-idx="${t.idx}" onclick="selectTheme(${t.idx})">
         <div class="theme-swatch" style="background:${t.bg};color:${t.accent};">A</div>
         <div class="theme-name">${t.label}</div>
       </div>`
    ).join('');
    selectTheme(0);
  } catch(e) {
    grid.textContent = 'Failed to load themes';
  }
}

function selectTheme(idx) {
  document.getElementById('theme').value = idx;
  document.querySelectorAll('.theme-opt').forEach(el => {
    el.classList.toggle('sel', parseInt(el.dataset.idx) === idx);
  });
}

async function loadTimezones() {
  const sel = document.getElementById('tz');
  try {
    const r = await fetch('/timezones');
    const zones = await r.json();
    sel.innerHTML = zones.map(z =>
      `<option value="${z.idx}">${z.label}</option>`
    ).join('');
    sel.value = '1'; // default UK
  } catch(e) {
    sel.innerHTML = '<option value="1">UK (default)</option>';
  }
}

async function save() {
  const data = {
    ssid:  document.getElementById('ssid').value.trim(),
    pass:  document.getElementById('pass').value,
    dy:    parseInt(document.getElementById('dy').value),
    dm:    parseInt(document.getElementById('dm').value),
    dd:    parseInt(document.getElementById('dd').value),
    sex:   document.getElementById('sex').value,
    theme: parseInt(document.getElementById('theme').value),
    tz:    parseInt(document.getElementById('tz').value)
  };
  const status = document.getElementById('status');
  if (!data.ssid || !data.dy || !data.dm || !data.dd) {
    status.className = 'err'; status.textContent = 'Fill in all fields'; return;
  }
  try {
    const r = await fetch('/save', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(data)
    });
    if (r.ok) {
      status.className = 'ok';
      status.textContent = 'Saved. Rebooting in 3s...';
    } else {
      const t = await r.text();
      status.className = 'err'; status.textContent = 'Error: ' + t;
    }
  } catch(e) {
    status.className = 'err'; status.textContent = 'Request failed';
  }
}

// Pre-fill form from /api if reachable (works in remote/ops mode where the
// device already has saved settings). Captive setup mode returns blank.
async function prefill() {
  try {
    const r = await fetch('/api', { cache: 'no-store' });
    if (!r.ok) return;
    const d = await r.json();
    if (d.ssid)   document.getElementById('ssid').value = d.ssid;
    if (d.dob) {
      const [y,m,da] = d.dob.split('-');
      document.getElementById('dy').value = parseInt(y);
      document.getElementById('dm').value = parseInt(m);
      document.getElementById('dd').value = parseInt(da);
    }
    if (d.sex) document.getElementById('sex').value = d.sex;
    if (typeof d.tz === 'number') {
      const trySel = () => {
        const opt = document.querySelector(`#tz option[value="${d.tz}"]`);
        if (opt) document.getElementById('tz').value = d.tz;
        else setTimeout(trySel, 50);
      };
      trySel();
    }
    if (typeof d.theme === 'number') {
      // selectTheme runs after loadThemes finishes; retry until grid exists
      const trySel = () => {
        const opt = document.querySelector(`.theme-opt[data-idx="${d.theme}"]`);
        if (opt) selectTheme(d.theme);
        else setTimeout(trySel, 50);
      };
      trySel();
    }
    // Make password optional in remote mode (server keeps existing if blank)
    document.getElementById('pass').placeholder = 'Leave blank to keep existing';
  } catch(e) { /* captive mode — /api absent or timing out, fine */ }
}

// Auto-scan + load themes + load timezones + pre-fill on page load
window.addEventListener('load', () => {
  setTimeout(scan, 200);
  loadThemes();
  loadTimezones();
  prefill();
});
</script>
</body>
</html>
)HTML";
