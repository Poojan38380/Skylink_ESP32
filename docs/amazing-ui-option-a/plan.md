# Skylink GCS — Implementation Plan (Scratch Rebuild)

> **Target:** A Bloomberg Terminal-grade drone GCS — dense with data, beautiful in darkness, effortless in operation.
> **Constraint:** Total dashboard asset size <500KB (ESP32 LittleFS). Every byte counts.
> **Stack:** Vanilla JS + Leaflet + CSS Custom Properties + esbuild (for minification only).

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Phase 1: Project Foundation](#2-phase-1-project-foundation)
3. [Phase 2: Layout Grid & Chrome](#3-phase-2-layout-grid--chrome)
4. [Phase 3: Primary Flight Display (PFD)](#4-phase-3-primary-flight-display-pfd)
5. [Phase 4: Map Stage](#5-phase-4-map-stage)
6. [Phase 5: Vehicle Status Panel (Right Wing)](#6-phase-5-vehicle-status-panel-right-wing)
7. [Phase 6: Command Bar (Bottom Dock)](#7-phase-6-command-bar-bottom-dock)
8. [Phase 7: WebSocket & State Layer](#8-phase-7-websocket--state-layer)
9. [Phase 8: Keyboard System](#9-phase-8-keyboard-system)
10. [Phase 9: Animations & Feedback](#10-phase-9-animations--feedback)
11. [Phase 10: Mobile & Responsive](#11-phase-10-mobile--responsive)
12. [Phase 11: Connection Loss & Error Handling](#12-phase-11-connection-loss--error-handling)
13. [Phase 12: Build, Optimize & Deploy](#12-phase-12-build-optimize--deploy)
14. [File Manifest](#14-file-manifest)
15. [Order of Implementation](#15-order-of-implementation)

---

## 1. Architecture Overview

### 1.1 Principles

| Principle | How we achieve it |
|-----------|-------------------|
| **No tabs — all data, always** | Fixed 3-column grid: Left Wing (280px) \| Map (flex) \| Right Wing (340px). Each zone is a persistent `<section>`. |
| **Keyboard-first** | Global `keydown` listener on `document`. Every shortcut documented in a `?` modal. |
| **ESP32-friendly** | Vanilla JS. No React/Vue/Svelte. Leaflet ~40KB gzipped. Self-host subsetted fonts. esbuild minification. Total <500KB. |
| **Dark cockpit** | `#050505` base, `#0A0E14` panels, `#E8EDF5` text. Cyan/green/amber/red color semantics. |
| **3-second situation awareness** | Altitude, heading, speed, armed/mode, battery always in the top ~20% of viewport. Color tells you health instantly. |
| **Resilient to disconnection** | Graceful degradation: map freezes, values show last known + `---` animation, controls disable, reconnect overlay after 5s. |

### 1.2 File Layout (under `data/`)

```
data/
├── index.html              # Single HTML file, all sections in DOM
├── gcs.css                 # All styles (CSS custom properties + utility classes)
├── gcs.js                  # Main application logic, state, WebSocket, commands
├── gcs_map.js              # Leaflet map setup, overlays, markers, trails
├── gcs_config.js           # User-facing settings (WS URL, defaults)
├── gcs_components.js       # Reusable component constructors (Tile, Sparkline,
│                           #   AttitudeIndicator, AltitudeTape, PreflightList, etc.)
├── favicon.ico
├── skylink_build.json      # Build metadata
└── lib/
    └── leaflet/            # Leaflet 1.9.x (unchanged from existing)
```

**Why this split:** Each file is <50KB uncompressed. Functional separation makes debugging and iteration fast. No bundler dependency — serve raw, let HTTP-level GZip handle compression.

### 1.3 Data Flow

```
WebSocket (5 Hz) → gcs.js:parseTelemetry()
                    → state.update(...)       (central state store)
                    → render loop (rAF)       (batch DOM updates once per frame)
                    → gcs_map.js update       (map marker, trail, geofence)
                    → gcs_components.js patch (each component patches own DOM)
```

Commands flow the opposite direction:

```
Keyboard shortcut / Button click → gcs.js:sendCommand()
                                  → WebSocket send
                                  → gcs_components.js:CommandLog (optimistic entry)
                                  → ACK handler → flash success / failure
```

---

## 2. Phase 1: Project Foundation

### 2.1 Design Tokens (CSS Custom Properties)

Create `gcs.css` with all tokens from the design spec:

```css
:root {
  /* Backgrounds */
  --bg-void: #050505;
  --bg-panel: #0A0E14;
  --bg-elevated: #11161E;

  /* Borders */
  --border-subtle: #1A1F2E;
  --border-active: #2D354A;

  /* Text */
  --text-primary: #E8EDF5;
  --text-secondary: #8B95A5;
  --text-muted: #4A5568;

  /* Accents */
  --accent-cyan: #00D4FF;
  --accent-green: #34D399;
  --accent-amber: #FBBF24;
  --accent-red: #EF4444;
  --accent-purple: #A78BFA;
  --accent-blue: #60A5FA;

  /* Spacing */
  --space-1: 4px;
  --space-2: 8px;
  --space-3: 12px;
  --space-4: 16px;
  --space-5: 24px;
  --space-6: 32px;
  --space-7: 48px;

  /* Radius */
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 12px;
  --radius-full: 999px;

  /* Typography */
  --font-mono: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace;
  --font-sans: 'Inter', -apple-system, 'Segoe UI', sans-serif;

  /* Shadows */
  --shadow-sm: 0 1px 2px rgba(0,0,0,0.3);
  --shadow-md: 0 4px 6px rgba(0,0,0,0.4);
  --shadow-lg: 0 10px 15px rgba(0,0,0,0.5);
  --shadow-glow-cyan: 0 0 8px rgba(0,212,255,0.3);
  --shadow-glow-red: 0 0 8px rgba(239,68,68,0.3);

  /* Z-index layers */
  --z-base: 0;
  --z-content: 10;
  --z-overlay: 100;
  --z-dropdown: 200;
  --z-toast: 300;
  --z-loading: 400;
  --z-max: 500;

  /* Layout */
  --left-wing-width: 280px;
  --right-wing-width: 340px;
  --status-bar-height: 32px;
  --header-height: 48px;
  --command-bar-height: 140px;
}
```

### 2.2 Font Strategy

- **JetBrains Mono** — subset to Latin Extended-A + figures + punctuation only (~30KB). Self-host as `woff2`.
- **Inter** — subset to Latin Extended-A required weights (400, 500, 600, 700) (~40KB). Self-host as `woff2`.
- **No Google Fonts CDN at runtime.** Preload with `<link rel="preload" as="font" crossorigin>`.
- Fonts served from `data/fonts/`.

### 2.3 CSS Reset & Base

```css
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html, body { height: 100%; overflow: hidden; background: var(--bg-void); color: var(--text-primary); font-family: var(--font-sans); }
```

### 2.4 Scrollbar Styling

Thin, dark, always-visible scrollbars matching the cockpit theme:

```css
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: transparent; }
::-webkit-scrollbar-thumb { background: var(--border-active); border-radius: 3px; }
```

### 2.5 `prefers-reduced-motion` Support

```css
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
  }
}
```

---

## 3. Phase 2: Layout Grid & Chrome

### 3.1 HTML Skeleton

The `index.html` body is a CSS Grid with these rows and columns:

```
┌─────────────────────────────────────────────────────────────┐
│ HEADER (48px) — Zone A                                       │
├──────────┬─────────────────────────┬────────────────────────┤
│          │                         │                         │
│ LEFT WING│      MAIN STAGE         │      RIGHT WING         │
│ (280px)  │      (flex)             │      (340px)            │
│ Zone B   │      Zone C             │      Zone D             │
│          │                         │                         │
├──────────┴─────────────────────────┴────────────────────────┤
│ COMMAND BAR (~140px) — Zone E                                 │
├──────────────────────────────────────────────────────────────┤
│ MESSAGE LOG STRIP (32px collapsible) — Zone F                │
├──────────────────────────────────────────────────────────────┤
│ STATUS BAR (32px) — Zone G                                    │
└──────────────────────────────────────────────────────────────┘
```

Grid definition:

```css
body {
  display: grid;
  grid-template-rows: [header] 48px [wings-start] 1fr [command-start] 140px [log-start] auto [status-start] 32px;
  grid-template-columns: [left] 280px [main] 1fr [right] 340px;
  height: 100vh;
}
```

### 3.2 Zone A — Header (48px)

| Left | Center | Right |
|------|--------|-------|
| Skylink SVG logo + build tag | Link chips row (WS● SITL● MAV● WiFi●) + GPS fix quality + sim badge | System time + elapsed mission time |

Elements:
- `#zone-header` — full-width, grid row 1, columns 1/4
- `#link-chips` — flex row of `.link-chip` pills
  - Colors: green=connected, amber=connecting/degraded, red=disconnected, grey=N/A
  - Subtle pulse animation via CSS when active
- `#sim-badge` — amber pill, shown only when `simulation: true` in telemetry
- `#sys-clock` — shows HH:MM:SS, updated via `setInterval`
- `#mission-elapsed` — shows mission timer since arm

### 3.3 Zone B — Left Wing (280px fixed)

Scrollable if content exceeds height. Contains:

- B1: Compass/Heading Strip (horizontal, ~40px)
- B2: Attitude Indicator (~200px)
- B3: Altitude Tape (vertical, flex)
- B4: Speed Tape (vertical, flex)
- B5: Vertical Situation Display (compact, ~120px at bottom)

### 3.4 Zone C — Main Stage (flex, fills remaining width)

- Leaflet map container: `#map-stage`
- Map controls overlay (bottom-right): zoom +/-, follow drone, center home, fullscreen, geofence toggle
- HUD overlays: mode badge (top-left), armed status (top-right)
- GOTO context card (shown on map click)

### 3.5 Zone D — Right Wing (340px fixed)

Scrollable. Contains:

- D1: Preflight Checklist (compact, ~120px)
- D2: Telemetry Grid (2-column tile grid)
- D3: Sparklines Panel (4 miniature charts)
- D4: Status Messages (scrollable, max 8 lines)

### 3.6 Zone E — Command Bar (~140px)

Three rows:

| Row 1 | Mode selector + ARM/DISARM + TAKEOFF + LAND + RTL + LOITER |
|-------|-------------------------------------------------------------|
| Row 2 | Precision movement D-pad with distance presets + Yaw buttons |
| Row 3 | EMERGENCY STOP (KILL) button (right-aligned, visually separated) |

### 3.7 Zone F — Message Log Strip

Collapsed state: single line `[HH:MM:SS] ▶ ACK | TAKEOFF: ACCEPTED`
Expanded: full-height overlay with scrolling log, filterable by tag.

Initial state: collapsed (`32px`). Click to expand.

### 3.8 Zone G — Status Bar (32px)

Always visible last line:
```
Build: FW12·FS15 ✓ | Protocol v1 | Uptime: 01:23:45 | 5 Hz | RSSI: -65 dBm | IP: 192.168.1.100
```

---

## 4. Phase 3: Primary Flight Display (PFD)

### 4.1 Compass / Heading Strip (B1)

- Horizontal strip, full width of left wing
- Display N/S/E/W cardinal marks + current heading number in center
- Current heading highlighted with cyan
- When GOTO active: show target bearing as chevron

**Implementation:** Canvas element or pure CSS with positioned elements. Canvas preferred for smooth rotation.

### 4.2 Attitude Indicator (B2)

- Canvas-based artificial horizon, 160×160px minimum
- Sky/ground gradient (blue top, dark brown bottom)
- Pitch ladder lines every 10° (5° intermediate ticks)
- Roll arc at top
- Fixed center drone triangle (▲)
- Update at ~10fps by interpolating between 5Hz telemetry
- On no data: show red "NO ATTITUDE" overlay with X through instrument

**Implementation:** `<canvas id="att-canvas">` with `requestAnimationFrame` draw loop. Store roll, pitch, yaw in state. Draw horizon clipped to a circle.

### 4.3 Altitude Tape (B3)

- Vertical strip, ~80px wide
- Numeric scale that scrolls vertically as altitude changes
- Current altitude highlighted, large (24-36px)
- Color: green (2-30m) → amber (30-45m or <2m) → red (>=50m or <1m)
- Relative altitude as smaller secondary number
- Rate of climb/descent indicator (▲/▼ + numeric)

**Implementation:** DOM-based with CSS `transform: translateY()` on the scale. Efficient — only reposition, never recreate elements.

### 4.4 Speed Tape (B4)

- Same vertical tape pattern as altitude
- Ground speed + airspeed (if available)
- Units: m/s

### 4.5 Vertical Situation Display (B5)

- Compact side-view: drone icon at current altitude, home line at 0m
- Shows GOTO target altitude as dashed line
- ~280×120px at bottom of left wing

**Implementation:** Simple canvas or positioned divs inside a fixed-height container.

---

## 5. Phase 4: Map Stage

### 5.1 Map Setup (`gcs_map.js`)

- Leaflet `L.map()` with `zoomControl: false` (custom controls)
- Tile layer: CartoDB Dark Matter or custom dark OSM tiles.
  - Custom tile URL: `https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png`
- Attributions minimized to single-line.
- Initial view: lat/lng from telemetry, or default to 28.6°N, 77.2°E, zoom 17.

### 5.2 Map Overlays

| Overlay | Implementation | Details |
|---------|---------------|---------|
| Drone marker | `L.divIcon` with custom CSS chevron/triangle | 28×28px, rotates to match heading, glow pulse when receiving data |
| Home marker | `L.divIcon` "H" + dashed circle | Dashed circle radius = geofence distance |
| Flight path trail | `L.polyline` with gradient | Max 120 points, newer points brighter, `weight: 2`, `color: cyan` |
| Geofence | `L.circle` with dash array | Amber stroke, 3px, `fill: false` |
| Waypoints | `L.circleMarker` with number labels | Only if mission loaded from MAVLink |
| GOTO target | `L.marker` + line from drone | Pulldown pin animation, chevron line from drone to target |

### 5.3 Map Controls (bottom-right overlay)

Buttons as leaflet controls positioned with CSS:

- `[+]` Zoom in
- `[-]` Zoom out
- `[◎]` Follow drone toggle (auto-center)
- `[⌂]` Center on home
- `[⛶]` Fullscreen map (hides wings + dock temporarily)
- `[◯]` Show/hide geofence

All buttons are ghost-style (icon only, appear on hover of map area).

### 5.4 Map HUD Overlays (fixed position on map)

```
┌────────────────────────────────────┐
│ [STABILIZE]           [DISARMED]   │  ← translucent 85% dark bg
│                                     │
│              MAP                    │
│                                     │
│              [+] [-]
│              [◎] [⌂]
└────────────────────────────────────┘
```

- Top-left: mode pill (colored by mode, e.g., GUIDED=green, RTL=red)
- Top-right: armed status pill (ARMED=red, DISARMED=green)

### 5.5 GOTO Interaction

Click on map (when GUIDED + armed):
1. Show pin at click location
2. Show context card:
   ```
   ┌─────────────────────┐
   │ Fly to: 28.6139°N    │
   │        77.2090°E     │
   │ Distance: 45.2m      │
   │ Altitude: [10] m     │
   │ [GO] [Cancel]        │
   └─────────────────────┘
   ```
3. On GO: send `GOTO_LATLON` via WebSocket, remove card
4. On Cancel or Esc: remove card

---

## 6. Phase 5: Vehicle Status Panel (Right Wing)

### 6.1 Preflight Checklist (D1)

- 5 items in compact list, ~100px height
- Each item: `[●] WiFi link to ESP32`
- Colors: green=pass, red=fail, grey=waiting
- Animate icon transition ○→● when status changes

```html
<ul id="preflight-list">
  <li id="pf-wifi"><span class="pf-icon">○</span><span>WiFi link</span></li>
  <li id="pf-gps"><span class="pf-icon">○</span><span>GPS 3D fix</span></li>
  <li id="pf-mav"><span class="pf-icon">○</span><span>MAVLink</span></li>
  <li id="pf-bat"><span class="pf-icon">○</span><span>Battery &gt;20%</span></li>
  <li id="pf-safe"><span class="pf-icon">○</span><span>Disarmed</span></li>
</ul>
```

### 6.2 Telemetry Grid (D2)

2-column CSS grid of data tiles. Each tile:

```html
<div class="telem-tile">
  <span class="telem-label">ALTITUDE</span>
  <span class="telem-value" data-field="altitude">5.2</span>
  <span class="telem-unit">m</span>
  <div class="telem-sparkline" data-spark="altitude"><!-- canvas --></div>
</div>
```

| Tiles | Left | Right |
|-------|------|-------|
| Row 1 | Altitude (m) | Speed (m/s) |
| Row 2 | Battery (%) + bar | Voltage (V) |
| Row 3 | Heading (°) | GPS Sats |
| Row 4 | Distance from Home (m) | Link Quality |

Value flash animation: when value changes, briefly flash green (increase) or red (decrease) for 300ms.

### 6.3 Sparklines (D3)

4 miniature Canvas charts (100×40px each):

- Altitude history (last 60s / ~300 data points at 5Hz)
- Speed history
- Battery voltage trend
- Link RSSI trend

**Implementation:** Each sparkline is a `<canvas>` element with a simple line draw function. On each heartbeat, push new value, shift old ones, redraw. Thin line (1-2px), single accent color per sparkline. No axes, no labels — just trend.

### 6.4 Status Messages (D4)

- Filtered log showing only STATUS_TEXT and ACK messages from autopilot
- Max 8 visible lines, scrollable
- Color-coded: info (white), success (green), warning (amber), error (red)
- Click to open full log overlay

---

## 7. Phase 6: Command Bar (Bottom Dock)

### 7.1 Flight Control Row (E1)

```html
<div class="cmd-row">
  <select id="mode-select">
    <option>GUIDED</option><option>LOITER</option>
    <option>STABILIZE</option><option>LAND</option><option>RTL</option>
  </select>
  <button id="btn-arm" class="btn-arm">ARM</button>
  <button id="btn-disarm" class="btn-danger" hidden>DISARM</button>
  <button id="btn-takeoff" class="btn-primary">TAKEOFF 5m</button>
  <button id="btn-land" class="btn-amber">LAND</button>
  <button id="btn-rtl" class="btn-danger">RTL</button>
  <button id="btn-loiter" class="btn-blue">LOITER</button>
</div>
```

Button state rules:
- **Disabled** = `opacity: 0.35; pointer-events: none`
- **ARM** enabled only when: GUIDED mode + GPS 3D fix + battery >20% + WiFi + MAVLink + Disarmed
- **TAKEOFF** enabled only when: Armed + GUIDED
- **MOVE** enabled only when: Armed + GUIDED + Altitude >2m + GPS 3D fix
- **LAND / RTL** always enabled
- Tooltip on disabled buttons explains why (e.g., "Set GUIDED mode first")

### 7.2 Precision Movement Pad (E2)

D-pad layout:

```
           [▲ Forward]   (W)
[◄ Left]   [▼ Back]      [► Right]   (A/S/D)
           [▲ Up]        [▼ Down]    (Shift+W / Shift+S)

[Yaw ◤ -90°]    [Yaw ◢ +90°]    (Q / E)

Distance: [1m] [3m] [5m] [10m] [___ m]
```

- Active distance preset highlighted
- Custom distance input accepts any value
- Keyboard shortcuts shown as secondary text on buttons (`<kbd>` elements)
- All movement buttons disabled when preconditions not met

### 7.3 Emergency Section (E3)

- Wide red button with diagonal stripe pattern (CSS `repeating-linear-gradient`)
- "EMERGENCY STOP (KILL)"
- Two-click confirmation: first click arms the button (shows "CONFIRM?"), second click within 3s sends kill
- Always visible, right-aligned in command bar

---

## 8. Phase 7: WebSocket & State Layer

### 8.1 Central State Store (`gcs.js`)

A single `StateStore` object as a JavaScript class:

```js
class StateStore {
  constructor() {
    this.data = {
      vehicle: { armed: false, mode: '--', alt: 0, speed: 0, heading: 0,
                 roll: 0, pitch: 0, yaw: 0, battery: 0, voltage: 0 },
      gps: { lat: 0, lng: 0, sats: 0, fix: 'NO_FIX', homeValid: false },
      link: { wifi: false, sitl: false, mav: false, ws: false, rssi: 0 },
      system: { uptime: 0, simulation: false, build: '--' },
      messages: [],              // max 500
      sparklines: { alt: [], speed: [], battery: [], rssi: [] },
      preflight: { wifi: false, gps: false, mav: false, bat: false, armed: false },
      commandState: { lastCmd: null, lastAck: null, pending: false, timer: null }
    };
    this.listeners = new Map();  // field → Set<callback>
    this.history = { trail: [] }; // max 120 points for map trail
  }

  update(field, value) { /* set value, fire listeners */ }
  on(field, callback) { /* subscribe to field changes */ }
  batchUpdate(telemetry) { /* parse entire heartbeat, batch fire */ }
}
```

### 8.2 WebSocket Manager

```js
class WSManager {
  constructor(url) {
    this.url = url;
    this.reconnectAttempts = 0;
    this.maxReconnect = 50;  // ~5 minutes total
    this.connect();
  }

  connect() { /* create WebSocket, bind onopen/onmessage/onclose/onerror */ }
  onMessage(event) { /* parse JSON, call state.batchUpdate(), call map.update() */ }
  send(command, payload) { /* send JSON command, track command ID, start ack timer */ }
  handleAck(cmdId) { /* clear ack timer, fire success callback */ }
  handleTimeout() { /* show "NO RESPONSE" state on the button */ }
  reconnect() { /* exponential backoff: 2^attempt * 500ms, cap at 30s */ }
}
```

### 8.3 Telemetry Parsing

The heartbeat JSON from ESP32:

```json
{"v":1,"type":"event","event":"HEARTBEAT","altitude":5.2,"armed":false,
 "flight_mode":"GUIDED","roll":-0.5,"pitch":1.2,"yaw":134,
 "lat":28.6139,"lng":77.2090,"sats":12,"gps_fix":"3D",
 "battery":87,"battery_v":12.4,"uptime":1234,
 "wifi_connected":true,"sitl_connected":true,"mav_connected":true,
 "statustext":[],"fw_build":"FW12","fs_build":"FS15"}
```

Parse function extracts fields, normalizes, computes derived values:
- `distanceFromHome` via Haversine formula
- `preflight` checks (all items boolean)
- Push to sparkline arrays
- Push to trail array

### 8.4 Render Loop

No React diffing. Use `requestAnimationFrame` batch update:

```js
function renderLoop() {
  // Check if state changed since last frame
  if (state.dirty) {
    updateTelemetryTiles(state.changedFields);
    updateAttitudeIndicator(state.data.vehicle);
    updateMapPosition(state.data.gps);
    updatePreflight(state.data.preflight);
    updateCommandBar(state.data.vehicle);
    updateSparklines(state.data.sparklines);
    state.dirty = false;
  }
  requestAnimationFrame(renderLoop);
}
```

Individual component update functions only touch their own DOM nodes via `document.getElementById()` or stored refs. No virtual DOM, no innerHTML for dynamic values — use `textContent` and class switching.

### 8.5 State Persistence

| Data | Storage | When |
|------|---------|------|
| Map position/zoom | `localStorage` | On every pan/zoom |
| Panel layout | `localStorage` | Future feature |
| Last telemetry | `sessionStorage` | On heartbeat (for refresh recovery) |
| Active view level | `localStorage` | On change |
| Command history | `sessionStorage` | Future feature |

---

## 9. Phase 8: Keyboard System

### 9.1 Global Key Handler

```js
document.addEventListener('keydown', (e) => {
  if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;
  if (e.ctrlKey || e.metaKey) { /* handle Ctrl+N combos */ return; }

  switch (e.key.toLowerCase()) {
    case 'w':         move(0, 1, 0); break;   // Forward (body X)
    case 's':         move(0, -1, 0); break;  // Back
    case 'a':         move(-1, 0, 0); break;  // Left (body Y)
    case 'd':         move(1, 0, 0); break;   // Right
    case 'w': e.shiftKey && move(0, 0, 1);    // Shift+W = Up
    case 's': e.shiftKey && move(0, 0, -1);   // Shift+S = Down
    case 'q':         yawRelative(-90); break;
    case 'e':         yawRelative(90); break;
    case ' ':         e.preventDefault(); toggleArm(); break;
    case 't':         takeoff(); break;
    case 'l':         land(); break;
    case 'r':         rtl(); break;
    case 'f':         toggleFollow(); break;
    case 'c':         centerOnDrone(); break;
    case 'g':         enterGotoMode(); break;
    case 'm':         toggleFullscreenMap(); break;
    case '?':         showShortcutsModal(); break;
    case 'escape':    closeOverlay(); break;
    case '1': case '2': case '3': case '4': case '5':
                      setDistancePreset(parseInt(e.key)); break;
  }
});
```

### 9.2 Shortcut Reference Modal

- Triggered by `?` key or status bar button
- Clean modal overlay, dark background
- Keyboard shortcuts grouped by function (Flight, Camera, Map, System)
- Close on Esc or click outside

---

## 10. Phase 9: Animations & Feedback

### 10.1 CSS Animations

Define in `gcs.css`:

```css
@keyframes value-flash-green {
  0% { background-color: rgba(52, 211, 153, 0.3); }
  100% { background-color: transparent; }
}
@keyframes value-flash-red {
  0% { background-color: rgba(239, 68, 68, 0.3); }
  100% { background-color: transparent; }
}
@keyframes pulse-subtle {
  0%, 100% { opacity: 0.8; }
  50% { opacity: 1; }
}
@keyframes pulse-glow {
  0%, 100% { box-shadow: 0 0 4px var(--accent-cyan); }
  50% { box-shadow: 0 0 12px var(--accent-cyan); }
}
@keyframes slide-in-right {
  from { transform: translateX(100%); opacity: 0; }
  to { transform: translateX(0); opacity: 1; }
}
@keyframes slide-out-right {
  from { transform: translateX(0); opacity: 1; }
  to { transform: translateX(100%); opacity: 0; }
}
@keyframes pin-drop {
  0% { transform: translateY(-100px); opacity: 0; }
  60% { transform: translateY(5px); }
  100% { transform: translateY(0); opacity: 1; }
}
@keyframes reconnect-dot {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 1; }
}
```

### 10.2 Animation Trigger Map

| Element | Animation | When | Duration |
|---------|-----------|------|----------|
| Telemetry value | `value-flash-green` / `value-flash-red` | Value changes | 300ms |
| Button press | CSS `transform: scale(0.97)` → `scale(1)` | On click | 100ms |
| Link chip | `pulse-subtle` | While connected | 2s cycle |
| Drone marker | `pulse-glow` | While receiving telemetry | 1.5s cycle |
| Map pin | `pin-drop` | Map click | 400ms |
| Preflight icon | CSS class swap ○→● | Status changes | 200ms |
| Battery bar | CSS `transition: width 300ms` | Value changes | 300ms |
| Toast | `slide-in-right` + delay + `slide-out-right` | Event | 4s total |
| Command button border | CSS `box-shadow` pulse | Waiting for ACK | Until ACK |
| Panel transition | `opacity + translateY` | View level change | 200ms |

### 10.3 Command Feedback Loop

```
1. Click ARM → button scale(0.97) [100ms]
2. button class="btn-pending" → "ARMING..." + amber border pulse
3. WebSocket send → log shows "TX: ARM_DRONE"
4. ACK received → button class="btn-success" → "ARMED" + green flash
5. No ACK in 2s → button class="btn-error" → "NO RESPONSE" in red
```

All `btn-pending`, `btn-success`, `btn-error` classes defined in CSS with transitions.

---

## 11. Phase 10: Mobile & Responsive

### 11.1 Breakpoints

| Viewport | Layout | Implementation |
|----------|--------|---------------|
| >1600px | Full 3-column | Default CSS grid |
| 1024-1600px | 3-column, compact spacing | Reduce left/right wing widths by 20px each, smaller font sizes |
| 768-1024px | 2-column: Map + stacked PFD/Status | CSS grid change: left wing stacks on top of right wing, map stays full height |
| <768px | Single column, stacked | Map top 50vh, PFD + status below in scrollable strip, floating bottom sheet for commands |

### 11.2 Mobile Adaptations

- `<768px`:
  - Map fills top 50% of viewport
  - PFD + status panels below in a scrollable strip
  - Command bar becomes a floating bottom sheet (slides up on tap)
  - All buttons min 44×44px touch targets
  - Keyboard shortcuts replaced by touch gestures:
    - Swipe up = increase altitude
    - Swipe down = decrease altitude
    - Tap map = set GOTO
    - Long-press ARM = arm
  - No preflight checklist on mobile (monitoring-only view)
  - Status bar collapsed, expandable on tap

### 11.3 CSS Grid Changes

Use `@media` queries to redefine the body grid at each breakpoint.

---

## 12. Phase 11: Connection Loss & Error Handling

### 12.1 Loss Timeline

| Time | Action |
|------|--------|
| 0ms | WebSocket `onclose` fires |
| 0-500ms | Link chip turns red, status bar shows "DISCONNECTED" |
| 0-5s | Map freezes (last position frozen), telemetry shows last values with `---` animation |
| 5s | Dark overlay appears over map only (left/right panels still visible) |
| 5s+ | Pulsing "RECONNECTING" text + animated dots `●○○` → `●●○` → `●●●` |
| 30s+ | Overlay changes: "Operating in offline mode. Last update: [time]" + manual reconnect button |
| Reconnect | All values snap to current with green flash, overlay fades out |

### 12.2 CSS for Connection States

```css
.connected    { --link-color: var(--accent-green); }
.degraded     { --link-color: var(--accent-amber); }
.disconnected { --link-color: var(--accent-red); }

.telemetry-stale .telem-value { opacity: 0.5; }
.telemetry-stale .telem-value::after { content: '---'; animation: blink 1s infinite; }
```

### 12.3 Reconnection Logic

Exponential backoff: `min(2^attempt * 500, 30000)` ms. Max 50 attempts. Manual reconnect button always visible after first failure.

---

## 13. Phase 12: Build, Optimize & Deploy

### 13.1 esbuild Configuration

```json
{
  "entryPoints": ["data/index.html"],
  "outdir": "dist",
  "minify": true,
  "legalComments": "none",
  "target": "es2020"
}
```

Separate build steps:
1. `esbuild` minify JS files individually (no bundling — keep files separate for caching)
2. `html-minifier-terser` on `index.html`
3. `csso` or `clean-css` on `gcs.css`
4. GZip all assets to `.gz` for ESP32 serving

### 13.2 Asset Size Budget

| Asset | Target size | Notes |
|-------|-------------|-------|
| `index.html` | <5KB | Minimal, semantic HTML |
| `gcs.css` | <20KB | Purged, no unused rules |
| `gcs.js` + `gcs_map.js` + `gcs_components.js` | <60KB total | ES2020 target, minified |
| `gcs_config.js` | <1KB | Just the URL + settings |
| JetBrains Mono subset | ~30KB woff2 | Latin + numbers + punctuation |
| Inter subset | ~40KB woff2 | Required weights only |
| Leaflet (JS + CSS) | ~45KB | Already minified |
| Leaflet tile images | ~3KB | |
| SVG logo + icons | <5KB | Inline SVG |
| **Total** | **<210KB** | Well under 500KB budget |

### 13.3 ESP32 Deployment

1. Build all minified assets to `dist/`
2. Run `esp32-littlefs-tool` or upload via web interface
3. The ESP32 serves files from LittleFS with `gzip` content-encoding for `.gz` files
4. Verify: open browser DevTools → Network tab → confirm all assets <500KB transferred

### 13.4 Performance Verification

| Check | Target | How |
|-------|--------|-----|
| Lighthouse Performance | 95+ | Run Lighthouse audit |
| Total transfer size | <500KB | DevTools Network tab |
| Telemetry → DOM | <20ms | `performance.now()` markers |
| Frame rate | 60fps | DevTools Performance tab |
| Memory | <80MB | DevTools Memory tab |
| Map pan/zoom | 60fps | Manual test |

---

## 14. File Manifest

| File | Location | Purpose | Approx size |
|------|----------|---------|-------------|
| `index.html` | `data/index.html` | Single HTML file, all zones in DOM | ~4KB |
| `gcs.css` | `data/gcs.css` | All styles: tokens, grid, components, animations | ~18KB |
| `gcs.js` | `data/gcs.js` | Core: StateStore, WSManager, render loop, keyboard, commands | ~25KB |
| `gcs_map.js` | `data/gcs_map.js` | Leaflet setup, overlays, trail, GOTO interaction | ~12KB |
| `gcs_components.js` | `data/gcs_components.js` | PFD, attitude indicator, telemetry tiles, sparklines, preflight | ~15KB |
| `gcs_config.js` | `data/gcs_config.js` | WebSocket URL, defaults, presets | ~1KB |
| `fonts/jetbrains-mono.woff2` | `data/fonts/` | JetBrains Mono subset | ~30KB |
| `fonts/inter.woff2` | `data/fonts/` | Inter subset | ~40KB |
| `lib/leaflet/leaflet.js` | `data/lib/leaflet/` | Leaflet 1.9.x | ~40KB |
| `lib/leaflet/leaflet.css` | `data/lib/leaflet/` | Leaflet styles (minified) | ~5KB |
| `favicon.ico` | `data/favicon.ico` | Favicon | ~1KB |

Total: ~190KB uncompressed, ~75KB gzipped (well under 500KB budget).

---

## 15. Order of Implementation

**Do not skip ahead.** Each phase depends on the previous one.

### Phase 1 — Foundation (files: `index.html` skeleton + `gcs.css` tokens)
1. Create `index.html` with the full zone structure (empty containers, all 7 zones)
2. Create `gcs.css` with all design tokens, reset, grid layout
3. Create `gcs_config.js` with default WS URL
4. Self-host font subsets

### Phase 2 — Static Layout (files: `index.html` + `gcs.css`)
1. CSS Grid for the 3-column body layout
2. Header (Zone A) with static content, link chips, clock
3. Left Wing (Zone B) with placeholders for each PFD component
4. Right Wing (Zone D) with static telemetry tiles, preflight list
5. Command Bar (Zone E) with all buttons (disabled)
6. Message strip (Zone F) and Status Bar (Zone G)
7. Verify layout renders correctly at 1920×1080 and 1366×768

### Phase 3 — Map (files: `gcs_map.js` + `index.html`)
1. Initialize Leaflet map in Zone C
2. Custom dark tile layer
3. Drone marker (divIcon, rotating)
4. Home marker + geofence circle
5. Flight path trail (polyline)
6. Map controls (zoom, follow, center, fullscreen)
7. Map HUD overlays (mode + armed badges)
8. GOTO click interaction + context card

### Phase 4 — WebSocket & State (files: `gcs.js`)
1. StateStore class with all fields and listeners
2. WSManager class with connect/reconnect/send
3. Telemetry parser that populates state
4. `batchUpdate()` that marks dirty fields
5. Minimal render loop that updates DOM
6. Test with SITL — verify telemetry appears in DOM

### Phase 5 — Components (files: `gcs_components.js`)
1. AttitudeIndicator (Canvas, 160×160, pitch ladder, roll arc)
2. AltitudeTape (DOM-based, scrolling scale, color coding)
3. SpeedTape (same pattern)
4. CompassStrip (Canvas or DOM, heading + cardinal marks)
5. VSD (Canvas, side-view with drone + target altitude)
6. TelemetryTile constructor (label + value + unit + flash animation)
7. PreflightList (status icons with class switching)
8. Sparkline constructor (Canvas, 60s history)
9. CommandBar updater (enabled/disabled states, mode selector)
10. MessageLog (scrollable, color-coded, filtering)

### Phase 6 — Commands & Keyboard (files: `gcs.js`)
1. Wire all command bar buttons to `sendCommand()`
2. Global keyboard handler with all shortcuts
3. Command feedback loop (pending → success/error)
4. GOTO flow (enter mode → click map → confirm)
5. KILL button (two-click confirmation)

### Phase 7 — Animations & Polish (files: `gcs.css` + `gcs_components.js`)
1. Add all CSS keyframes
2. Wire value flash animations to telemetry tile updates
3. Link chip pulse animation
4. Connection loss overlay (5s trigger, pulsing reconnect dots)
5. Toast notification system
6. Button press feedback (scale)
7. Map pin drop animation
8. Keyboard shortcut modal (`?` key)
9. First-visit guided tour (6 tooltips, dismissable)
10. `prefers-reduced-motion` support

### Phase 8 — Error Handling & Edge Cases (files: `gcs.js`)
1. No-telemetry empty state overlay
2. Connection loss timeline (5s → 30s → offline mode)
3. Exponential backoff reconnection
4. Command timeout (2s → "NO RESPONSE")
5. All buttons disabled when disconnected
6. sessionStorage recovery on page refresh

### Phase 9 — Responsive (files: `gcs.css`)
1. 1024-1600px: compact spacing
2. 768-1024px: 2-column (map + stacked panels)
3. <768px: single column, floating command sheet, touch gestures
4. Test all breakpoints

### Phase 10 — Build & Deploy
1. esbuild minification
2. Asset size verification (<500KB)
3. Deploy to ESP32 LittleFS
4. Lighthouse audit (target 95+)
5. Full SITL test: arm → takeoff → move → land → RTL

---

## Appendix: Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Vanilla JS over Svelte** | The user already maintains vanilla JS. Adding a compiler introduces build complexity and a learning curve for future maintenance. The ESP32 constraint (<500KB) is achievable with vanilla JS + minification. |
| **Separate component file** | `gcs_components.js` keeps component constructors isolated from app logic. Each component is a function that takes a container element + state reference and returns an update method. |
| **Canvas for attitude indicator** | The pitch ladder and roll arc require per-frame redraws. Canvas is more performant than DOM transforms for this use case at 10fps. |
| **sessionStorage for refresh recovery** | WebSocket state is ephemeral. On refresh, the dashboard shows the last known telemetry from sessionStorage while reconnecting — prevents a blank screen. |
| **No CSS frameworks** | Tailwind requires purging and adds build complexity. CSS custom properties provide all the theming power needed in a single file. |
| **Incremental approach** | Each phase produces a testable, working dashboard. At any point, the dashboard is functional (even if not complete). This avoids the "big bang" rewrite risk. |

---

*"Command the sky."*