// Skylink GCS — application logic (see gcs_config.js)
'use strict';

const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};

let ws;
let reconnectDelay = CFG.wsReconnectInitialMs || 1500;
let countdownTimer;
let pingTimestamp = 0;
let connectedIP = location.hostname;
let selectedMoveM = 1;
let usingCustomDist = false;
let wsConnected = false;
let moveControlsEnabled = false;
let lastKeyMoveMs = 0;
let lastStatusSnapshot = '';
let pendingGoto = null;
let lastHbForGoto = null;
let lastCommandGate = { ready: false, reason: 'Waiting for heartbeat', heartbeatMs: 0, commandPending: false, commandName: '' };
let lastFlightCommandTxMs = 0;
let activeTab = 'map';
let _armInProgress = false;
let _takeoffInProgress = false;
let lastLoggedState = { armed: null, mode: null, gps: null, connected: null, commandKey: null };
let emergencyConfirmTimeout = null;
let lastEscapePressMs = 0;
let ackWaiters = [];

const MAV_CMD = {
  DO_SET_MODE: 176,
  COMPONENT_ARM_DISARM: 400,
  NAV_TAKEOFF: 22,
};

const flightUiState = {
  armed: false,
  guided: false,
  pendingKey: '',
  stableCount: 0,
};
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
    activeTab = name;
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
  initKeyboardControls();

  // Sync mode selectors
  const modeSelect = document.getElementById('mode-select');
  const qcModeSelect = document.getElementById('qc-mode-select');
  if (modeSelect && qcModeSelect) {
    modeSelect.addEventListener('change', (e) => { qcModeSelect.value = e.target.value; });
    qcModeSelect.addEventListener('change', (e) => { modeSelect.value = e.target.value; });
  }
});

function updateDistanceLabel() {
  const el = document.getElementById('distance-active-label');
  if (!el) return;
  const text = usingCustomDist
    ? ('Active: ' + selectedMoveM + ' m (custom)')
    : ('Active: ' + selectedMoveM + ' m');
  el.textContent = text;
}

function selectPresetDistance(m) {
  selectedMoveM = m;
  usingCustomDist = false;
  const input = document.getElementById('move-dist-custom');
  if (input) input.classList.remove('active-source');
  document.querySelectorAll('.preset-btn[data-move-m]').forEach((b) => {
    b.classList.toggle('active', parseFloat(b.dataset.moveM) === m);
  });
  updateDistanceLabel();
}

function moveDistanceLimits() {
  return { min: CFG.moveMinM || 0.5, max: CFG.moveMaxM || 200 };
}

/** Read custom field if it has a valid number (no separate "Use" required). */
function readCustomDistanceFromInput() {
  const input = document.getElementById('move-dist-custom');
  if (!input || String(input.value).trim() === '') return null;
  const { min, max } = moveDistanceLimits();
  const v = parseFloat(input.value);
  if (isNaN(v) || v < min || v > max) return null;
  return v;
}

function applyCustomDistance(silent) {
  const input = document.getElementById('move-dist-custom');
  if (!input) return false;
  const { min, max } = moveDistanceLimits();
  const v = parseFloat(input.value);
  if (isNaN(v) || v < min || v > max) {
    if (!silent) {
      log('ERR', 'tag-err', 'Distance must be between ' + min + ' and ' + max + ' m');
    }
    return false;
  }
  selectedMoveM = v;
  usingCustomDist = true;
  input.classList.add('active-source');
  document.querySelectorAll('.preset-btn[data-move-m]').forEach((b) => b.classList.remove('active'));
  updateDistanceLabel();
  return true;
}

function getMoveDistanceM() {
  const custom = readCustomDistanceFromInput();
  if (custom != null) {
    if (!usingCustomDist || custom !== selectedMoveM) {
      selectedMoveM = custom;
      usingCustomDist = true;
      const input = document.getElementById('move-dist-custom');
      if (input) input.classList.add('active-source');
      document.querySelectorAll('.preset-btn[data-move-m]').forEach((b) => b.classList.remove('active'));
      updateDistanceLabel();
    }
    return custom;
  }
  return selectedMoveM;
}

function initMoveControls() {
  document.querySelectorAll('.preset-btn[data-move-m]').forEach((btn) => {
    btn.addEventListener('click', () => {
      selectPresetDistance(parseFloat(btn.dataset.moveM) || 1);
    });
  });

  const applyBtn = document.getElementById('btn-apply-custom-dist');
  const customInput = document.getElementById('move-dist-custom');
  if (applyBtn) applyBtn.addEventListener('click', () => applyCustomDistance(false));
  if (customInput) {
    customInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.preventDefault();
        applyCustomDistance(false);
        customInput.blur();
      }
    });
    customInput.addEventListener('input', () => {
      const v = readCustomDistanceFromInput();
      if (v != null) {
        selectedMoveM = v;
        usingCustomDist = true;
        customInput.classList.add('active-source');
        document.querySelectorAll('.preset-btn[data-move-m]').forEach((b) => b.classList.remove('active'));
        updateDistanceLabel();
      }
    });
    customInput.addEventListener('blur', () => {
      if (String(customInput.value).trim() !== '') applyCustomDistance(true);
    });
  }

  document.querySelectorAll('.move-dir').forEach((btn) => {
    btn.addEventListener('click', () => {
      const axis = btn.dataset.move;
      const sign = parseFloat(btn.dataset.sign) || 1;
      sendMoveBody(axis, sign * getMoveDistanceM());
    });
  });

  document.querySelectorAll('.btn-yaw').forEach((btn) => {
    btn.addEventListener('click', () => {
      sendYawRelative(parseFloat(btn.dataset.yaw) || 0);
    });
  });

  updateDistanceLabel();
}

