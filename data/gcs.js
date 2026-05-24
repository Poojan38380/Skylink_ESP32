// Skylink GCS — application logic (see gcs_config.js for tunables)
'use strict';

const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};

let ws;
let reconnectDelay = CFG.wsReconnectInitialMs || 1500;
let countdownTimer;
let pingTimestamp = 0;
let connectedIP = location.hostname;

function updateBuildTag(d) {
  const el = document.getElementById('build-tag');
  if (!el) return;
  const fw = d && d.fw_build != null ? d.fw_build : '?';
  const fs = d && d.fs_build != null ? d.fs_build : '?';
  const fsExp = d && d.fs_build_expected != null ? d.fs_build_expected : (CFG.fsBuild || '?');
  let text = 'FW ' + fw + ' · FS ' + fs;
  if (d && d.fs_build_ok === false) {
    text += ' (expected ' + fsExp + ' — uploadfs?)';
    el.className = 'build-mismatch';
  } else {
    el.className = '';
  }
  el.textContent = text;
}

document.addEventListener('DOMContentLoaded', () => {
  const simBanner = document.getElementById('sim-banner');
  if (simBanner && CFG.simulationBanner) simBanner.hidden = false;
  updateBuildTag(null);

  const logToggle = document.getElementById('log-toggle');
  const logPanel = document.getElementById('log-panel');
  const logIcon = document.getElementById('log-collapse-icon');
  if (logToggle && logPanel) {
    const toggleLog = () => {
      const collapsed = logPanel.classList.toggle('collapsed');
      logToggle.setAttribute('aria-expanded', collapsed ? 'false' : 'true');
      if (logIcon) logIcon.textContent = collapsed ? '▲' : '▼';
      if (typeof SkylinkMap !== 'undefined' && window.L) {
        setTimeout(() => window.dispatchEvent(new Event('resize')), 300);
      }
    };
    logToggle.addEventListener('click', toggleLog);
    logToggle.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        toggleLog();
      }
    });
  }
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

function setChip(el, state, text) {
  if (!el) return;
  el.className = 'link-chip ' + state;
  el.textContent = text;
}

function updateLinkChips(d) {
  const wsOk = d.ws_connected === true;
  setChip(document.getElementById('chip-ws'), wsOk ? 'ok' : 'warn', wsOk ? 'WS ●' : 'WS ○');

  const tcpOk = d.sitl_tcp_connected === true;
  setChip(document.getElementById('chip-sitl'), tcpOk ? 'ok' : (d.sitl_host_ready ? 'warn' : 'bad'),
    tcpOk ? 'SITL ●' : (d.sitl_host_ready ? 'SITL ◌' : 'SITL ○'));

  const mavOk = d.mav_connected === true;
  setChip(document.getElementById('chip-mav'), mavOk ? 'ok' : 'bad', mavOk ? 'MAV ●' : 'MAV ○');

  const wifiOk = d.wifi_connected === true;
  const rssi = Number(d.wifi_rssi) || 0;
  const wifiText = wifiOk ? ('WiFi ' + rssi + ' dBm') : 'WiFi —';
  setChip(document.getElementById('chip-wifi'), wifiOk ? 'ok' : 'bad', wifiText);

  if (d.simulation && document.getElementById('sim-banner')) {
    document.getElementById('sim-banner').hidden = false;
  }
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
  const yaw = Number(d.yaw);

  document.getElementById('tl-alt').textContent = alt.toFixed(1);
  document.getElementById('tl-speed').textContent = spd.toFixed(1);
  document.getElementById('tl-lat').textContent = lat.toFixed(6);
  document.getElementById('tl-lng').textContent = lng.toFixed(6);
  const hdgEl = document.getElementById('tl-hdg');
  if (hdgEl) hdgEl.textContent = Number.isFinite(yaw) ? String(Math.round(yaw % 360)) : '—';
  document.getElementById('lnk-ip').textContent = connectedIP;
  document.getElementById('lnk-uptime').textContent = fmtUptime(d.uptime || 0);

  const sitlEl = document.getElementById('lnk-sitl');
  if (d.mav_connected) {
    sitlEl.textContent = '● MAVLink · ' + (d.sitl_host || '?') + ':' + (d.sitl_port || CFG.sitlPortDefault || 5763);
    sitlEl.className = 'stat-value good';
  } else if (d.sitl_tcp_connected) {
    sitlEl.textContent = '◌ TCP only · ' + (d.sitl_host || '?') + ':' + (d.sitl_port || CFG.sitlPortDefault || 5763);
    sitlEl.className = 'stat-value warn';
  } else if (d.sitl_host_ready) {
    sitlEl.textContent = '◌ connecting ' + (d.sitl_host || '?') + ':' + (d.sitl_port || CFG.sitlPortDefault || 5763);
    sitlEl.className = 'stat-value warn';
  } else {
    sitlEl.textContent = '○ OPEN DASHBOARD FIRST';
    sitlEl.className = 'stat-value bad';
  }

  const stateBadge = document.getElementById('lnk-state');
  if (d.armed) {
    stateBadge.textContent = '● ARMED / FLYING';
    stateBadge.className = 'stat-value bad';
  } else {
    stateBadge.textContent = '○ DISARMED / SAFE';
    stateBadge.className = 'stat-value good';
  }

  updateLinkChips(d);

  document.getElementById('tl-bat').textContent = bat + '% · ' + batV.toFixed(1) + 'V';
  const bar = document.getElementById('bat-bar');
  bar.style.width = bat + '%';
  bar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';

  const sb = document.getElementById('signal-bars');
  if (d.wifi_connected && d.wifi_rssi) {
    sb.className = 'signal-bars ' + signalClass(Number(d.wifi_rssi));
  } else if (sats > 9) sb.className = 'signal-bars s5';
  else if (sats > 6) sb.className = 'signal-bars s4';
  else if (sats > 3) sb.className = 'signal-bars s3';
  else sb.className = 'signal-bars s1';

  const sitlNote = d.sitl_connected ? 'MAVLink' : 'SITL pending';
  document.getElementById('last-hb-text').textContent = sitlNote + ' · ' + sats + ' sats';

  if (typeof SkylinkMap !== 'undefined') SkylinkMap.updateFromTelemetry(d);
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
              updateBuildTag(d);
              break;
        case 'LED_STATE':
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
