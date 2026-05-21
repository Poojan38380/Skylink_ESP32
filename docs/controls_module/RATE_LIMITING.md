# Command Rate Limiting & Debounce

## Firmware: SKYLINK_CMD_DEBOUNCE_MS = 500ms
Applied in: moveBody(), yawRelative(), gotoLatLon(), gotoAlt()

## Browser: keyThrottleMs = 500ms (matching firmware)
Prevents redundant WebSocket messages before they reach ESP32.

## Why 500ms?
- Keyboard repeat fires every 30-50ms → 20+ commands/sec without debounce
- ArduPilot processes targets sequentially — flooding causes jitter
- 500ms = 2 cmd/sec, smooth and predictable
