// Skylink GCS — application logic (see gcs_config.js)
'use strict';

const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};

let ws;
let reconnectDelay = CFG.wsReconnectInitialMs || 1500;
let countdownTimer;
let pingTimestamp = 0;
let connectedIP = location.hostname;
let selectedMoveM = 1;
let wsConnected = false;
function updateBuildTag(d) {
  const el = document.getElementById('build-tag');
  if (!el) return;
  const fw = d && d.fw_build != null ? d.fw_build : '?';
  const fs = d && d.fs_build != null ? d.fs_build : '?';
  const fsExp = d && d.fs_build_expected != null ? d.fs_build_expected : (CFG.fsBuild || '?');
  let text = 'FW ' + fw + ' · FS ' + fs;
  if (d && d.fs_build_ok === false) {
    text += ' (expected ' + fsExp + ' — run uploadfs)';
    el.className = 'build-mismatch';
  } else {
    el.className = '';
  }
  el.textContent = text;
}

function initTabs() {
  const buttons = document.querySelectorAll('.tab-btn');
  const panels = document.querySelectorAll('.tab-panel');

  function showTab(name) {
    buttons.forEach((btn) => {
      const on = btn.dataset.tab === name;
      btn.classList.toggle('active', on);
      btn.setAttribute('aria-selected', on ? 'true' : 'false');
    });
    panels.forEach((panel) => {
      const on = panel.id === 'tab-' + name;
      panel.classList.toggle('active', on);
      panel.hidden = !on;
    });
    if (name === 'map' && typeof SkylinkMap !== 'undefined' && window.L) {
      setTimeout(() => window.dispatchEvent(new Event('resize')), 120);
    }
  }

  buttons.forEach((btn) => {
    btn.addEventListener('click', () => showTab(btn.dataset.tab));
  });

  const saved = CFG.defaultTab || 'map';
  showTab(saved);
}

document.addEventListener('DOMContentLoaded', () => {
  const simBanner = document.getElementById('sim-banner');
  if (simBanner && CFG.simulationBanner) simBanner.hidden = false;
  updateBuildTag(null);
  initTabs();
  initMoveControls();
});

function initMoveControls() {
  document.querySelectorAll('.preset-btn[data-move-m]').forEach((btn) => {
    btn.addEventListener('click', () => {
      selectedMoveM = parseFloat(btn.dataset.moveM) || 1;
      document.querySelectorAll('.preset-btn[data-move-m]').forEach((b) => {
        b.classList.toggle('active', b === btn);
      });
    });
  });

  document.querySelectorAll('.move-dir').forEach((btn) => {
    btn.addEventListener('click', () => {
      const axis = btn.dataset.move;
      const sign = parseFloat(btn.dataset.sign) || 1;
      sendMoveBody(axis, sign * selectedMoveM);
    });
  });

  document.querySelectorAll('.btn-yaw').forEach((btn) => {
    btn.addEventListener('click', () => {
      const deg = parseFloat(btn.dataset.yaw) || 0;
      sendYawRelative(deg);
    });
  });
}

function updateMoveControls(d) {
  const minAgl = 2;
  const alt = Number(d.relative_alt) || Number(d.altitude) || 0;
  const canMove =
    wsConnected &&
    d.armed === true &&
    (d.flight_mode_name === 'GUIDED' || d.flight_mode === 4) &&
    (Number(d.gps_fix) || 0) >= (CFG.preflightMinGpsFix || 3) &&
    alt >= minAgl;

  document.querySelectorAll('.move-dir, .btn-yaw').forEach((el) => {
    el.disabled = !canMove;
  });

  const hint = document.getElementById('move-hint');
  if (hint) {
    if (canMove) {
      hint.textContent = 'Ready — ' + selectedMoveM + ' m steps (body frame, nose = forward).';
      hint.className = 'move-hint ready';
    } else if (!d.armed) {
      hint.textContent = 'Arm in GUIDED after preflight checks to enable moves.';
      hint.className = 'move-hint';
    } else if (d.flight_mode_name !== 'GUIDED' && d.flight_mode !== 4) {
      hint.textContent = 'Switch to GUIDED mode to use relative moves.';
      hint.className = 'move-hint';
    } else {
      hint.textContent = 'Need 3D GPS fix and at least ' + minAgl + ' m altitude.';
      hint.className = 'move-hint';
    }
  }
}

