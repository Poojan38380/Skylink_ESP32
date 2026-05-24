// Skylink GCS — application logic (see gcs_config.js for tunables)
'use strict';

const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};

let ws;
let reconnectDelay = CFG.wsReconnectInitialMs || 1500;
let countdownTimer;
let pingTimestamp = 0;
let connectedIP = location.hostname;

document.addEventListener('DOMContentLoaded', () => {
  const el = document.getElementById('proto-ver');
  if (el) el.textContent = String(CFG.protocolVersion || 1);
});

// ── CLOCK ──────────────────────────────────────────────────────────
function updateClock() {
  document.getElementById('sys-time').textContent = new Date().toLocaleTimeString('en-GB', { hour12: false });
}
setInterval(updateClock, 1000);
updateClock();

// ── UPTIME FORMAT ──────────────────────────────────────────────────
function fmtUptime(s) {
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60;
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(ss).padStart(2, '0')}`;
}

// ── SIGNAL CLASS ───────────────────────────────────────────────────
function signalClass(dbm) {
  if (dbm >= -55) return 's5';
  if (dbm >= -65) return 's4';
  if (dbm >= -75) return 's3';
  if (dbm >= -85) return 's2';
  return 's1';
}

// ── CONNECTION UI ──────────────────────────────────────────────────
function setLinkState(state) {
  const badge = document.getElementById('link-badge');
  const dot = document.getElementById('badge-dot');
  const lnkState = document.getElementById('lnk-state');
  const btns = ['btn-arm', 'btn-disarm', 'btn-liftoff', 'btn-land', 'btn-rtl', 'btn-set-mode', 'mode-select'];
  const overlay = document.getElementById('reconnect-overlay');

  if (state === 'connected') {
    badge.className = 'link-badge connected';
    document.getElementById('badge-text').textContent = 'LINK ACTIVE';
    dot.className = 'badge-dot pulse';
    lnkState.textContent = '● CONNECTED';
    lnkState.className = 'stat-value good';
    btns.forEach(id => document.getElementById(id).disabled = false);
    overlay.classList.remove('show');
  } else if (state === 'disconnected') {
    badge.className = 'link-badge disconnected';
    document.getElementById('badge-text').textContent = 'LINK LOST';
    dot.className = 'badge-dot';
    lnkState.textContent = '○ DISCONNECTED';
    lnkState.className = 'stat-value bad';
    btns.forEach(id => document.getElementById(id).disabled = true);
    overlay.classList.add('show');
  } else {
    badge.className = 'link-badge connecting';
    document.getElementById('badge-text').textContent = 'CONNECTING…';
    dot.className = 'badge-dot pulse';
    lnkState.textContent = '◌ CONNECTING';
    lnkState.className = 'stat-value warn';
  }
}

// ── LED UI ─────────────────────────────────────────────────────────
function setLED(on) {
  const orb = document.getElementById('led-orb');
  const label = document.getElementById('led-status-text');
  orb.className = 'led-orb' + (on ? ' on' : '');
  label.className = 'led-status-text' + (on ? ' on' : '');
  label.textContent = on ? 'ARMED / ON' : 'DISARMED / OFF';
}

// ── TELEMETRY UI ───────────────────────────────────────────────────
function updateTelemetry(d) {
  const alt = Number(d.altitude) || 0;
  const spd = Number(d.speed) || 0;
  const lat = Number(d.lat) || 0;
  const lng = Number(d.lng) || 0;
  const bat = Number(d.battery) || 0;
  const batV = Number(d.battery_v) || 0;
  const sats = Number(d.sats) || 0;

  document.getElementById('tl-alt').textContent = alt.toFixed(1);
  document.getElementById('tl-speed').textContent = spd.toFixed(1);
  document.getElementById('tl-lat').textContent = lat.toFixed(6) + '°N';
  document.getElementById('tl-lng').textContent = lng.toFixed(6) + '°E';
  document.getElementById('lnk-ip').textContent = connectedIP;
  document.getElementById('lnk-uptime').textContent = fmtUptime(d.uptime || 0);

  const sitlEl = document.getElementById('lnk-sitl');
  if (d.sitl_connected) {
    sitlEl.textContent = '● MAVLink OK';
    sitlEl.className = 'stat-value good';
  } else if (d.sitl_host_ready) {
        sitlEl.textContent = '◌ ' + (d.sitl_host || '?') + ':' + (d.sitl_port || CFG.sitlPortDefault || 5763);
    sitlEl.className = 'stat-value warn';
  } else {
    sitlEl.textContent = '○ OPEN DASHBOARD FIRST';
    sitlEl.className = 'stat-value bad';
  }

  // Arm Status
  const stateBadge = document.getElementById('lnk-state');
  if (d.armed) {
    stateBadge.textContent = "● ARMED / FLYING";
    stateBadge.className = "stat-value bad"; // Show red alarm when armed
    setLED(true);
  } else {
    stateBadge.textContent = "○ DISARMED / SAFE";
    stateBadge.className = "stat-value good";
    setLED(false);
  }

  // Battery
  document.getElementById('tl-bat').textContent = bat + "% (" + batV.toFixed(1) + "V)";
  const bar = document.getElementById('bat-bar');
  bar.style.width = bat + '%';
  bar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';

  // Signal Strength (Parse GPS Sats instead of RSSI to indicate localization quality)
  const sb = document.getElementById('signal-bars');
  if (sats > 9) sb.className = 'signal-bars s5';
  else if (sats > 6) sb.className = 'signal-bars s4';
  else if (sats > 3) sb.className = 'signal-bars s3';
  else sb.className = 'signal-bars s1';

  const sitlNote = d.sitl_connected ? 'MAVLink live' : 'SITL pending';
  document.getElementById('last-hb-text').textContent = sitlNote + ' | Sats: ' + sats;
}

// ── LOG ────────────────────────────────────────────────────────────
function log(tag, tagClass, msg) {
  const box = document.getElementById('log');
  const ts = new Date().toLocaleTimeString('en-GB', { hour12: false });
  const el = document.createElement('div');
  el.className = 'log-entry';
  el.innerHTML =
    `<span class="log-ts">${ts}</span>` +
    `<span class="log-tag ${tagClass}">${tag}</span>` +
    `<span class="log-msg">${msg}</span>`;
  box.prepend(el);
      const maxLog = CFG.commsLogMaxEntries || 40;
      while (box.children.length > maxLog) box.removeChild(box.lastChild);
}

// ── COMMANDS ───────────────────────────────────────────────────────
function sendCmd(command, extra) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
      const msg = { v: CFG.protocolVersion || 1, type: 'command', command, ...(extra || {}) };
  ws.send(JSON.stringify(msg));
  log('TX', 'tag-sys', command + (extra ? ' → ' + JSON.stringify(extra) : ''));
}

function sendPing() {
  pingTimestamp = performance.now();
  sendCmd('PING');
}

function applyFlightMode() {
  const mode = document.getElementById('mode-select').value;
  sendCmd('SET_FLIGHT_MODE', { mode });
}

function armSequence() {
  const mode = document.getElementById('mode-select').value;
  if (mode !== 'GUIDED') {
    log('SYS', 'tag-err', 'Select GUIDED mode before arming (STABILIZE will auto-disarm on ground)');
    document.getElementById('mode-select').value = 'GUIDED';
  }
  sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
      setTimeout(() => sendCmd('ARM_DRONE'), CFG.armModeDelayMs || 400);
}

function autonomousTakeoff() {
      const alt = prompt('Takeoff altitude (meters)?', String(CFG.defaultTakeoffAltM || 5));
      if (alt === null) return;
      const meters = parseFloat(alt);
      const altMin = CFG.takeoffAltMinM || 1;
      const altMax = CFG.takeoffAltMaxM || 50;
      if (isNaN(meters) || meters < altMin || meters > altMax) {
        log('ERR', 'tag-err', `Invalid altitude (use ${altMin}–${altMax}m)`);
        return;
      }
      sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
      setTimeout(() => {
        sendCmd('ARM_DRONE');
        setTimeout(() => sendCmd('TAKEOFF', { altitude: meters }), CFG.takeoffArmDelayMs || 500);
      }, CFG.armModeDelayMs || 400);
  log('SYS', 'tag-sys', 'Takeoff sequence: GUIDED → ARM → ' + meters + 'm');
}

// ── RECONNECT COUNTDOWN ────────────────────────────────────────────
function startCountdown(seconds) {
  clearInterval(countdownTimer);
  let t = seconds;
  document.getElementById('countdown').textContent = t;
  countdownTimer = setInterval(() => {
    t--;
    document.getElementById('countdown').textContent = t;
    if (t <= 0) clearInterval(countdownTimer);
  }, 1000);
}

// ── WEBSOCKET ──────────────────────────────────────────────────────
function connect() {
  setLinkState('connecting');
  const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
  ws = new WebSocket(wsProtocol + window.location.host + '/ws');

  ws.onopen = () => {
        reconnectDelay = CFG.wsReconnectInitialMs || 1500;
    setLinkState('connected');
    log('SYS', 'tag-sys', 'WebSocket link established → ws://' + location.host + '/ws');
  };

  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      switch (d.event) {
        case 'HEARTBEAT':
          updateTelemetry(d);
          break;
        case 'LED_STATE':
          setLED(d.value);
          log('LED', 'tag-led', 'State changed → ' + (d.value ? 'ON ✔' : 'OFF ✖'));
          break;
        case 'PONG': {
          const latency = pingTimestamp ? Math.round(performance.now() - pingTimestamp) : '?';
          log('PING', 'tag-ping', `PONG ← round-trip ${latency}ms`);
          break;
        }
        case 'ERROR':
          log('ERR', 'tag-err', d.message || 'Unknown error from device');
          break;
        default:
          log('RX', 'tag-sys', JSON.stringify(d));
      }
    } catch (err) {
      log('ERR', 'tag-err', 'Bad JSON from device: ' + e.data);
    }
  };

  ws.onclose = () => {
    setLinkState('disconnected');
    const delay = reconnectDelay;
    log('SYS', 'tag-err', `Link closed. Reconnecting in ${Math.round(delay / 1000)}s…`);
    startCountdown(Math.round(delay / 1000));
    setTimeout(connect, delay);
        reconnectDelay = Math.min(reconnectDelay * 1.5, CFG.wsReconnectMaxMs || 20000);
  };

  ws.onerror = () => {
    log('ERR', 'tag-err', 'WebSocket transport error — check device power and network');
  };
}

connect();
