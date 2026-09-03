/**
 * Skylink Bench Test — props-off ground testing only. No takeoff.
 */
'use strict';

const Bench = (function () {
  const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};
  const MAV = {
    DO_SET_MODE: 176,
    COMPONENT_ARM_DISARM: 400,
    DO_MOTOR_TEST: 209,
  };

  let active = false;
  let lastTelemetry = null;
  let hbTimestamps = [];
  let pingRtts = [];
  let runnerBusy = false;
  let checklistRows = [];
  let propsConfirmed = false;

  const els = {};

  function $(id) {
    return document.getElementById(id);
  }

  function sleep(ms) {
    return new Promise((r) => setTimeout(r, ms));
  }

  function median(arr) {
    if (!arr.length) return 0;
    const s = [...arr].sort((a, b) => a - b);
    const mid = Math.floor(s.length / 2);
    return s.length % 2 ? s[mid] : (s[mid - 1] + s[mid]) / 2;
  }

  function p95(arr) {
    if (!arr.length) return 0;
    const s = [...arr].sort((a, b) => a - b);
    const idx = Math.min(s.length - 1, Math.ceil(s.length * 0.95) - 1);
    return s[idx];
  }

  function setMetric(id, text, cls) {
    const el = els[id] || $(id);
    if (!el) return;
    el.textContent = text;
    if (cls) el.className = 'bench-metric-val ' + cls;
  }

  function updateGpsChip(d) {
    const chip = els.gpsChip || $('bench-gps-chip');
    if (!chip) return;
    const fix = Number(d.gps_fix) || 0;
    const sats = Number(d.sats) || 0;
    let label = getGpsFixLabel(fix) + ' · ' + sats + ' sats';
    let cls = 'bench-gps-neutral';
    if (fix >= 3) cls = 'bench-gps-ok';
    else if (fix >= 1) cls = 'bench-gps-warn';
    chip.className = 'bench-gps-chip ' + cls;
    chip.textContent = label + ' (informational)';
  }

  function getGpsFixLabel(fix) {
    switch (fix) {
      case 0: return 'No GPS';
      case 1: return 'No fix';
      case 2: return '2D';
      case 3: return '3D';
      default: return 'Fix ' + fix;
    }
  }

  function updateMetrics(d) {
    lastTelemetry = d;
    const now = Date.now();
    hbTimestamps.push(now);
    hbTimestamps = hbTimestamps.filter((t) => now - t <= 5000);
    const rate = (hbTimestamps.length / 5).toFixed(1);

    setMetric('bench-metric-rate', rate + ' Hz');
    setMetric(
      'bench-metric-hb',
      d.autopilot_heartbeat_fresh
        ? (Number(d.autopilot_heartbeat_age_ms || 0) + ' ms')
        : 'stale'
    );
    setMetric('bench-metric-rssi', d.wifi_connected ? (Number(d.wifi_rssi) || 0) + ' dBm' : '—');
    setMetric('bench-metric-heap', d.min_free_heap != null ? (Number(d.min_free_heap) + ' B') : '—');
    setMetric('bench-metric-ping', pingRtts.length ? (median(pingRtts) + ' / ' + p95(pingRtts) + ' ms') : '—');
    updateGpsChip(d);
    updateGroupBButtons(d);
  }

  function allConfirmationsChecked() {
    return ['bench-confirm-props', 'bench-confirm-hold', 'bench-confirm-arming', 'bench-confirm-notest'].every(
      (id) => {
        const el = $(id);
        return el && el.checked;
      }
    );
  }

  function updateGroupBButtons(d) {
    propsConfirmed = allConfirmationsChecked();
    const gate = propsConfirmed && d && d.can_motor_test !== false;
    const armOk = propsConfirmed && d && d.can_arm === true;
    const disarmOk = d && d.can_disarm === true;

    ['bench-btn-arm', 'bench-btn-disarm'].forEach((id, on) => {
      const el = $(id);
      if (!el) return;
      if (id === 'bench-btn-arm') el.disabled = !armOk || runnerBusy;
      else el.disabled = !disarmOk || runnerBusy;
    });

    document.querySelectorAll('[data-bench-motor]').forEach((btn) => {
      btn.disabled = !gate || runnerBusy || (d && d.armed === true);
    });

    const runB = $('bench-run-group-b');
    if (runB) runB.disabled = !propsConfirmed || runnerBusy;
  }

  function onConfirmChange() {
    updateGroupBButtons(lastTelemetry || {});
  }

  function recordStep(group, step, command, ackMs, pass, notes) {
    const d = lastTelemetry || {};
    const row = {
      timestamp: new Date().toISOString(),
      group,
      step,
      command,
      ack_ms: ackMs != null ? ackMs : '',
      pass: pass ? 'PASS' : 'FAIL',
      gps_fix: d.gps_fix != null ? d.gps_fix : '',
      sats: d.sats != null ? d.sats : '',
      rssi: d.wifi_rssi != null ? d.wifi_rssi : '',
      min_heap: d.min_free_heap != null ? d.min_free_heap : '',
      notes: notes || '',
    };
    checklistRows.push(row);
    renderChecklistTable();
  }

  function renderChecklistTable() {
    const tbody = $('bench-checklist-body');
    if (!tbody) return;
    tbody.innerHTML = checklistRows
      .map(
        (r) =>
          '<tr class="' +
          (r.pass === 'PASS' ? 'pass' : 'fail') +
          '"><td>' +
          escapeHtml(r.step) +
          '</td><td>' +
          escapeHtml(r.command) +
          '</td><td>' +
          escapeHtml(String(r.ack_ms)) +
          '</td><td>' +
          escapeHtml(r.pass) +
          '</td><td>' +
          escapeHtml(r.notes) +
          '</td></tr>'
      )
      .join('');
  }

  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;');
  }

  function exportCsv() {
    const headers = ['timestamp', 'group', 'step', 'command', 'ack_ms', 'pass', 'gps_fix', 'sats', 'rssi', 'min_heap', 'notes'];
    const lines = [headers.join(',')];
    checklistRows.forEach((r) => {
      lines.push(
        headers
          .map((h) => {
            const v = r[h] != null ? String(r[h]) : '';
            return v.includes(',') || v.includes('"') ? '"' + v.replace(/"/g, '""') + '"' : v;
          })
          .join(',')
      );
    });
    const blob = new Blob([lines.join('\n')], { type: 'text/csv' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'skylink_bench_' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.csv';
    a.click();
    URL.revokeObjectURL(a.href);
    if (typeof log === 'function') log('SYS', 'tag-sys', 'Exported ' + checklistRows.length + ' bench rows');
  }

  async function waitForAck(mavCmd, timeoutMs) {
    const t0 = performance.now();
    await window.SkylinkGcs.waitForCommandAck(mavCmd, timeoutMs || CFG.benchAckTimeoutMs || 8000);
    return Math.round(performance.now() - t0);
  }

  let lastBenchCmdAt = 0;

  async function waitBeforeBenchCmd(command) {
    if (typeof window.SkylinkGcs.waitForCommandIdle === 'function') {
      await window.SkylinkGcs.waitForCommandIdle(8000);
    }
    const spacing =
      command === 'SET_FLIGHT_MODE'
        ? CFG.benchCommandSpacingMs || 1700
        : CFG.benchMotorSpacingMs || 800;
    const elapsed = Date.now() - lastBenchCmdAt;
    if (lastBenchCmdAt > 0 && elapsed < spacing) {
      await sleep(spacing - elapsed);
    }
  }

  async function sendBenchCmd(command, extra, mavCmd) {
    await waitBeforeBenchCmd(command);
    const ackPromise = window.SkylinkGcs.waitForCommandAck(mavCmd, CFG.benchAckTimeoutMs || 8000);
    if (!window.SkylinkGcs.sendCmd(command, extra)) {
      window.SkylinkGcs.cancelCommandAckWait(mavCmd);
      throw new Error(command + ' not sent');
    }
    const t0 = performance.now();
    lastBenchCmdAt = Date.now();
    await ackPromise;
    return Math.round(performance.now() - t0);
  }

  async function runPingBurst() {
    pingRtts = [];
    const count = CFG.benchPingBurstCount || 10;
    for (let i = 0; i < count; i++) {
      const rtt = await window.SkylinkGcs.pingOnce();
      if (rtt != null) pingRtts.push(rtt);
      await sleep(CFG.benchPingIntervalMs || 150);
    }
    setMetric('bench-metric-ping', pingRtts.length ? median(pingRtts) + ' / ' + p95(pingRtts) + ' ms' : 'fail');
    return pingRtts.length >= Math.ceil(count / 2);
  }

  async function runGroupA() {
    runnerBusy = true;
    updateGroupBButtons(lastTelemetry || {});
    if (typeof log === 'function') log('SYS', 'tag-sys', '[Bench A] Starting link & comms checklist…');

    try {
      if (!window.SkylinkGcs.isWsConnected()) {
        recordStep('A', 'WebSocket', '—', null, false, 'not connected');
        throw new Error('WebSocket not connected');
      }
      recordStep('A', 'WebSocket', '—', null, true, 'connected');

      if (!window.SkylinkGcs.sendCmd('BENCH_MODE_ON')) {
        throw new Error('BENCH_MODE_ON not sent');
      }
      recordStep('A', 'Bench mode', 'BENCH_MODE_ON', null, true, '');

      const pingOk = await runPingBurst();
      recordStep('A', 'PING burst', 'PING', null, pingOk, pingRtts.length + ' replies');

      const d = lastTelemetry || {};
      const mavOk = d.mav_connected && d.autopilot_heartbeat_fresh;
      recordStep('A', 'MAVLink HB', '—', null, mavOk, d.autopilot_heartbeat_age_ms + ' ms');

      const telemOk = d.flight_mode_name != null && d.battery != null;
      recordStep('A', 'Telemetry', '—', null, telemOk, 'mode/battery present');

      let ackMs = await sendBenchCmd('SET_FLIGHT_MODE', { mode: 'STABILIZE' }, MAV.DO_SET_MODE);
      recordStep('A', 'Mode STABILIZE', 'SET_FLIGHT_MODE', ackMs, true, '');

      ackMs = await sendBenchCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' }, MAV.DO_SET_MODE);
      recordStep('A', 'Mode GUIDED', 'SET_FLIGHT_MODE', ackMs, true, '');

      ackMs = await sendBenchCmd('SET_FLIGHT_MODE', { mode: 'LAND' }, MAV.DO_SET_MODE);
      recordStep('A', 'Mode LAND', 'SET_FLIGHT_MODE', ackMs, true, '');

      if (typeof log === 'function') log('SYS', 'tag-sys', '[Bench A] Complete');
    } catch (err) {
      recordStep('A', 'Error', '—', null, false, err.message || String(err));
      if (typeof log === 'function') log('ERR', 'tag-err', '[Bench A] ' + (err.message || err));
    } finally {
      runnerBusy = false;
      updateGroupBButtons(lastTelemetry || {});
    }
  }

  async function runGroupB() {
    if (!allConfirmationsChecked()) {
      if (typeof log === 'function') log('ERR', 'tag-err', 'Complete all props-off confirmations first');
      return;
    }
    runnerBusy = true;
    updateGroupBButtons(lastTelemetry || {});
    if (typeof log === 'function') log('SYS', 'tag-sys', '[Bench B] Props-off mechanical tests…');

    try {
      if (!lastTelemetry || !lastTelemetry.bench_mode_active) {
        if (!window.SkylinkGcs.sendCmd('BENCH_MODE_ON')) {
          throw new Error('BENCH_MODE_ON not sent');
        }
        await sleep(300);
      }

      let ackMs = await sendBenchCmd('ARM_DRONE', null, MAV.COMPONENT_ARM_DISARM);
      recordStep('B', 'Arm', 'ARM_DRONE', ackMs, true, 'hold drone');

      await sleep(CFG.benchArmHoldMs || 1500);

      ackMs = await sendBenchCmd('DISARM_DRONE', null, MAV.COMPONENT_ARM_DISARM);
      recordStep('B', 'Disarm', 'DISARM_DRONE', ackMs, true, '');

      const motors = ['A', 'B', 'C', 'D'];
      for (let i = 0; i < motors.length; i++) {
        const label = motors[i];
        const motorNum = i + 1;
        ackMs = await sendBenchCmd(
          'MOTOR_TEST',
          {
            motor: motorNum,
            throttle_pct: CFG.benchMotorThrottlePct || 10,
            duration_s: CFG.benchMotorDurationS || 2,
          },
          MAV.DO_MOTOR_TEST
        );
        recordStep('B', 'Motor ' + label, 'MOTOR_TEST', ackMs, true, motorNum + '@10%');
        await sleep(500);
      }

      if (typeof log === 'function') log('SYS', 'tag-sys', '[Bench B] Complete');
    } catch (err) {
      recordStep('B', 'Error', '—', null, false, err.message || String(err));
      if (typeof log === 'function') log('ERR', 'tag-err', '[Bench B] ' + (err.message || err));
    } finally {
      runnerBusy = false;
      updateGroupBButtons(lastTelemetry || {});
    }
  }

  function enterBenchTab() {
    if (active) return;
    active = true;
    if (typeof window.SkylinkGcs !== 'undefined' && window.SkylinkGcs.sendCmd) {
      window.SkylinkGcs.sendCmd('BENCH_MODE_ON');
      if (typeof log === 'function') log('SYS', 'tag-sys', 'Bench mode activated');
    }
  }

  function leaveBenchTab() {
    if (!active) return;
    active = false;
    if (typeof window.SkylinkGcs !== 'undefined' && window.SkylinkGcs.sendCmd) {
      window.SkylinkGcs.sendCmd('BENCH_MODE_OFF');
      if (typeof log === 'function') log('SYS', 'tag-sys', 'Bench mode deactivated');
    }
  }

  function bindUi() {
    els.gpsChip = $('bench-gps-chip');
    $('bench-run-group-a')?.addEventListener('click', () => runGroupA());
    $('bench-run-group-b')?.addEventListener('click', () => runGroupB());
    $('bench-export-csv')?.addEventListener('click', exportCsv);
    $('bench-btn-arm')?.addEventListener('click', async () => {
      try {
        const ms = await sendBenchCmd('ARM_DRONE', null, MAV.COMPONENT_ARM_DISARM);
        recordStep('B', 'Manual arm', 'ARM_DRONE', ms, true, '');
      } catch (e) {
        recordStep('B', 'Manual arm', 'ARM_DRONE', null, false, e.message);
      }
    });
    $('bench-btn-disarm')?.addEventListener('click', async () => {
      try {
        const ms = await sendBenchCmd('DISARM_DRONE', null, MAV.COMPONENT_ARM_DISARM);
        recordStep('B', 'Manual disarm', 'DISARM_DRONE', ms, true, '');
      } catch (e) {
        recordStep('B', 'Manual disarm', 'DISARM_DRONE', null, false, e.message);
      }
    });
    document.querySelectorAll('[data-bench-motor]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        const motor = parseInt(btn.dataset.benchMotor, 10) || 1;
        try {
          const ms = await sendBenchCmd(
            'MOTOR_TEST',
            {
              motor,
              throttle_pct: CFG.benchMotorThrottlePct || 10,
              duration_s: CFG.benchMotorDurationS || 2,
            },
            MAV.DO_MOTOR_TEST
          );
          recordStep('B', 'Motor ' + btn.textContent.trim(), 'MOTOR_TEST', ms, true, '');
        } catch (e) {
          recordStep('B', 'Motor ' + btn.textContent.trim(), 'MOTOR_TEST', null, false, e.message);
        }
      });
    });
    ['bench-confirm-props', 'bench-confirm-hold', 'bench-confirm-arming', 'bench-confirm-notest'].forEach((id) => {
      $(id)?.addEventListener('change', onConfirmChange);
    });
  }

  function onTabChange(tabName) {
    if (tabName === 'bench') enterBenchTab();
    else leaveBenchTab();
  }

  function showTabForHardware(isHw) {
    const btn = $('tab-btn-bench');
    if (btn) btn.hidden = !isHw;
  }

  document.addEventListener('DOMContentLoaded', bindUi);

  return {
    onTelemetry: updateMetrics,
    onTabChange,
    showTabForHardware,
    isActive: () => active,
  };
})();

window.SkylinkBench = Bench;