function sendMoveBody(axis, meters) {
  const m = Math.abs(meters);
  const confirmAbove = CFG.moveConfirmAboveM || 10;
  if (m > confirmAbove && !confirm('Move ' + m + ' meters?')) return;

  const payload = { x: 0, y: 0, z: 0 };
  if (axis === 'x') payload.x = meters;
  else if (axis === 'y') payload.y = meters;
  else if (axis === 'z') payload.z = meters;

  sendCmd('MOVE_BODY', payload);
  log('SYS', 'tag-sys', 'MOVE_BODY ' + JSON.stringify(payload));
}

function sendYawRelative(deg) {
  const abs = Math.abs(deg);
  const confirmAbove = CFG.moveConfirmAboveM || 10;
  if (abs > confirmAbove && !confirm('Yaw ' + abs + '°?')) return;
  sendCmd('YAW_RELATIVE', { deg });
  log('SYS', 'tag-sys', 'YAW_RELATIVE ' + deg + '°');
}

function updateClock() {
  const el = document.getElementById('sys-time');
  if (el) el.textContent = new Date().toLocaleTimeString('en-GB', { hour12: false });
}
setInterval(updateClock, 1000);
updateClock();

function fmtUptime(s) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const ss = s % 60;
  return String(h).padStart(2, '0') + ':' + String(m).padStart(2, '0') + ':' + String(ss).padStart(2, '0');
}

function signalClass(dbm) {
  if (dbm >= -55) return 's5';
  if (dbm >= -65) return 's4';
  if (dbm >= -75) return 's3';
  if (dbm >= -85) return 's2';
  return 's1';
}

function setLinkState(state) {
  const badge = document.getElementById('link-badge');
  const dot = document.getElementById('badge-dot');
  const badgeText = document.getElementById('badge-text');
  const lnkState = document.getElementById('lnk-state');
  const btns = [
    'btn-arm', 'btn-disarm', 'btn-liftoff', 'btn-land', 'btn-rtl', 'btn-set-mode', 'mode-select',
  ];
  const moveBtns = document.querySelectorAll('.move-dir, .btn-yaw');
  const overlay = document.getElementById('reconnect-overlay');

  if (state === 'connected') {
    wsConnected = true;
    if (badge) badge.className = 'link-badge connected';
    if (badgeText) badgeText.textContent = 'Connected';
    if (dot) dot.className = 'badge-dot pulse';
    if (lnkState) lnkState.textContent = 'Connected';
    btns.forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.disabled = false;
    });
    if (overlay) overlay.classList.remove('show');
  } else if (state === 'disconnected') {
    wsConnected = false;
    if (badge) badge.className = 'link-badge disconnected';
    if (badgeText) badgeText.textContent = 'Disconnected';
    if (dot) dot.className = 'badge-dot';
    if (lnkState) lnkState.textContent = 'Disconnected';
    btns.forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.disabled = true;
    });
    moveBtns.forEach((el) => { el.disabled = true; });
    if (overlay) overlay.classList.add('show');
  } else {
    wsConnected = false;
    if (badge) badge.className = 'link-badge connecting';
    if (badgeText) badgeText.textContent = 'Connecting…';
    if (dot) dot.className = 'badge-dot pulse';
    if (lnkState) lnkState.textContent = 'Connecting…';
  }
}

function setPreflightItem(id, pass) {
  const el = document.getElementById(id);
  if (!el) return;
  el.className = pass ? 'pass' : 'fail';
  const icon = el.querySelector('.pf-icon');
  if (icon) icon.textContent = pass ? '●' : '○';
}