function isTypingInField() {
  const el = document.activeElement;
  if (!el) return false;
  const tag = el.tagName;
  return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || el.isContentEditable;
}

function initKeyboardControls() {
  document.addEventListener('keydown', (e) => {
    if (e.code === 'Escape') {
      if (isTypingInField()) return;
      
      const qcEmergency = document.getElementById('qc-btn-emergency');
      const btnEmergency = document.getElementById('btn-emergency');
      const isArmed = (qcEmergency && !qcEmergency.disabled) || (btnEmergency && !btnEmergency.disabled);
      
      if (isArmed) {
        e.preventDefault();
        const now = performance.now();
        if (now - lastEscapePressMs < 1500) {
          lastEscapePressMs = 0;
          playWarningSound('trigger');
          sendCmd('EMERGENCY_STOP');
          log('SYS', 'tag-err', 'EMERGENCY DISARM VIA ESCAPE DOUBLE-PRESS!');
          resetEmergencyButtons();
        } else {
          lastEscapePressMs = now;
          playWarningSound('alarm');
          const allEmergencyBtns = [qcEmergency, btnEmergency];
          allEmergencyBtns.forEach(b => {
            if (b) {
              b.textContent = 'ESC AGAIN TO DISARM';
              b.classList.add('confirm-active');
            }
          });
          if (emergencyConfirmTimeout) clearTimeout(emergencyConfirmTimeout);
          emergencyConfirmTimeout = setTimeout(() => {
            resetEmergencyButtons();
          }, 1500);
        }
      }
      return;
    }

    if (activeTab !== 'map') return;
    if (isTypingInField()) return;
    if (!moveControlsEnabled) return;

    const throttle = CFG.keyboardMoveThrottleMs || 500;
    const now = performance.now();
    if (now - lastKeyMoveMs < throttle) return;

    let handled = false;
    const dist = getMoveDistanceM();

    switch (e.code) {
      case 'ArrowUp':
        sendMoveBody('x', dist);
        handled = true;
        break;
      case 'ArrowDown':
        sendMoveBody('x', -dist);
        handled = true;
        break;
      case 'ArrowLeft':
        sendMoveBody('y', -dist);
        handled = true;
        break;
      case 'ArrowRight':
        sendMoveBody('y', dist);
        handled = true;
        break;
      case 'Numpad8':
        sendMoveBody('z', -dist);
        handled = true;
        break;
      case 'Numpad2':
        sendMoveBody('z', dist);
        handled = true;
        break;
      case 'Numpad4':
        sendYawRelative(-90);
        handled = true;
        break;
      case 'Numpad6':
        sendYawRelative(90);
        handled = true;
        break;
      default:
        break;
    }

    if (handled) {
      e.preventDefault();
      lastKeyMoveMs = now;
    }
  });
}

function syncFlightUiState(d) {
  if (d.mav_connected !== true || d.autopilot_heartbeat_fresh !== true) {
    flightUiState.armed = false;
    flightUiState.guided = false;
    flightUiState.pendingKey = '';
    flightUiState.stableCount = 0;
    return;
  }

  const key =
    (d.armed ? '1' : '0') + '|' +
    (d.flight_mode_name || '') + '|' +
    String(d.flight_mode ?? '');
  const need = CFG.flightStateStableSamples || 3;

  if (key === flightUiState.pendingKey) {
    flightUiState.stableCount++;
  } else {
    flightUiState.pendingKey = key;
    flightUiState.stableCount = 1;
  }

  if (flightUiState.stableCount >= need) {
    flightUiState.armed = d.armed === true;
    flightUiState.guided =
      d.flight_mode_name === 'GUIDED' || Number(d.flight_mode) === 4;
  }
}

function updateMoveControls(d) {
  syncFlightUiState(d);

  const minAgl = 1;
  const canMove = d.can_move === true;

  moveControlsEnabled = canMove;
  document.querySelectorAll('.move-dir, .btn-yaw').forEach((el) => {
    el.disabled = !canMove;
  });

  const hint = document.getElementById('move-hint');
  if (hint) {
    if (canMove) {
      hint.textContent = 'Ready — ' + getMoveDistanceM() + ' m per move. Arrows = translate, Num 4/6 = yaw.';
      hint.className = 'move-hint ready';
    } else if (d.cmd_gate_ready === false) {
      hint.textContent = 'Commands locked: ' + (d.cmd_gate_reason || 'waiting for firmware gate');
      hint.className = 'move-hint';
    } else if (!flightUiState.armed) {
      hint.textContent = 'Arm in GUIDED after preflight checks to enable moves.';
      hint.className = 'move-hint';
    } else if (!flightUiState.guided) {
      hint.textContent = 'Switch to GUIDED mode to use relative moves.';
      hint.className = 'move-hint';
    } else {
      hint.textContent = 'Need 3D GPS fix and at least ' + minAgl + ' m altitude.';
      hint.className = 'move-hint';
    }
  }
}

