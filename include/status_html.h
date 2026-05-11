// status_html.h - Live status page served at "/" in remote mode.
// Polls /api once per second and renders the same data the device shows.
#pragma once

const char STATUS_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Memento Mori</title>
<style>
  :root {
    --bg: #0a0a0b;
    --panel: #15161a;
    --border: #2a2c33;
    --text: #e8e8ea;
    --muted: #888a92;
    --accent: #00d563;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 24px;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
    transition: background 0.4s, color 0.4s;
  }
  .wrap { max-width: 560px; margin: 0 auto; }
  .head {
    display: flex; justify-content: space-between; align-items: baseline;
    margin-bottom: 4px;
  }
  h1 {
    font-size: 14px; font-weight: 600;
    text-transform: uppercase; letter-spacing: 0.12em;
    color: var(--accent);
    margin: 0;
  }
  .meta { color: var(--muted); font-size: 12px; }
  .divider {
    height: 1px; background: var(--accent); opacity: 0.7;
    margin: 8px 0 24px;
  }
  .hero {
    text-align: center;
    padding: 24px 0 16px;
  }
  .years {
    font-size: 96px; font-weight: 700;
    color: var(--accent);
    line-height: 1;
    letter-spacing: -0.02em;
    font-variant-numeric: tabular-nums;
  }
  .sub {
    color: var(--muted); font-size: 14px;
    margin-top: 8px;
  }
  .clock {
    text-align: center;
    font-size: 42px; font-weight: 600;
    margin: 16px 0 24px;
    font-variant-numeric: tabular-nums;
    letter-spacing: 0.02em;
  }
  .clock .ss { color: var(--accent); transition: color 0.2s; }
  .bar {
    display: grid;
    grid-template-columns: repeat(30, 1fr);
    gap: 2px;
    margin: 16px 0 8px;
  }
  .seg {
    height: 16px; border-radius: 2px;
    background: var(--panel);
    transition: background 0.2s;
  }
  .seg.on { background: var(--accent); }
  .pct { text-align: center; color: var(--muted); font-size: 13px; }
  .quote {
    text-align: center;
    color: var(--accent);
    margin: 24px 0 8px;
    font-size: 15px;
    letter-spacing: 0.05em;
  }
  .footer {
    display: flex; justify-content: space-between;
    color: var(--muted); font-size: 12px;
    margin-top: 24px;
    padding-top: 16px;
    border-top: 1px solid var(--border);
  }
  .footer a {
    color: var(--accent); text-decoration: none;
  }
  .footer a:hover { text-decoration: underline; }
  .offline { color: #ff4d4d; font-size: 12px; text-align: center; }
</style>
</head>
<body>
<div class="wrap">
  <div class="head">
    <h1 id="title">[ MEMENTO MORI ]</h1>
    <div class="meta" id="meta">--</div>
  </div>
  <div class="divider"></div>

  <div class="hero">
    <div class="years" id="years">--</div>
    <div class="sub" id="sub">years remaining</div>
  </div>

  <div class="clock"><span id="hm">--:--:</span><span class="ss" id="ss">--</span></div>

  <div class="bar" id="bar"></div>
  <div class="pct" id="pct">--% lived</div>

  <div class="quote">- Make Every Day Count -</div>

  <div class="footer">
    <span id="info">--</span>
    <a href="/config">Reconfigure &rarr;</a>
  </div>
  <div id="err" class="offline"></div>
</div>

<script>
const bar = document.getElementById('bar');
for (let i = 0; i < 30; i++) {
  const s = document.createElement('div');
  s.className = 'seg';
  bar.appendChild(s);
}

let lastSecs = -1;

async function poll() {
  try {
    const r = await fetch('/api', { cache: 'no-store' });
    if (!r.ok) throw new Error('http ' + r.status);
    const d = await r.json();

    document.getElementById('err').textContent = '';

    // Apply theme colors
    if (d.colors) {
      document.documentElement.style.setProperty('--bg', d.colors.bg);
      document.documentElement.style.setProperty('--accent', d.colors.accent);
      document.documentElement.style.setProperty('--text', d.colors.text);
    }

    document.getElementById('title').textContent =
      d.mode === 0 ? '[ MEMENTO MORI ]' : '[ TIME LIVED ]';
    document.getElementById('meta').textContent =
      `${d.sex}  ${d.dob}`;
    document.getElementById('years').textContent = d.years;
    document.getElementById('sub').textContent =
      `${d.days} days ${d.mode === 0 ? 'remaining' : 'lived'}`;
    document.getElementById('hm').textContent =
      String(d.hrs).padStart(2,'0') + ':' +
      String(d.mins).padStart(2,'0') + ':';
    document.getElementById('ss').textContent =
      String(d.secs).padStart(2,'0');
    document.getElementById('ss').style.color =
      (d.secs & 1) ? d.colors.accent : d.colors.text;

    const filled = Math.round(d.pct * 30);
    [...bar.children].forEach((seg, i) => {
      seg.classList.toggle('on', i < filled);
    });
    document.getElementById('pct').textContent =
      (d.pct * 100).toFixed(2) + '% lived';

    document.getElementById('info').textContent =
      `${d.hostname}.local   |   ${d.ip}   |   ${d.theme_label}`;

    lastSecs = d.secs;
  } catch (e) {
    document.getElementById('err').textContent = 'Connection lost: ' + e.message;
  }
}

poll();
setInterval(poll, 1000);
</script>
</body>
</html>
)HTML";