function escapeHtml(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

function updatePreflight(d) {
  const wifiOk = d.wifi_connected === true;
  const minFix = CFG.preflightMinGpsFix || 3;
  const minBat = CFG.preflightMinBatteryPct || 20;
  const gpsOk = (Number(d.gps_fix) || 0) >= minFix;
  const mavOk = d.mav_connected === true;
  const bat = Number(d.battery);
  const batOk = bat < 0 || bat > minBat;
  const safeOk = !d.armed;

  setPreflightItem('pf-wifi', wifiOk);
  setPreflightItem('pf-gps', gpsOk);
  setPreflightItem('pf-mav', mavOk);
  setPreflightItem('pf-bat', batOk);
  setPreflightItem('pf-safe', safeOk);

  const ready = wifiOk && gpsOk && mavOk && batOk && safeOk;
  const summary = document.getElementById('preflight-summary');
  if (summary) {
    if (ready) {
      summary.textContent = 'All checks passed — you may arm from the Fly tab.';
      summary.className = 'fly-ready-banner ready';
    } else if (!mavOk) {
      summary.textContent = 'Waiting for MAVLink — open dashboard while SITL is running.';
      summary.className = 'fly-ready-banner bad';
    } else {
      summary.textContent = 'Not ready to arm — review the Status tab checklist.';
      summary.className = 'fly-ready-banner warn';
    }
  }

  const msgs = document.getElementById('preflight-msgs');
  if (msgs) {
    if (Array.isArray(d.statustext) && d.statustext.length) {
      msgs.innerHTML = d.statustext.slice(-8).map((line) => {
        const err = /denied|fail|error|prearm|reject/i.test(line);
        return '<div class="fc-msg' + (err ? ' err' : '') + '">' + escapeHtml(line) + '</div>';
      }).join('');
    } else if (!msgs.querySelector('.fc-msg')) {
      msgs.innerHTML = '<div class="fc-msg-empty">No messages yet.</div>';
    }
  }
}

function updateAttitude(d) {
  const roll = Number(d.roll) || 0;
  const pitch = Number(d.pitch) || 0;
  const horizon = document.getElementById('att-horizon');
  const rollEl = document.getElementById('att-roll');
  const pitchEl = document.getElementById('att-pitch');
  const bubble = document.getElementById('att-bubble');

  if (horizon) {
    horizon.style.transform = 'translateY(-50%) rotate(' + (-roll).toFixed(1) + 'deg)';
    horizon.style.top = (50 + pitch * 1.2) + '%';
  }
  if (bubble) bubble.style.transform = 'rotate(' + roll.toFixed(1) + 'deg)';
  if (rollEl) rollEl.textContent = 'Roll ' + roll.toFixed(0) + '°';
  if (pitchEl) pitchEl.textContent = 'Pitch ' + pitch.toFixed(0) + '°';
}

function updateLiveStrip(d) {
  const mode = d.flight_mode_name || '—';
  const modeEl = document.getElementById('live-mode');
  if (modeEl) modeEl.textContent = mode;

  const armEl = document.getElementById('live-arm');
  if (armEl) {
    armEl.textContent = d.armed ? 'ARMED' : 'DISARMED';
    armEl.classList.toggle('armed', !!d.armed);
  }

  const altEl = document.getElementById('live-alt');
  if (altEl) altEl.textContent = 'ALT ' + (Number(d.altitude) || 0).toFixed(1) + ' m';

  const spdEl = document.getElementById('live-spd');
  if (spdEl) spdEl.textContent = 'SPD ' + (Number(d.speed) || 0).toFixed(1) + ' m/s';
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
  setChip(
    document.getElementById('chip-sitl'),
    tcpOk ? 'ok' : (d.sitl_host_ready ? 'warn' : 'bad'),
    tcpOk ? 'SITL ●' : (d.sitl_host_ready ? 'SITL ◌' : 'SITL ○')
  );

  const mavOk = d.mav_connected === true;
  setChip(document.getElementById('chip-mav'), mavOk ? 'ok' : 'bad', mavOk ? 'MAV ●' : 'MAV ○');

  const wifiOk = d.wifi_connected === true;
  const rssi = Number(d.wifi_rssi) || 0;
  setChip(document.getElementById('chip-wifi'), wifiOk ? 'ok' : 'bad', wifiOk ? ('WiFi ' + rssi) : 'WiFi');

  if (d.simulation) {
    const banner = document.getElementById('sim-banner');
    if (banner) banner.hidden = false;
  }
}

function updateTelemetry(d) {
  const alt = Number(d.altitude) || 0;
  const spd = Number(d.speed) || 0;
  const lat = Number(d.lat) || 0;
  const lng = Number(d.lng) || 0;
  const bat = Number(d.battery) || 0;
  const batV = Number(d.battery_v) || 0;
  const sats = Number(d.sats) || 0;
  const yaw = Number(d.yaw);

  const set = (id, text) => {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  };

  set('tl-alt', alt.toFixed(1));
  set('tl-speed', spd.toFixed(1));
  set('tl-lat', lat.toFixed(6));
  set('tl-lng', lng.toFixed(6));
  set('tl-hdg', Number.isFinite(yaw) ? String(Math.round(yaw % 360)) : '—');
  set('lnk-ip', connectedIP);
  set('lnk-uptime', fmtUptime(d.uptime || 0));
  set('tl-mode', d.flight_mode_name || String(d.flight_mode ?? '—'));
  set('tl-bat', bat + '% · ' + batV.toFixed(1) + ' V');

  const sitlEl = document.getElementById('lnk-sitl');
  if (sitlEl) {
    const port = d.sitl_port || CFG.sitlPortDefault || 5763;
    const host = d.sitl_host || '?';
    if (d.mav_connected) {
      sitlEl.textContent = 'MAVLink OK · ' + host + ':' + port;
    } else if (d.sitl_tcp_connected) {
      sitlEl.textContent = 'TCP only · ' + host + ':' + port;
    } else if (d.sitl_host_ready) {
      sitlEl.textContent = 'Connecting · ' + host + ':' + port;
    } else {
      sitlEl.textContent = 'Open this page to start SITL link';
    }
  }

  const stateEl = document.getElementById('lnk-state');
  if (stateEl) stateEl.textContent = d.armed ? 'Armed' : 'Disarmed';

  const bar = document.getElementById('bat-bar');
  if (bar) {
    bar.style.width = Math.min(100, Math.max(0, bat)) + '%';
    bar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';
  }

  const sb = document.getElementById('signal-bars');
  if (sb) {
    if (d.wifi_connected && d.wifi_rssi) {
      sb.className = 'signal-bars ' + signalClass(Number(d.wifi_rssi));
    } else if (sats > 9) sb.className = 'signal-bars s5';
    else if (sats > 6) sb.className = 'signal-bars s4';
    else if (sats > 3) sb.className = 'signal-bars s3';
    else sb.className = 'signal-bars s1';
  }

  set('last-hb-text', (d.sitl_connected ? 'MAVLink live' : 'Waiting') + ' · ' + sats + ' sats');

  updateLinkChips(d);
  updatePreflight(d);
  updateAttitude(d);
  updateLiveStrip(d);

  updateMoveControls(d);

  if (typeof SkylinkMap !== 'undefined') SkylinkMap.updateFromTelemetry(d);
}

function log(tag, tagClass, msg) {
  const box = document.getElementById('log');
  if (!box) return;
  const ts = new Date().toLocaleTimeString('en-GB', { hour12: false });
  const el = document.createElement('div');
  el.className = 'log-entry';
  el.innerHTML =
    '<span class="log-ts">' + ts + '</span>' +
    '<span class="log-tag ' + tagClass + '">' + tag + '</span>' +
    '<span class="log-msg">' + escapeHtml(msg) + '</span>';
  box.prepend(el);
  const maxLog = CFG.commsLogMaxEntries || 40;
  while (box.children.length > maxLog) box.removeChild(box.lastChild);
}

function sendCmd(command, extra) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  const msg = { v: CFG.protocolVersion || 1, type: 'command', command, ...(extra || {}) };
  ws.send(JSON.stringify(msg));
  log('TX', 'tag-sys', command + (extra ? ' → ' + JSON.stringify(extra) : ''));
}