function sendMoveBody(axis, meters) {
  const payload = { x: 0, y: 0, z: 0 };
  if (axis === 'x') payload.x = meters;
  else if (axis === 'y') payload.y = meters;
  else if (axis === 'z') payload.z = meters;

  if (sendCmd('MOVE_BODY', payload)) {
    log('SYS', 'tag-sys', 'MOVE_BODY ' + JSON.stringify(payload));
  }
}

function isClientFlightCommand(command) {
  return command === 'SET_FLIGHT_MODE' ||
    command === 'ARM_DRONE' ||
    command === 'DISARM_DRONE' ||
    command === 'TAKEOFF' ||
    command === 'MOVE_BODY' ||
    command === 'YAW_RELATIVE' ||
    command === 'GOTO_LATLON' ||
    command === 'LOITER_HERE' ||
    command === 'LAND' ||
    command === 'RTL';
}

function isCommandGateFresh() {
  const maxAgeMs = CFG.commandGateMaxAgeMs || 1200;
  return lastCommandGate.ready === true &&
    lastCommandGate.heartbeatMs > 0 &&
    Date.now() - lastCommandGate.heartbeatMs <= maxAgeMs;
}

function commandGateBlockReason() {
  if (lastCommandGate.ready && !isCommandGateFresh()) {
    return 'stale firmware heartbeat';
  }
  return lastCommandGate.reason || 'waiting for firmware command gate';
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function waitForCommandAck(commandId, timeoutMs) {
  return new Promise((resolve, reject) => {
    const waiter = {
      commandId,
      resolve,
      reject,
      timer: setTimeout(() => {
        ackWaiters = ackWaiters.filter((w) => w !== waiter);
        reject(new Error('ACK timeout for command ' + commandId));
      }, timeoutMs),
    };
    ackWaiters.push(waiter);
  });
}

function resolveCommandAck(commandId, ok, label) {
  const matched = ackWaiters.filter((w) => w.commandId === commandId);
  if (!matched.length) return;
  ackWaiters = ackWaiters.filter((w) => w.commandId !== commandId);
  lastCommandGate.commandPending = false;
  lastCommandGate.commandName = '';
  matched.forEach((w) => {
    clearTimeout(w.timer);
    if (ok) w.resolve(label);
    else w.reject(new Error('Command ' + commandId + ': ' + label));
  });
}

function waitForState(predicate, label, timeoutMs) {
  return new Promise((resolve, reject) => {
    const deadline = Date.now() + timeoutMs;
    const timer = setInterval(() => {
      if (!isCommandGateFresh()) {
        clearInterval(timer);
        reject(new Error(label + ' interrupted: ' + commandGateBlockReason()));
      } else if (predicate()) {
        clearInterval(timer);
        resolve();
      } else if (Date.now() > deadline) {
        clearInterval(timer);
        reject(new Error(label + ' timed out'));
      }
    }, 200);
  });
}

async function sendCommandAndWaitAck(command, extra, mavCommandId, label) {
  if (!sendCmd(command, extra)) {
    throw new Error(label + ' command was not sent');
  }
  const ackTimeoutMs = CFG.commandAckTimeoutMs || 4000;
  await waitForCommandAck(mavCommandId, ackTimeoutMs);
}

function sendYawRelative(deg) {
  if (sendCmd('YAW_RELATIVE', { deg })) {
    log('SYS', 'tag-sys', 'YAW_RELATIVE ' + deg + '°');
  }
}

function closeGotoSheet() {
  pendingGoto = null;
  const sheet = document.getElementById('goto-sheet');
  if (sheet) sheet.hidden = true;
}

function openGotoSheet(lat, lng) {
  if (!wsConnected) return;
  if (!lastHbForGoto?.can_goto) {
    log('ERR', 'tag-err', 'Fly here blocked: ' + (lastHbForGoto?.cmd_gate_reason || 'need armed GUIDED, 3D GPS, home, and safe altitude'));
    return;
  }
  const radius = CFG.geofenceRadiusM != null ? CFG.geofenceRadiusM : 1000;
  const dist = typeof SkylinkMap !== 'undefined' ? SkylinkMap.distanceFromHomeM(lat, lng) : null;
  if (dist == null) {
    log('ERR', 'tag-err', 'Home not set — wait for GPS/home on map');
    return;
  }
  if (dist > radius) {
    log('ERR', 'tag-err', 'Target ' + dist.toFixed(0) + ' m from home (max ' + radius + ' m)');
    return;
  }

  pendingGoto = { lat, lng };
  if (typeof SkylinkMap !== 'undefined') SkylinkMap.showTargetMarker(lat, lng);

  const coords = document.getElementById('goto-coords');
  const distEl = document.getElementById('goto-dist');
  const altInput = document.getElementById('goto-alt');
  const sheet = document.getElementById('goto-sheet');
  if (coords) coords.textContent = lat.toFixed(6) + ', ' + lng.toFixed(6);
  if (distEl) distEl.textContent = dist.toFixed(1) + ' m from home (inside geofence)';
  if (altInput) {
    const relAlt = Number(lastHbForGoto?.relative_alt) || Number(lastHbForGoto?.altitude) || 0;
    const minA = CFG.gotoAltMinM != null ? CFG.gotoAltMinM : 2;
    const maxA = CFG.gotoAltMaxM != null ? CFG.gotoAltMaxM : 50;
    const offset = CFG.gotoAltOffsetM != null ? CFG.gotoAltOffsetM : 5;
    const defAlt = Math.min(maxA, Math.max(minA, relAlt + offset));
    altInput.value = String(defAlt);
    altInput.min = String(minA);
    altInput.max = String(maxA);
  }
  if (sheet) sheet.hidden = false;
}

function confirmGoto() {
  if (!pendingGoto) return;
  const altInput = document.getElementById('goto-alt');
  const alt = parseFloat(altInput?.value);
  const minA = CFG.gotoAltMinM != null ? CFG.gotoAltMinM : 2;
  const maxA = CFG.gotoAltMaxM != null ? CFG.gotoAltMaxM : 50;
  if (!Number.isFinite(alt) || alt < minA || alt > maxA) {
    log('ERR', 'tag-err', 'Altitude must be ' + minA + '–' + maxA + ' m');
    return;
  }
  if (sendCmd('GOTO_LATLON', { lat: pendingGoto.lat, lon: pendingGoto.lng, alt })) {
    log('SYS', 'tag-sys', 'GOTO_LATLON @ ' + alt + ' m');
    closeGotoSheet();
  }
}

function updateMapFlightActions(d) {
  lastHbForGoto = d;
  const armed = d.armed === true;

  const loiterBtn = document.getElementById('btn-map-loiter');
  const landBtn = document.getElementById('btn-map-land');
  const rtlBtn = document.getElementById('btn-map-rtl');
  if (loiterBtn) loiterBtn.disabled = !d.can_loiter;
  if (landBtn) landBtn.disabled = !d.can_land;
  if (rtlBtn) rtlBtn.disabled = !d.can_rtl;

  // Quick control buttons
  const qcArm = document.getElementById('qc-btn-arm');
  const qcDisarm = document.getElementById('qc-btn-disarm');
  const qcLiftoff = document.getElementById('qc-btn-liftoff');
  const qcLand = document.getElementById('qc-btn-land');
  const qcRtl = document.getElementById('qc-btn-rtl');
  const qcSetMode = document.getElementById('qc-btn-set-mode');
  const qcModeSelect = document.getElementById('qc-mode-select');

  if (qcSetMode) qcSetMode.disabled = !d.can_set_mode;
  if (qcModeSelect) qcModeSelect.disabled = !d.can_set_mode;
  if (qcArm) qcArm.disabled = !d.can_arm;
  if (qcDisarm) qcDisarm.disabled = !d.can_disarm;
  if (qcLiftoff) qcLiftoff.disabled = !d.can_takeoff;
  if (qcLand) qcLand.disabled = !d.can_land;
  if (qcRtl) qcRtl.disabled = !d.can_rtl;
  
  const qcEmergency = document.getElementById('qc-btn-emergency');
  if (qcEmergency) qcEmergency.disabled = !d.can_emergency_stop;

  // Sync the Fly tab ones for parity and premium behavior
  const btnArm = document.getElementById('btn-arm');
  const btnDisarm = document.getElementById('btn-disarm');
  const btnLiftoff = document.getElementById('btn-liftoff');
  const btnLand = document.getElementById('btn-land');
  const btnRtl = document.getElementById('btn-rtl');
  const btnEmergency = document.getElementById('btn-emergency');
  const btnSetMode = document.getElementById('btn-set-mode');
  const modeSelect = document.getElementById('mode-select');

  if (btnSetMode) btnSetMode.disabled = !d.can_set_mode;
  if (modeSelect) modeSelect.disabled = !d.can_set_mode;
  if (btnArm) btnArm.disabled = !d.can_arm;
  if (btnDisarm) btnDisarm.disabled = !d.can_disarm;
  if (btnLiftoff) btnLiftoff.disabled = !d.can_takeoff;
  if (btnLand) btnLand.disabled = !d.can_land;
  if (btnRtl) btnRtl.disabled = !d.can_rtl;
  if (btnEmergency) btnEmergency.disabled = !d.can_emergency_stop;

  if (!armed) {
    resetEmergencyButtons();
  }
}

function initMapGoto() {
  if (typeof SkylinkMap !== 'undefined') {
    SkylinkMap.setGotoClickHandler(openGotoSheet);
  }
  document.getElementById('goto-cancel')?.addEventListener('click', closeGotoSheet);
  document.getElementById('goto-confirm')?.addEventListener('click', confirmGoto);
  document.getElementById('btn-map-loiter')?.addEventListener('click', () => {
    sendCmd('LOITER_HERE');
    log('SYS', 'tag-sys', 'LOITER_HERE');
  });
  document.getElementById('btn-map-land')?.addEventListener('click', () => {
    sendCmd('LAND');
    log('SYS', 'tag-sys', 'LAND');
  });
  document.getElementById('btn-map-rtl')?.addEventListener('click', () => {
    sendCmd('RTL');
    log('SYS', 'tag-sys', 'RTL');
  });
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
    'qc-btn-arm', 'qc-btn-disarm', 'qc-btn-liftoff', 'qc-btn-land', 'qc-btn-rtl', 'qc-btn-set-mode', 'qc-mode-select',
    'qc-btn-emergency', 'btn-emergency'
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
      if (el) el.disabled = true;
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
    resetEmergencyButtons();
    ['btn-map-loiter', 'btn-map-land', 'btn-map-rtl'].forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.disabled = true;
    });
    closeGotoSheet();
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
  if (el) {
    el.className = pass ? 'pass' : 'fail';
    const icon = el.querySelector('.pf-icon');
    if (icon) icon.textContent = pass ? '●' : '○';
  }
  const qcEl = document.getElementById('qc-' + id);
  if (qcEl) {
    qcEl.className = pass ? 'qc-led-item pass' : 'qc-led-item fail';
  }
}

function escapeHtml(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

function getGpsFixName(fix) {
  const f = Number(fix) || 0;
  switch (f) {
    case 0: return 'No GPS';
    case 1: return 'No Fix';
    case 2: return '2D Fix';
    case 3: return '3D Fix';
    case 4: return 'DGPS';
    case 5: return 'RTK Float';
    case 6: return 'RTK Fixed';
    case 7: return 'Static';
    case 8: return 'PPP';
    default: return 'Unknown (' + f + ')';
  }
}

function updatePreflight(d) {
  const wifiOk = d.wifi_connected === true;
  const minFix = CFG.preflightMinGpsFix || 3;
  const minBat = CFG.preflightMinBatteryPct || 20;
  const gpsOk = (Number(d.gps_fix) || 0) >= minFix;
  const mavOk = d.mav_connected === true && d.autopilot_heartbeat_fresh === true;
  const bat = Number(d.battery);
  const batOk = bat < 0 || bat >= minBat || d.simulation === true;
  syncFlightUiState(d);
  const safeOk = !flightUiState.armed;

  setPreflightItem('pf-wifi', wifiOk);
  setPreflightItem('pf-gps', gpsOk);
  setPreflightItem('pf-mav', mavOk);
  setPreflightItem('pf-bat', batOk);
  setPreflightItem('pf-safe', safeOk);

  const ready = wifiOk && gpsOk && mavOk && batOk && safeOk;
  const summary = document.getElementById('preflight-summary');
  const qcSummary = document.getElementById('qc-preflight-summary');

  let text = '';
  let className = '';

  if (ready) {
    text = d.safety_reason || 'All checks passed — you may arm.';
    className = 'ready';
  } else if (!mavOk) {
    text = d.safety_reason || 'Waiting for MAVLink connection…';
    className = 'bad';
  } else {
    text = d.safety_reason || 'Not ready to arm — check status.';
    className = 'warn';
  }

  if (summary) {
    summary.textContent = (d.safety_state_name || 'SAFETY') + ': ' + text;
    summary.className = 'fly-ready-banner ' + (ready ? 'ready' : (mavOk ? 'warn' : 'bad'));
  }
  if (qcSummary) {
    qcSummary.textContent = text;
    qcSummary.className = 'qc-banner ' + className;
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
  syncFlightUiState(d);

  const modeEl = document.getElementById('live-mode');
  if (modeEl) {
    modeEl.textContent = flightUiState.guided
      ? 'GUIDED'
      : (d.flight_mode_name || '—');
    modeEl.title = (d.safety_state_name || 'SAFETY') + ': ' + (d.safety_reason || '');
  }

  const armEl = document.getElementById('live-arm');
  if (armEl) {
    armEl.textContent = flightUiState.armed ? 'ARMED' : 'DISARMED';
    armEl.classList.toggle('armed', flightUiState.armed);
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
  const hbFresh = d.autopilot_heartbeat_fresh === true;
  const hbAgeS = Number(d.autopilot_heartbeat_age_ms || 0) / 1000;
  setChip(
    document.getElementById('chip-mav'),
    mavOk && hbFresh ? 'ok' : (mavOk ? 'warn' : 'bad'),
    mavOk && hbFresh ? 'MAV ●' : (mavOk ? ('MAV ◌ ' + hbAgeS.toFixed(1) + 's') : 'MAV ○')
  );

  const safetyName = d.safety_state_name || 'SAFETY';
  const safetyOk = d.safety_state_name === 'READY_TO_ARM' ||
                   d.safety_state_name === 'ARMED_GROUND' ||
                   d.safety_state_name === 'FLYING' ||
                   d.safety_state_name === 'LANDING';
  const safetyWarn = d.safety_state_name === 'PREFLIGHT' ||
                     d.safety_state_name === 'SETTLING';
  setChip(
    document.getElementById('chip-safety'),
    safetyOk ? 'ok' : (safetyWarn ? 'warn' : 'bad'),
    safetyName
  );

  const wifiOk = d.wifi_connected === true;
  const rssi = Number(d.wifi_rssi) || 0;
  setChip(document.getElementById('chip-wifi'), wifiOk ? 'ok' : 'bad', wifiOk ? ('WiFi ' + rssi) : 'WiFi');

  if (d.simulation) {
    const banner = document.getElementById('sim-banner');
    if (banner) banner.hidden = false;
  }
}

function logNewStatusTexts(lines) {
  if (!Array.isArray(lines) || !lines.length) return;
  const snap = lines.join('\n');
  if (snap === lastStatusSnapshot) return;
  const prev = lastStatusSnapshot ? lastStatusSnapshot.split('\n') : [];
  for (const txt of lines) {
    if (!txt || prev.includes(txt)) continue;
    const isErr = /denied|fail|error|prearm|reject/i.test(txt);
    log('FC', isErr ? 'tag-err' : 'tag-sys', txt);
  }
  lastStatusSnapshot = snap;
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
  lastCommandGate = {
    ready: d.cmd_gate_ready === true,
    reason: d.cmd_gate_reason || '',
    heartbeatMs: Date.now(),
    commandPending: d.command_pending === true,
    commandName: d.command_name || ''
  };

  // Log important telemetry state changes in dashboard log pane
  if (lastLoggedState.connected !== d.mav_connected) {
    if (lastLoggedState.connected !== null) {
      log('SYS', 'tag-sys', 'MAVLink Link: ' + (d.mav_connected ? 'CONNECTED' : 'LOST'));
    }
    lastLoggedState.connected = d.mav_connected;
  }
  if (d.mav_connected) {
    if (lastLoggedState.armed !== d.armed) {
      if (lastLoggedState.armed !== null) {
        log('SYS', 'tag-sys', 'Vehicle status: ' + (d.armed ? 'ARMED' : 'DISARMED'));
      }
      lastLoggedState.armed = d.armed;
    }
    const modeName = d.flight_mode_name || 'UNKNOWN';
    if (lastLoggedState.mode !== modeName) {
      if (lastLoggedState.mode !== null) {
        log('SYS', 'tag-sys', 'Flight Mode: ' + modeName);
      }
      lastLoggedState.mode = modeName;
    }
    const gpsFixText = getGpsFixName(d.gps_fix) + ' (' + sats + ' sats)';
    if (lastLoggedState.gps !== gpsFixText) {
      if (lastLoggedState.gps !== null) {
        log('SYS', 'tag-sys', 'GPS Fix: ' + gpsFixText);
      }
      lastLoggedState.gps = gpsFixText;
    }
  }

  const commandName = d.command_name || '';
  const commandStatus = d.command_status_name || '';
  const commandKey = commandName + '|' + commandStatus + '|' + String(d.command_result ?? '');
  if (commandName && commandStatus && commandKey !== lastLoggedState.commandKey) {
    lastLoggedState.commandKey = commandKey;
    const ageMs = Number(d.command_age_ms) || 0;
    const suffix = commandStatus === 'PENDING' ? (' (' + (ageMs / 1000).toFixed(1) + 's)') : '';
    const isErr = commandStatus === 'REJECTED' || commandStatus === 'TIMEOUT';
    if (commandStatus !== 'IDLE') {
      log(isErr ? 'ERR' : 'CMD', isErr ? 'tag-err' : 'tag-sys', commandName + ' ' + commandStatus + suffix);
    }
  }

  const set = (id, text) => {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  };

  set('tl-alt', alt.toFixed(1));
  set('qc-tl-alt', alt.toFixed(1));

  set('tl-speed', spd.toFixed(1));
  set('qc-tl-speed', spd.toFixed(1));

  set('tl-lat', lat.toFixed(6));
  set('tl-lng', lng.toFixed(6));
  set('tl-hdg', Number.isFinite(yaw) ? String(Math.round(yaw % 360)) : '—');
  set('lnk-ip', connectedIP);
  set('lnk-uptime', fmtUptime(d.uptime || 0));

  const modeName = d.flight_mode_name || String(d.flight_mode ?? '—');
  set('tl-mode', modeName);
  set('qc-tl-mode', modeName);

  set('tl-bat', bat + '% · ' + batV.toFixed(1) + ' V');
  set('qc-tl-bat', bat + '%');

  const compactGpsText = sats + ' Sats (' + (Number(d.gps_fix) >= 3 ? '3D' : (Number(d.gps_fix) === 2 ? '2D' : 'No Fix')) + ')';
  set('qc-tl-gps', compactGpsText);

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
  if (stateEl) stateEl.textContent = flightUiState.armed ? 'Armed' : 'Disarmed';

  const bar = document.getElementById('bat-bar');
  if (bar) {
    bar.style.width = Math.min(100, Math.max(0, bat)) + '%';
    bar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';
  }
  const qcBar = document.getElementById('qc-bat-bar');
  if (qcBar) {
    qcBar.style.width = Math.min(100, Math.max(0, bat)) + '%';
    qcBar.style.background = bat > 50 ? 'var(--green)' : bat > 20 ? 'var(--orange)' : 'var(--red)';
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

  const hbText = d.autopilot_heartbeat_fresh
    ? ('Autopilot HB ' + (Number(d.autopilot_heartbeat_age_ms || 0) / 1000).toFixed(1) + 's')
    : (d.mav_connected ? ('HB stale ' + (Number(d.autopilot_heartbeat_age_ms || 0) / 1000).toFixed(1) + 's') : 'Waiting');
  set('last-hb-text', hbText + ' · ' + sats + ' sats');

  logNewStatusTexts(d.statustext);
  updateLinkChips(d);
  updatePreflight(d);
  updateAttitude(d);
  updateLiveStrip(d);

  updateMoveControls(d);
  updateMapFlightActions(d);

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
  if (!ws || ws.readyState !== WebSocket.OPEN) return false;
  if (isClientFlightCommand(command) && !isCommandGateFresh()) {
    log('ERR', 'tag-err', command + ' not sent: ' + commandGateBlockReason());
    return false;
  }
  if (isClientFlightCommand(command) && lastCommandGate.commandPending) {
    log('ERR', 'tag-err', command + ' not sent: waiting for ' + (lastCommandGate.commandName || 'pending command'));
    return false;
  }
  if (isClientFlightCommand(command)) {
    const minIntervalMs = CFG.clientFlightCommandMinIntervalMs || 650;
    const now = Date.now();
    if (now - lastFlightCommandTxMs < minIntervalMs) {
      log('ERR', 'tag-err', command + ' not sent: client command spacing');
      return false;
    }
    lastFlightCommandTxMs = now;
  }
  const msg = { v: CFG.protocolVersion || 1, type: 'command', command, ...(extra || {}) };
  ws.send(JSON.stringify(msg));
  log('TX', 'tag-sys', command + (extra ? ' → ' + JSON.stringify(extra) : ''));
  return true;
}

function applyFlightMode(selectId = 'mode-select') {
  const val = document.getElementById(selectId)?.value;
  if (val) sendCmd('SET_FLIGHT_MODE', { mode: val });
}

async function armSequence(selectId = 'mode-select') {
  if (_armInProgress || _takeoffInProgress) {
    log('ERR', 'tag-err', 'Arm/takeoff sequence already in progress.');
    return;
  }

  const modeSelect = document.getElementById(selectId);
  if (modeSelect && modeSelect.value !== 'GUIDED') {
    log('SYS', 'tag-err', 'GUIDED mode required before arming');
    modeSelect.value = 'GUIDED';
    // Sync the other selector
    const otherId = selectId === 'mode-select' ? 'qc-mode-select' : 'mode-select';
    const otherSelect = document.getElementById(otherId);
    if (otherSelect) otherSelect.value = 'GUIDED';
  }

  try {
    _armInProgress = true;
    const stateTimeoutMs = CFG.stateConfirmTimeoutMs || 10000;
    log('SYS', 'tag-sys', '[ARM 1/4] Sending GUIDED mode…');
    await sendCommandAndWaitAck('SET_FLIGHT_MODE', { mode: 'GUIDED' }, MAV_CMD.DO_SET_MODE, 'GUIDED');
    log('SYS', 'tag-sys', '[ARM 2/4] GUIDED ACK accepted — waiting for GUIDED state…');
    await waitForState(() => flightUiState.guided, 'GUIDED state confirmation', stateTimeoutMs);

    await sleep(CFG.armModeDelayMs || 800);
    log('SYS', 'tag-sys', '[ARM 3/4] GUIDED confirmed — sending ARM…');
    await sendCommandAndWaitAck('ARM_DRONE', null, MAV_CMD.COMPONENT_ARM_DISARM, 'ARM');
    log('SYS', 'tag-sys', '[ARM 4/4] ARM ACK accepted — waiting for ARMED state…');
    await waitForState(() => flightUiState.armed, 'ARMED state confirmation', stateTimeoutMs);
  } catch (err) {
    log('ERR', 'tag-err', '[ARM ABORTED] ' + (err && err.message ? err.message : String(err)));
  } finally {
    _armInProgress = false;
  }
}

async function autonomousTakeoff() {
  // FIX: Mutex — prevent multiple concurrent takeoff state machines.
  // Each call creates new setInterval instances; without this guard, rapid
  // button clicks stack up and race each other to ARM + TAKEOFF.
  if (_armInProgress || _takeoffInProgress) {
    log('ERR', 'tag-err', 'Arm/takeoff sequence already in progress — wait or disarm first.');
    return;
  }

  if (!isCommandGateFresh()) {
    log('ERR', 'tag-err', 'Takeoff not started: ' +
      commandGateBlockReason());
    return;
  }

  const alt = prompt('Takeoff altitude (meters)?', String(CFG.defaultTakeoffAltM || 5));
  if (alt === null) return;
  const meters = parseFloat(alt);
  const altMin = CFG.takeoffAltMinM || 1;
  const altMax = CFG.takeoffAltMaxM || 50;
  if (isNaN(meters) || meters < altMin || meters > altMax) {
    log('ERR', 'tag-err', 'Altitude must be between ' + altMin + ' and ' + altMax + ' m');
    return;
  }

  _takeoffInProgress = true;

  try {
    const stateTimeoutMs = CFG.stateConfirmTimeoutMs || 10000;

    flightUiState.guided = false;
    flightUiState.armed = false;
    flightUiState.stableCount = 0;

    log('SYS', 'tag-sys', '[TAKEOFF 1/6] Sending GUIDED mode…');
    await sendCommandAndWaitAck('SET_FLIGHT_MODE', { mode: 'GUIDED' }, MAV_CMD.DO_SET_MODE, 'GUIDED');
    log('SYS', 'tag-sys', '[TAKEOFF 2/6] GUIDED ACK accepted — waiting for GUIDED state…');
    await waitForState(() => flightUiState.guided, 'GUIDED state confirmation', stateTimeoutMs);

    await sleep(CFG.takeoffArmDelayMs || 800);
    log('SYS', 'tag-sys', '[TAKEOFF 3/6] GUIDED confirmed — sending ARM…');
    await sendCommandAndWaitAck('ARM_DRONE', null, MAV_CMD.COMPONENT_ARM_DISARM, 'ARM');
    log('SYS', 'tag-sys', '[TAKEOFF 4/6] ARM ACK accepted — waiting for ARMED state…');
    await waitForState(() => flightUiState.armed && flightUiState.guided, 'ARMED GUIDED state confirmation', stateTimeoutMs);

    await sleep(CFG.takeoffCommandDelayMs || 800);
    log('SYS', 'tag-sys', '[TAKEOFF 5/6] Armed in GUIDED confirmed — sending TAKEOFF…');
    await sendCommandAndWaitAck('TAKEOFF', { altitude: meters }, MAV_CMD.NAV_TAKEOFF, 'TAKEOFF');
    log('SYS', 'tag-sys', '[TAKEOFF 6/6] TAKEOFF ACK accepted for ' + meters + ' m');
  } catch (err) {
    log('ERR', 'tag-err', '[TAKEOFF ABORTED] ' + (err && err.message ? err.message : String(err)));
  } finally {
    _takeoffInProgress = false;
  }
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
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
    return;
  }
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
          resolveCommandAck(Number(d.command), ok, label);
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
    ws = null;
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
initMapGoto();

function playWarningSound(type = 'alarm') {
  try {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    if (!AudioContext) return;
    const ctx = new AudioContext();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    
    if (type === 'trigger') {
      // Urgent triple beep
      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(988, ctx.currentTime); // B5 note
      osc.frequency.setValueAtTime(0, ctx.currentTime + 0.08);
      osc.frequency.setValueAtTime(988, ctx.currentTime + 0.12);
      osc.frequency.setValueAtTime(0, ctx.currentTime + 0.20);
      osc.frequency.setValueAtTime(988, ctx.currentTime + 0.24);
      gain.gain.setValueAtTime(0.25, ctx.currentTime);
      gain.gain.linearRampToValueAtTime(0.01, ctx.currentTime + 0.4);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.4);
    } else {
      // First click/alarm warning beep
      osc.type = 'sine';
      osc.frequency.setValueAtTime(587.33, ctx.currentTime); // D5 note
      osc.frequency.exponentialRampToValueAtTime(880, ctx.currentTime + 0.2);
      gain.gain.setValueAtTime(0.15, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.2);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.2);
    }
  } catch (e) {
    console.error('Audio feedback error', e);
  }
}

function triggerEmergencyDisarm(btn) {
  const targetBtn = btn || document.getElementById('qc-btn-emergency') || document.getElementById('btn-emergency');
  if (!targetBtn || targetBtn.disabled) return;

  const allEmergencyBtns = [
    document.getElementById('qc-btn-emergency'),
    document.getElementById('btn-emergency')
  ];

  if (emergencyConfirmTimeout) {
    // Second click: Execute immediate disarm
    clearTimeout(emergencyConfirmTimeout);
    emergencyConfirmTimeout = null;
    
    playWarningSound('trigger');
    sendCmd('EMERGENCY_STOP');
    log('SYS', 'tag-err', 'EMERGENCY DISARM COMMAND SENT!');

    resetEmergencyButtons();
  } else {
    // First click: arm and show confirmation warning
    playWarningSound('alarm');
    allEmergencyBtns.forEach(b => {
      if (b) {
        b.textContent = 'CONFIRM DISARM';
        b.classList.add('confirm-active');
      }
    });

    emergencyConfirmTimeout = setTimeout(() => {
      resetEmergencyButtons();
    }, 2500); // 2.5 second window to confirm
  }
}

function resetEmergencyButtons() {
  if (emergencyConfirmTimeout) {
    clearTimeout(emergencyConfirmTimeout);
    emergencyConfirmTimeout = null;
  }
  const allEmergencyBtns = [
    document.getElementById('qc-btn-emergency'),
    document.getElementById('btn-emergency')
  ];
  allEmergencyBtns.forEach(b => {
    if (b) {
      b.textContent = 'Emergency Disarm';
      b.classList.remove('confirm-active');
    }
  });
}