function applyFlightMode() {
  sendCmd('SET_FLIGHT_MODE', { mode: document.getElementById('mode-select').value });
}

function armSequence() {
  const modeSelect = document.getElementById('mode-select');
  if (modeSelect && modeSelect.value !== 'GUIDED') {
    log('SYS', 'tag-err', 'GUIDED mode required before arming');
    modeSelect.value = 'GUIDED';
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
    log('ERR', 'tag-err', 'Altitude must be between ' + altMin + ' and ' + altMax + ' m');
    return;
  }
  sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
  setTimeout(() => {
    sendCmd('ARM_DRONE');
    setTimeout(() => sendCmd('TAKEOFF', { altitude: meters }), CFG.takeoffArmDelayMs || 500);
  }, CFG.armModeDelayMs || 400);
  log('SYS', 'tag-sys', 'Takeoff: GUIDED → arm → ' + meters + ' m');
}

function startCountdown(seconds) {
  clearInterval(countdownTimer);
  let t = seconds;
  const el = document.getElementById('countdown');
  if (el) el.textContent = t;
  countdownTimer = setInterval(() => {
    t--;
    if (el) el.textContent = t;
    if (t <= 0) clearInterval(countdownTimer);
  }, 1000);
}

function connect() {
  setLinkState('connecting');
  const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
  ws = new WebSocket(wsProtocol + window.location.host + '/ws');

  ws.onopen = () => {
    reconnectDelay = CFG.wsReconnectInitialMs || 1500;
    setLinkState('connected');
    log('SYS', 'tag-sys', 'Connected to ' + location.host);
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
        case 'ACK': {
          const ok = d.ok === true || d.result === 0;
          const label = d.result_name || ('code ' + d.result);
          log(ok ? 'ACK' : 'ERR', ok ? 'tag-sys' : 'tag-err', 'Command ' + (d.command ?? '?') + ': ' + label);
          break;
        }
        case 'STATUSTEXT': {
          const txt = d.text || '';
          if (txt) {
            const isErr = (Number(d.severity) || 0) >= 4 || /denied|fail|error|prearm|reject/i.test(txt);
            log('FC', isErr ? 'tag-err' : 'tag-sys', txt);
          }
          break;
        }
        case 'PONG': {
          const latency = pingTimestamp ? Math.round(performance.now() - pingTimestamp) : '?';
          log('PING', 'tag-ping', 'Round-trip ' + latency + ' ms');
          break;
        }
        case 'ERROR':
          log('ERR', 'tag-err', d.message || 'Unknown error');
          break;
        default:
          break;
      }
    } catch (err) {
      log('ERR', 'tag-err', 'Invalid message from device');
    }
  };

  ws.onclose = () => {
    setLinkState('disconnected');
    const delay = reconnectDelay;
    log('SYS', 'tag-err', 'Disconnected — retry in ' + Math.round(delay / 1000) + ' s');
    startCountdown(Math.round(delay / 1000));
    setTimeout(connect, delay);
    reconnectDelay = Math.min(reconnectDelay * 1.5, CFG.wsReconnectMaxMs || 20000);
  };

  ws.onerror = () => {
    log('ERR', 'tag-err', 'WebSocket error — check ESP32 power and Wi‑Fi');
  };
}

connect();
