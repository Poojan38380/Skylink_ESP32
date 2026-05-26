# Skylink GCS — The New Dashboard: Vision, Brand & Design

> **Document purpose:** This document is the single source of truth for the complete redesign of the Skylink Ground Control Station web dashboard. It contains brand philosophy, UI/UX principles, layout specifications, interaction patterns, and visual design direction. Use this as the prompt for AI agents tasked with building the next-generation dashboard.
>
> **Target:** A Bloomberg Terminal-grade GCS — dense with data, beautiful in darkness, effortless in operation.

---

## Table of Contents

1. [Project Context — What Skylink Is](#1-project-context)
2. [Why a Full Redesign?](#2-why-a-full-redesign)
3. [Brand & Identity](#3-brand--identity)
4. [Design Inspirations](#4-design-inspirations)
5. [Layout Philosophy — The Bloomberg Grid](#5-layout-philosophy)
6. [The Complete Layout Blueprint](#6-the-complete-layout-blueprint)
7. [User Interface Components](#7-user-interface-components)
8. [User Experience Principles](#8-user-experience-principles)
9. [Usability & Learnability](#9-usability--learnability)
10. [Interactions & Animation](#10-interactions--animation)
11. [Data Visualization Principles](#11-data-visualization-principles)
12. [Mobile & Responsive Strategy](#12-mobile--responsive-strategy)
13. [Technical Considerations](#13-technical-considerations)
14. [Design System Tokens](#14-design-system-tokens)
15. [Appendix: Reference Dashboards](#15-appendix-reference-dashboards)

---

## 1. Project Context

### What is Skylink?

Skylink is an **ESP32-based WiFi MAVLink bridge** that connects a browser-based Ground Control Station (GCS) to an ArduPilot flight controller. It supports two modes:

- **SITL Mode** — TCP connection to ArduPilot SITL (simulation) running on a PC
- **Hardware Mode** — UART2 serial connection to a Pixhawk 2.4.8 flight controller

The ESP32 serves a WebSocket-powered dashboard from its LittleFS filesystem, pushing telemetry at **5 Hz** and accepting command inputs.

### Current System Architecture

```
Browser (HTML/CSS/JS)  ←──WebSocket──→  ESP32 (AsyncWebServer)
                                              │
                                              ├── MAVLink TCP → ArduPilot SITL
                                              └── MAVLink UART → Pixhawk 2.4.8
```

### Data Streamed to Dashboard (5 Hz heartbeat JSON)

| Category | Fields |
|----------|--------|
| **Vehicle State** | armed, flight_mode, flight_mode_name, altitude, relative_alt, speed |
| **Attitude** | roll, pitch, yaw |
| **GPS** | lat, lng, sats, gps_fix, home_valid, home_lat, home_lng |
| **Battery** | battery (%), battery_v |
| **Link Status** | wifi_connected, wifi_rssi, sitl_connected, mav_connected, sitl_tcp_connected, ws_connected |
| **System** | uptime, timestamp, simulation flag, fw_build, fs_build |
| **Messages** | statustext[] (status messages from flight controller) |

### Commands (WebSocket → ESP32)

ARM_DRONE, DISARM_DRONE, SET_FLIGHT_MODE, TAKEOFF, LAND, RTL, MOVE_BODY (x/y/z), YAW_RELATIVE, GOTO_LATLON, LOITER_HERE, RC_OVERRIDE, PING, LED_SET, LED_TOGGLE

---

## 2. Why a Full Redesign?

The current prototype served its purpose — proving the concept. But it has critical shortcomings:

| Problem | Impact |
|---------|--------|
| **Tab-based navigation** | Only one screen visible at a time. A pilot needs altitude AND map AND telemetry simultaneously. |
| **No persistent awareness** | Switching tabs loses context. You cannot watch the map while adjusting settings. |
| **Low information density** | Large empty spaces, minimal data per view. A GCS should feel *alive* with data. |
| **No hierarchy of urgency** | All data treated equally. No visual prioritization of critical vs. informational. |
| **Minimal visual polish** | Functional but not *inspiring*. A GCS should feel like a cockpit — every pixel purposeful. |
| **No keyboard-first workflow** | Dependable on mouse clicks. Real operators need keyboard muscle memory. |
| **No multi-monitor awareness** | Designed for a single small viewport, not the multi-screen setups operators actually use. |

**The new design must feel like stepping into a NASA mission control center, not browsing a settings page.**

---

## 3. Brand & Identity

### Brand Name
**Skylink** — concise, technical, evocative of sky + connection/link/bridge

### Tagline
*"Command the sky."* — Short, bold, aspirational. Or more descriptive: *"Ground Control for the Next Generation."*

### Brand Personality

| Attribute | Manifestation |
|-----------|---------------|
| **Authoritative** | Every pixel signals "this is a serious tool." No frivolous decoration. |
| **Precise** | Numerical accuracy, clean typography, sharp edges. |
| **Calm** | Dark backgrounds, controlled color usage, no visual panic except for genuine alerts. |
| **Forward-leaning** | Feels like a sci-fi cockpit from the near future — not retro, not gaudy, just advanced. |
| **Resilient** | Connection-loss handling is graceful, never panicked. The UI stays calm when the link drops. |

### Color Philosophy

The palette is inspired by **night-vision-compatible military displays**, **diving watch luminescence**, and **Bloomberg's iconic amber-on-black**.

**Dark base** — not just "dark mode" but a deliberate *black-background* environment. True black (#000000 or very dark #050505) as the canvas. This:
- Reduces eye strain during long operations
- Makes LEDs and status indicators pop like real hardware
- Creates the illusion of infinite depth — widgets float on a void
- Saves power on OLED displays
- Matches every aerospace/military cockpit ever built

### Logo & Iconography

- **Primary logo:** A minimalist geometric satellite/drone silhouette — think 3-4 vector lines maximum. Should work as a favicon at 16×16.
- **Secondary mark:** The letters "SK" in an angular, custom logotype reminiscent of aerospace stencils.
- **Icon set:** Use a custom set of outlined, consistent icons (feather-style or phosphor-style). No emoji, no irregular weights. Every icon must be meaningful at 14px.

---

## 4. Design Inspirations

### Primary: Bloomberg Terminal

The Bloomberg Terminal is the gold standard for professional data dashboards. Why it works:

- **Multi-pane, always-visible layout** — Up to 6+ panels visible simultaneously. No tab switching for core data.
- **Keyboard-driven** — Every function accessible via 2-3 keystrokes. No hunting through menus.
- **Color as data** — Green = up/positive, Red = down/negative, Yellow/Amber = warning. Colors encode meaning instantly.
- **Information density** — Beautiful density. Every screenful delivers maximum useful data without feeling cluttered.
- **Consistent visual grammar** — Same fonts, same grid, same hierarchy everywhere. Learn one pane, learn them all.
- **Amber/Green phosphor aesthetic** — The iconic CRT-monitor look has been refined to a modern, readable palette.

**What to borrow for Skylink:**
- The multi-column grid layout (but adapted for a drone GCS)
- Color-as-status encoding
- Keyboard shortcut culture
- Dense-but-readable data presentation
- Status bar at bottom showing link quality, time, system health

### Secondary: NASA Mission Control / SpaceX Dragon UI

- **Contextual information grouping** — Telemetry grouped by subsystem (Propulsion, Guidance, Electrical, Thermal). For drones: Flight, Navigation, Power, Link.
- **Glass-cockpit philosophy** — The best HUDs show exactly what you need, exactly when you need it. No information, no matter how cool, is shown unless it serves a decision.
- **Redline indicators** — When a value exceeds safe limits, the entire value cell glows red/orange immediately. Not a separate alert box — the data itself warns you.
- **Clean sans-serif typography** — NASA uses fixed-width for numbers (for alignment) and sans-serif for labels.

### Tertiary: Trading Dashboards (Robinhood, TD Ameritrade Thinkorswim)

- **Speed of data rendering** — These dashboards update hundreds of numbers per second without flicker.
- **Sparklines** — Tiny inline charts that show trends without taking space. Altitude history as a sparkline next to the altitude number.
- **Flexible layout** — Thinkorswim lets you drag panes, resize them, tear them off into separate windows.

### Quaternary: Tesla In-Car UI

- **Minimal chrome** — No borders, no shadows, no gradients. Content IS the interface.
- **Context-adaptive controls** — Controls appear only when they're usable. The "Takeoff" button doesn't show until GUIDED mode is active.
- **High-contrast at all times** — Perfect readability in direct sunlight (or dark rooms).

### Quinary: Military Avionics (F-35 Panoramic Cockpit Display / Honeywell Primus Epic)

- **Declutter and re-clutter** — The ability to hide non-critical data during high-workload phases (takeoff, landing), then show it again during cruise.
- **Persistence of critical data** — Altitude, attitude, heading, and airspeed are ALWAYS visible. Always. Top of the screen, always.
- **Redundancy of critical information** — Altitude shown as a number AND a vertical tape AND on the map. Three ways to know.

---

## 5. Layout Philosophy — The Bloomberg Grid

### Core Principle: No Tabs. All Data, All The Time.

The new Skylink GCS is a **single-page, multi-pane layout** where every major data category has its own dedicated zone. Nothing hides behind a tab. The operator's eyes move, not their hands.

### The Layout Metaphor

Think of the screen as an **HUD overlay on a virtual cockpit window**:

```
┌──────────────────────────────────────────────────────────────────┐
│  STATUS BAR                                                        │
│  [Skylink Logo] [Link: WS● SITL● MAV●] [GPS: 3D Fix] [Time]      │
├────────────────────┬───────────────────────┬──────────────────────┤
│                    │                       │                        │
│   LEFT WING        │    MAIN STAGE          │   RIGHT WING          │
│   (Flight Data)    │    (Map / 3D View)     │   (Vehicle Status)    │
│                    │                       │                        │
│  ┌──────────────┐  │  ┌─────────────┐      │  ┌──────────────────┐  │
│  │ Attitude     │  │  │             │      │  │ Preflight Check  │  │
│  │ Indicator    │  │  │  Map View   │      │  │ [✓] WiFi         │  │
│  │   ┌──┐       │  │  │  (Leaflet)  │      │  │ [✓] GPS          │  │
│  │   │▲│       │  │  │             │      │  │ [✓] MAVLink      │  │
│  │   └──┘       │  │  │  Drone pos  │      │  │ [✓] Battery      │  │
│  │ Roll: 2.3°   │  │  │  Trail      │      │  │ [✓] Disarmed     │  │
│  │ Pitch: -1.1° │  │  │  Geofence   │      │  └──────────────────┘  │
│  │ Yaw: 134°    │  │  │             │      │                        │
│  └──────────────┘  │  └─────────────┘      │  ┌──────────────────┐  │
│                    │                       │  │ Telemetry Grid   │  │
│  ┌──────────────┐  │                       │  │ Alt: 5.2m        │  │
│  │ Altitude Tape│  │                       │  │ Spd: 0.8 m/s     │  │
│  │  ┌──────┐    │  │                       │  │ Bat: 87% ████░   │  │
│  │  │ 5.2m │    │  │                       │  │ Sats: 12         │  │
│  │  └──────┘    │  │                       │  │ HDG: 134°        │  │
│  │  [=====●==]  │  │                       │  └──────────────────┘  │
│  └──────────────┘  │                       │                        │
│                    │                       │  ┌──────────────────┐  │
│  ┌──────────────┐  │                       │  │ Sparkline Grid   │  │
│  │ Speed Tape   │  │                       │  │ ╱╲╱╲╱╲ Altitude  │  │
│  │  0.8 m/s     │  │                       │  │ ╲╱╲╱╲╱ Speed     │  │
│  │  [████░░░░]  │  │                       │  │ ╱╲╱╲╱╲ Battery   │  │
│  └──────────────┘  │                       │  └──────────────────┘  │
├────────────────────┴───────────────────────┴──────────────────────┤
│  BOTTOM DOCK — COMMAND BAR                                          │
│  [GUIDED ▼] [ARM] [TAKEOFF 5m] [LAND] [RTL] [LOITER]               │
│  ┌───────────── Keyboard: WASD=Move, QE=Yaw, Space=Arm ─────────┐  │
├──────────────────────────────────────────────────────────────────┤
│  MESSAGE LOG — single-line strip, expandable on hover/click       │
│  [14:32:05] ACK: TAKEOFF accepted  │  [14:32:04] FC: Pre-arm OK  │
├──────────────────────────────────────────────────────────────────┤
│  STATUS BAR — always visible                                        │
│  Uptime: 01:23:45 | Build: FW12·FS15 | 5 Hz | Signal: -65 dBm     │
└──────────────────────────────────────────────────────────────────┘
```

---

## 6. The Complete Layout Blueprint

### 6.1 Screen Canvas

The entire dashboard is designed for **1920×1080** minimum. It scales down gracefully to 1366×768 and up to 4K. The layout is a **fixed grid of resizable panels** — no scrolling for primary content.

### 6.2 Zone Breakdown

#### ZONE A: Status Bar (Top — 48px fixed height)

Perma-visible strip. Never hides. Ever.

| Left third | Center | Right third |
|------------|--------|-------------|
| Skylink logo + build tag | Link chips (WS● SITL● MAV● WiFi●) with status colors | System time (NTP-synced) |
| | GPS fix quality indicator | Connection latency (ms) |
| | Simulation mode badge (amber) | Mission elapsed time |

**Link chips** are small rounded pills, colored:
- **Green** = connected & healthy
- **Amber** = connecting / degraded
- **Red** = disconnected
- Grey = not applicable

Each chip pulses subtly when active — the pulse communicates "alive" without requiring the operator to read the text.

#### ZONE B: Left Wing — Flight Data (280px fixed width)

Always visible. Non-negotiable. This is the **Primary Flight Display (PFD)** zone.

**B1: Attitude Indicator (Artificial Horizon)**
- A proper head-down attitude indicator, not the current small bubble
- Sky/ground gradient with pitch ladder markings (every 10°)
- Roll indicator with bank angle markings
- Drone symbol fixed center
- Dimensions: 160×160px minimum
- **Improvement over current:** The current bubble is 64px and barely readable. This needs to be a real instrument.

**B2: Altitude Tape (vertical strip)**
- Vertical numeric scale with the current altitude highlighted
- Color coding: green (safe zone) → amber (caution) → red (too low/too high)
- Relative altitude shown as a smaller secondary number
- Rate of climb/descent indicator (small arrow + numeric)

**B3: Speed Tape (vertical strip)**
- Vertical numeric scale
- Ground speed (GPS-derived)
- Airspeed if available from MAVLink (AirSpeed message)

**B4: Compass / Heading Strip (horizontal strip at top of wing)**
- 360° horizontal compass with cardinal directions
- Current heading highlighted
- Target bearing shown as a chevron when a GOTO is active

**B5: Vertical Situation Display (VSD)**
- A simplified side-view showing:
  - Drone altitude relative to home
  - Terrain proximity (if available)
  - Target altitude for active GOTO
- Compact: ~280×120px

#### ZONE C: Main Stage — Map (flexible, fills remaining width)

This is the largest zone and the primary spatial awareness tool.

**C1: Map Layer (Leaflet with custom dark tile style)**
- Default tiles: CartoDB Dark Matter (no labels) or custom OpenStreetMap with a monochrome/amber render
- No colorful map pins or tourist attractions — this is an operational map
- Custom dark tile style: ocean = very dark blue (#0a1628), land = dark grey (#1a1d23), roads/subtle in low opacity

**C2: Map Overlays**
- **Drone marker:** A directional chevron/triangle pointing in heading direction. Size: 28×28px. With a subtle glow when recently updated.
- **Home marker:** A house icon or "H" with a dashed circle around it
- **Geofence circle:** Dashed line, amber, with radius label
- **Flight path trail:** Gradient line from transparent to full opacity (newer = brighter). Max 120 points.
- **Waypoints:** Small numbered circles if a mission is loaded
- **GOTO target:** Pulldown marker at the clicked location with a chevron line from drone to target

**C3: Map Controls (bottom-right of map zone, always visible)**
- **Zoom** + / — buttons
- **Follow drone** toggle (auto-centers map on drone position)
- **Center on home** button
- **Fullscreen map** button (expands map to full viewport, hides other zones temporarily)
- **Show/Hide geofence** toggle

**C4: Map HUD Overlays (data rendered ON the map, not below it)**
- Top-left: Mode badge (STABILIZE / GUIDED / RTL) with color
- Top-right: Armed status (ARMED in red, DISARMED in green)
- Overlay opacity: 85% dark background, never obstructs the map

**Interaction: Click on map to set GOTO target**
- Clicking shows a pin with a context card:
  ```
  ┌───────────────────┐
  │ Fly to: 28.6139°N │
  │        77.2090°E  │
  │ Distance: 45.2m   │
  │ Altitude: [10] m  │ ← editable
  │ [GO] [Cancel]     │
  └───────────────────┘
  ```

#### ZONE D: Right Wing — Vehicle Status (340px fixed width)

Always visible. The systems status panel.

**D1: Preflight Checklist (compact)**
- 5-6 items in a compact list
- Each item: [●] label
- Colors: green = pass, red = fail, grey = waiting
- Show only the CURRENT state — don't show historical passes/fails
- Compact enough that 5 items fit in ~100px of vertical space

**D2: Telemetry Grid (compact tiles)**
- 2-column grid of data tiles
- Each tile: Label (small, uppercase, muted) + Value (large, bright) + Unit
- Tile animations: value changes get a brief green flash (increase) or red flash (decrease)

| Left Column | Right Column |
|-------------|--------------|
| Altitude (m) | Speed (m/s) |
| Battery (%) | Voltage (V) |
| Heading (°) | GPS Sats |
| Distance from Home (m) | Link Quality |

**D3: Sparklines Panel**
- Four miniature line charts (100×40px each):
  - Altitude history (last 60 seconds)
  - Speed history
  - Battery voltage trend
  - Link RSSI trend
- Thin lines, single color per sparkline, no axes, just trend

**D4: Status Messages (scrollable, max 8 lines)**
- Filtered view of important autopilot messages
- NOT a raw log — only STATUS_TEXT and ACK messages
- Click to expand to full log view
- Messages color-coded: info (white), success (green), warning (amber), error (red)

#### ZONE E: Bottom Dock — Command Bar (variable height, ~120px)

The action center. Always visible.

**E1: Flight Control Row**
```
[GUIDED ▼] — mode selector dropdown, compact
[ARM] — green button, only enabled when preflight passes
[DISARM] — red button, only enabled when armed
[TAKEOFF 5m ▼] — with altitude preset dropdown
[LAND] — amber
[RTL] — red (emergency)
[LOITER] — blue
```

Button states:
- **Enabled**: full color, clickable
- **Disabled**: dimmed to 35% opacity
- **Armed**: ARM button replaced by DISARM button
- **Moving**: all movement buttons pulse briefly on command send

**E2: Precision Movement Pad**
```
          [▲ Forward]   (W key)
[◄ Left]  [▼ Back]      [► Right]   (A/S/D keys)
          [▲ Up]        [▼ Down]    (Shift+W / Shift+S)
          
[Yaw ◄ -90°]      [Yaw ► +90°]      (Q / E keys)

Distance: [1m] [3m] [5m] [10m] [Custom: ___ m]
```

- Compact D-pad layout, not the current sprawling one
- Active distance highlighted
- Keyboard shortcuts shown directly on the buttons as secondary text

**E3: Emergency Section (right side of dock, visually separated)**
- **EMERGENCY STOP (KILL)** — a single, wide, red button with diagonal stripes pattern
- Requires confirmation: click once to arm the button, click again within 3s to execute
- Always visible, always at the right edge

#### ZONE F: Message Log Strip (collapsible, ~32px when collapsed)

A single-line strip at the bottom showing the most recent message. Click to expand into a full-screen overlay log.

- Collapsed: `[14:32:05] ▶ ACK | TAKEOFF: ACCEPTED`
- Expanded overlay: Full scrolling log with timestamp, tag, message tri-column layout
- Auto-scrolls to newest
- Filterable by tag (SYS, FC, ACK, ERR, CMD)
- Max 500 entries in memory, older entries purged

#### ZONE G: Status Bar (Bottom — 32px fixed height)

Always visible last line:
```
Build: FW12·FS15 ✓ | Protocol v1 | Uptime: 01:23:45 | 5 Hz Telemetry | RSSI: -65 dBm | IP: 192.168.1.100
```

---

## 7. User Interface Components

### 7.1 Typography

| Usage | Font | Size | Weight | Case |
|-------|------|------|--------|------|
| Primary telemetry values | JetBrains Mono | 24–36px | Bold | Default |
| Secondary values | JetBrains Mono | 14–18px | SemiBold | Default |
| Labels (small) | Inter or SF Pro Text | 10–11px | Medium | Uppercase, 0.5px letter-spacing |
| Section headers | Inter | 11–12px | SemiBold | Uppercase, 1px letter-spacing |
| Status bar | JetBrains Mono | 11px | Regular | Default |
| Button labels | Inter | 12–13px | SemiBold | Sentence case |
| Log text | JetBrains Mono | 11px | Regular | Default |

**Why monospace for numbers?** Numeric values align perfectly column-wise. In any grid of telemetry, the decimal points line up. This is non-negotiable for altitude, speed, GPS coordinates.

**Why Inter for labels?** Better readability at small sizes for proportional text. The UI needs both extremes — tiny labels and large numbers.

### 7.2 Color System

```
Background (deep):      #050505  — the void. True dark.
Background (panel):     #0A0E14  — slightly lighter than void for panels
Background (elevated):  #11161E  — hovered/active panels
Border (subtle):        #1A1F2E  — 1px, very faint
Border (active):        #2D354A  — selected/focused elements

Text (primary):         #E8EDF5  — main data values
Text (secondary):       #8B95A5  — labels, hints, units
Text (muted):           #4A5568  — disabled, placeholder

Accent (cyan):          #00D4FF  — Skylink primary. Connection, telemetry, active state
Accent (green):         #34D399  — Success, armed, good, healthy
Accent (amber):         #FBBF24  — Warning, caution, degraded, simulation
Accent (red):           #EF4444  — Error, critical, disarmed, kill
Accent (purple):        #A78BFA  — Command acknowledged, mode change
Accent (blue):          #60A5FA  — Information, loiter, neutral state
```

**Gradient usage:** Minimally. A subtle cyan-to-blue gradient for the Skylink logo mark. A green-to-red gradient for battery bar. That's it. No glossy gradients, no glassmorphism.

### 7.3 Components & Patterns

#### Buttons

| Type | Style | Use |
|------|-------|-----|
| Primary | Solid cyan bg, white text | GO, Confirm, primary action |
| Secondary | Dark bg, 1px border, cyan text on hover | Cancel, alternate action |
| Arm | Solid green bg, white text | ARM — only when preflight passes |
| Danger | Solid red bg, white text | DISARM, KILL |
| Flight move | Dark bg, amber border, amber text | MOVE directions |
| Mode pill | Small pill, colored by mode | Displaying current flight mode |
| Map control | Ghost button, icon only, appears on hover | Zoom, center, follow |

#### Cards / Tiles

Every telemetry tile follows the same pattern:

```
┌──────────────────────┐
│ LABEL                │  ← 10px uppercase muted
│ VALUE UNIT           │  ← 24-36px bold bright
│ [sparkline]          │  ← optional, 60px wide
│ [progress bar]       │  ← optional, e.g. battery
└──────────────────────┘
```

Padding: 12-16px. No shadows (dark theme doesn't need them). Background: #0A0E14.

#### Status Indicators

Do not use words where colors + shapes suffice:

- **Dots:** ● = active/connected, ○ = disconnected, ◌ = pending
- **Bars:** Vertical segments for signal strength (like phone reception bars)
- **Pills:** Rounded rectangles for mode/state display
- **Rings:** Circular progress for battery, GPS accuracy

#### Notifications

- **Toast notifications:** Slide in from the top-right, auto-dismiss after 4 seconds
- **Use cases:** Command acknowledged, connection restored, preflight check passed
- **Critical alerts:** A centered modal overlay that requires acknowledgment (e.g., "LOW BATTERY — RETURN IMMEDIATELY")
- **All notifications** are also logged in the message log strip

---

## 8. User Experience Principles

### 8.1 The Three-Second Rule

An operator must be able to assess the **state of the vehicle** within 3 seconds of glancing at the screen.

How this is achieved:
1. **Altitude, Heading, Speed, Armed/Mode, Battery** in the top 20% of the screen — always
2. Color encoding tells you health at a glance (all green = good, any red = problem)
3. The preflight checklist is a single glance — 5 items, all must be green

### 8.2 Contextual Enablement

Controls are **disabled by default** and become enabled only when their preconditions are met:

| Control | Enables when |
|---------|-------------|
| ARM | GUIDED mode + GPS 3D fix + battery >20% + WiFi connected + MAVLink connected + Disarmed |
| TAKEOFF | Armed + GUIDED mode |
| MOVE | Armed + GUIDED + Altitude >2m + GPS 3D fix |
| GOTO (click map) | Same as MOVE + within geofence |
| LAND | Always (but shows warning if not armed) |
| RTL | Always |

Disabled buttons show a tooltip explaining WHY they're disabled:
- `ARM` disabled → tooltip: "Set GUIDED mode and complete preflight checks"
- `MOVE` disabled → tooltip: "Requires GUIDED, armed, >2m altitude, 3D GPS fix"

**Never hide buttons. Disable them with explanation.** This teaches the operator the system.

### 8.3 Progressive Disclosure

The dashboard has three "detail levels" that the operator can switch between:

| Level | What's shown | When to use |
|-------|-------------|-------------|
| **CRUISE** (default) | Map + PFD + Status + Command bar | Normal flight monitoring |
| **LAUNCH** | Enlarged telemetry, hide sparklines, show preflight in focus | Before/during takeoff |
| **ANALYZE** | Full-screen telemetry grid, larger charts, historical data | Post-flight analysis, troubleshooting |

Switch via a small dropdown in the status bar or a keyboard shortcut (e.g., Ctrl+1/2/3).

### 8.4 Error Recovery

The most stressful moment — connection loss — must be handled beautifully:

1. **Immediate feedback:** Status bar link indicator turns red within 500ms
2. **Graceful degradation:** Map stops updating but stays frozen. Telemetry shows last values with `---` animation (not disappearing). Controls disable.
3. **Reconnection UX:**
   - First 5 seconds: "Connection lost — reconnecting..." in the status bar
   - After 5 seconds: A dark overlay with the Skylink logo and a pulsing "RECONNECTING" text appears over the map only (left and right panels stay visible)
   - Progress dots animate: `●○○` → `●●○` → `●●●` → `●●●` → `●○○`
4. **On reconnect:** All values snap to current with a brief green flash. Overlay vanishes with a fade.
5. **If offline >30s:** Show "Dashboard operating in offline mode. Last update: [time]. Telemetry recording paused." with a prominent manual reconnect button.

### 8.5 State Persistence

The dashboard must survive browser refreshes:

- **Current mode, arm state, and telemetry** — restored from last WebSocket message (kept in sessionStorage)
- **Map position and zoom** — saved to localStorage
- **Panel layout customizations** — saved to localStorage
- **Active tab/level** — restored on reload
- **No data ever sent to a server** — all state is local

---

## 9. Usability & Learnability

### 9.1 Onboarding for New Operators

A new operator should be able to fly a simulated drone within 2 minutes of opening the dashboard.

**Strategies:**

1. **Empty-state guidance:** When no telemetry is received, the dashboard shows an overlay:
   ```
   ┌───────────────────────────────────────────┐
   │                                           │
   │   📡 Waiting for vehicle connection...    │
   │                                           │
   │   Start ArduPilot SITL, then open this    │
   │   page. The dashboard connects via WS.    │
   │                                           │
   │   [How to start SITL →]                   │
   │                                           │
   └───────────────────────────────────────────┘
   ```

2. **Interactive tooltips on first visit:** The first time an operator opens the dashboard, a short guided tour highlights key zones (5-7 stops, dismissable). "This is the map — click to set a destination."

3. **Keyboard shortcut reference card:** A small `?` button in the status bar that opens a clean modal with all keyboard shortcuts grouped by function.

4. **Visual affordance:** All clickable elements have hover states. Buttons that do something destructive have a secondary confirmation step.

### 9.2 Keyboard-First Design

The dashboard must be fully operable without a mouse. Every function accessible via keyboard.

| Shortcut | Action |
|----------|--------|
| **W** | Move Forward |
| **S** | Move Backward |
| **A** | Move Left |
| **D** | Move Right |
| **Shift+W** | Move Up |
| **Shift+S** | Move Down |
| **Q** | Yaw Left 90° |
| **E** | Yaw Right 90° |
| **Space** | ARM (when in GUIDED) / DISARM (when armed) |
| **T** | TAKEOFF (with last used altitude) |
| **L** | LAND |
| **R** | RTL |
| **F** | Toggle Follow mode on map |
| **C** | Center map on drone |
| **G** | GOTO — enter click-to-fly mode (map click sets target) |
| **1-5** | Select distance preset |
| **Ctrl+1** | Switch to CRUISE view level |
| **Ctrl+2** | Switch to LAUNCH view level |
| **Ctrl+3** | Switch to ANALYZE view level |
| **M** | Toggle minimap (if fullscreen map is active) |
| **?** | Show keyboard shortcuts |
| **Esc** | Close overlay / Cancel GOTO / Deselect |

All shortcuts are **remappable** in a settings panel (future feature — for now, hardcode but document clearly).

### 9.3 Accessibility

- All interactive elements must have visible focus rings (cyan, 2px)
- Tab order: Command Bar → Map → Left Wing → Right Wing → Status Bar
- Color is never the sole indicator of state — add icon changes or text for colorblind users
- Minimum contrast ratio 4.5:1 for all text
- Support for browser zoom up to 200% without layout breakage
- ARIA labels on all interactive elements

---

## 10. Interactions & Animation

### 10.1 Animation Philosophy

**Purposeful. Brief. Informative.**

Every animation answers a question:
- "Did my command work?" → Button flashes green on success
- "Did something change?" → Value flashes once on update
- "Where should I look?" → Subtle directional pulse for warnings

**Animation principles:**
- Duration: 150-300ms for state transitions, never more
- Easing: `cubic-bezier(0.4, 0, 0.2, 1)` — standard material ease
- No decorative animations. No parallax. No confetti.
- Every animation has `prefers-reduced-motion: reduce` support

### 10.2 Specific Animations

| Element | Animation | Trigger | Duration |
|---------|-----------|---------|----------|
| Telemetry value | Green background flash → fade to normal | Value increases | 300ms |
| Telemetry value | Red background flash → fade to normal | Value decreases | 300ms |
| Button press | Scale 1→0.97→1 | On click | 100ms |
| Toast notification | Slide in from right, stay, slide out | Event occurs | 4s total |
| Panel show/hide | Opacity 0→1 + translateY(4px→0) | View level change | 200ms |
| Link status chip | Subtle pulse (opacity 0.8→1→0.8) | While connected | 2s cycle |
| Connection overlay | Opacity 0→1, blur bg | Connection lost | 300ms |
| Drone marker | Soft glow pulse | While receiving telemetry | 1.5s cycle |
| Map pin drop | Pin drops from top, bounces slightly | Map click sets target | 400ms |
| Preflight item | Icon transitions ○→● with color change | Status changes | 200ms |
| Battery bar | Smooth width transition | Value changes | 300ms |
| Sparkline | New point slides in from right | Each heartbeat | 200ms |
| Command bar button | Brief border glow | Command sent, waiting for ACK | Until ACK received |

### 10.3 Feedback Loops

Every user action produces **immediate, predictable feedback**:

1. Operator clicks ARM → button briefly depresses (100ms)
2. Button shows "ARMING..." pulse state (border pulses amber)
3. WebSocket message sent → log shows "TX: ARM_DRONE"
4. ACK received → button turns green solid, log shows "ACK: ACCEPTED"
5. Arm state changes → PFD shows ARMED in red, preflight updates

**If no ACK within 2 seconds:** Button shows "NO RESPONSE" in red, log shows warning. The operator knows the command was sent but not confirmed — they can retry or investigate.

---

## 11. Data Visualization Principles

### 11.1 The Altitude Display

Altitude is **the most critical data point**. It deserves special treatment:

- **Primary display:** Large monospace number (36px), always at the top of the left wing
- **Vertical tape:** A side strip with a moving indicator, like an aircraft altimeter tape
- **Color code:** Green (2-30m) → Amber (30-45m or <2m) → Red (>=50m or <1m)
- **Sparkline:** 60-second history behind the number
- **Units:** Always meters. Never feet. (Can be configurable later.)

### 11.2 The Battery Display

- **Primary:** Percentage in large text
- **Secondary:** Voltage in smaller text below
- **Visual bar:** Horizontal bar that changes color:
  - Green (>50%)
  - Amber (20-50%)
  - Red (<20%)
  - Flashing red (<10% — CRITICAL)
- **Estimated time remaining:** Calculated from voltage drop rate (future: from current draw via MAVLink)
- **Sparkline:** Voltage trend over last 60s

### 11.3 The Attitude Indicator

Replace the current 64px bubble with a proper instrument:

- **Size:** 160×160px minimum (expandable to fill available space)
- **Sky/ground:** Blue gradient top, brown/dark bottom
- **Pitch ladder:** Horizontal lines every 10° pitch (plus 5° intermediate ticks)
- **Roll arc:** A curved scale at the top showing bank angle
- **Drone symbol:** Small triangle in the center, fixed
- **Performance:** Must update at 10fps minimum, interpolating between 5Hz telemetry updates
- **On no data:** Show a "NO ATTITUDE" overlay with an X through the instrument

### 11.4 The Map

- **Tile layer:** Custom dark theme (CartoDB Dark Matter or a custom OpenStreetMap render)
- **Night mode:** Map auto-tilts to a darker palette based on local time at the drone's position
- **Zoom levels:** 15-19 for operational use, with auto-zoom that adjusts to keep the drone and its geofence visible
- **Multiple map sources:** Toggle between satellite, street, and terrain views (via a small icon in the map controls)
- **3D option (future):** CesiumJS integration for 3D terrain visualization — the drone floating above real terrain

### 11.5 Color Semantics

| Color | Meaning | Used For |
|-------|---------|----------|
| Cyan | Active, connected, information | Skylink brand, telemetry values, map trail |
| Green | Safe, good, success | Armed (oddly enough — green = safe to operate), preflight pass, battery >50%, link OK |
| Amber | Warning, caution | Battery 20-50%, weak GPS, degraded link, simulation mode |
| Red | Error, critical, danger | Battery <20%, disconnected, preflight fail, RTL, KILL |
| Purple | Change of state, acknowledgment | Mode change, command accepted |

---

## 12. Mobile & Responsive Strategy

### 12.1 Primary Target

**Desktop 1920×1080 and larger.** This is an operational tool for a laptop/desktop with a keyboard. Mobile is a secondary use case for quick monitoring.

### 12.2 Responsive Breakpoints

| Breakpoint | Layout | Usage |
|------------|--------|-------|
| >1600px | Full 3-column layout | Desktop monitor |
| 1024-1600px | 3-column, compact spacing | Laptop |
| 768-1024px | 2-column: Map + stacked PFD/Status | Tablet landscape |
| <768px | Single column, stacked, simplified | Tablet portrait / Phone |

### 12.3 Mobile Adaptations

On mobile (<768px):

- **Map fills the top 50% of the viewport**
- **PFD and Status collapse into a single scrollable strip below the map**
- **Command bar becomes a floating bottom sheet** that slides up on tap
- **Buttons are larger** (minimum 44×44px touch targets)
- **Keyboard shortcuts are replaced by long-press and swipe gestures**
  - Swipe up on drone = increase altitude
  - Swipe down = decrease altitude
  - Tap map = set GOTO
  - Long-press ARM = arm (with haptic feedback on supported devices)
- **Landscape orientation preferred** and locked by default
- **Status bar becomes a collapsed strip** that expands on tap
- No preflight checklist on mobile — it's a monitoring-only view

---

## 13. Technical Considerations

### 13.1 Performance Targets

| Metric | Target |
|--------|--------|
| Initial page load | <500ms (from ESP32 flash memory, ~300KB total page size) |
| Map interaction | 60fps pan/zoom |
| Telemetry update rendering | <20ms from WebSocket message → DOM update |
| Memory usage | <80MB browser memory |
| CPU usage | <10% on modern hardware during normal operation |
| Animation frame budget | <8ms per frame (120fps target) |

### 13.2 Constraints (ESP32 Hardware)

The ESP32 serves files from LittleFS, which has **~1.5MB available** for the entire web dashboard. This means:

- **Total asset size must be <1MB** (ideally <500KB)
- No frameworks heavier than vanilla JS + Leaflet
- No Webpack/Rollup bundler output >200KB
- All CSS must be self-contained (no CDN fonts — embed fonts or use system font stacks)
- All JS must be hand-optimized or minimally bundled
- GZip (or Brotli if ESP32 supports it) all served assets
- **Lighthouse score target:** 95+ on all metrics

### 13.3 WebSocket Protocol

Current protocol uses versioned JSON messages:

```json
// From ESP32 (heartbeat):
{"v":1,"type":"event","event":"HEARTBEAT","altitude":5.2,"armed":false,...}

// From browser (command):
{"v":1,"type":"command","command":"ARM_DRONE"}
```

Keep this protocol but add:
- **Heartbeat sequence number** for detecting missed messages
- **Server timestamp** on every message (already partially implemented)
- **Command ack ID** — each command gets a unique ID from the browser so the ACK can be matched

### 13.4 Tech Stack Recommendations

| Layer | Recommended | Rationale |
|-------|-------------|-----------|
| View library | Vanilla JS or Svelte (compiled, <3KB) | ESP32 constraints — no React/Vue bundle overhead |
| Map | Leaflet 1.9+ (already in use, ~40KB gzipped) | Smallest mature map library |
| Charts | ApexCharts (lightweight) or custom Canvas sparklines | Tiny sparklines don't need a chart library |
| CSS | PostCSS + custom properties or Tailwind (purged) | Either works — Tailwind must be purged to <20KB |
| Icons | Feather icons (SVG, ~5KB subset) | Consistent 24px outline icons |
| Fonts | JetBrains Mono (subset, ~30KB) + Inter (subset, ~40KB) | Self-host subsets; never load Google Fonts at runtime |
| Animations | CSS transitions + Web Animations API | No animation library needed |
| Build | esbuild (fast, small output) | Minimal configuration, fast rebuilds |

---

## 14. Design System Tokens

### 14.1 Spacing Scale

| Token | Pixels | Use |
|-------|--------|-----|
| --space-1 | 4px | Tiny gaps |
| --space-2 | 8px | Element padding, grid gaps |
| --space-3 | 12px | Card padding, button padding |
| --space-4 | 16px | Section spacing |
| --space-5 | 24px | Panel padding |
| --space-6 | 32px | Large gaps between sections |
| --space-7 | 48px | Page-level margins |

### 14.2 Border Radius Scale

| Token | Value | Use |
|-------|-------|-----|
| --radius-sm | 4px | Input fields, small indicators |
| --radius-md | 8px | Buttons, cards, tiles |
| --radius-lg | 12px | Panels, modals |
| --radius-full | 999px | Pills, badges, link chips |

### 14.3 Shadows

In a dark theme, shadows are barely perceptible:

```css
--shadow-sm: 0 1px 2px rgba(0,0,0,0.3);
--shadow-md: 0 4px 6px rgba(0,0,0,0.4);
--shadow-lg: 0 10px 15px rgba(0,0,0,0.5);
--shadow-glow-cyan: 0 0 8px rgba(0,212,255,0.3);
--shadow-glow-red: 0 0 8px rgba(239,68,68,0.3);
```

### 14.4 Z-Index Scale

| Layer | z-index | Elements |
|-------|---------|----------|
| Base | 0 | Map tiles, panel backgrounds |
| Content | 10 | Text, controls, buttons |
| Overlay | 100 | Map HUD overlays, context cards |
| Dropdown | 200 | Modals, expanded panels |
| Toast | 300 | Notifications |
| Loading | 400 | Connection overlay |
| Max | 500 | Emergency modal, confirmation dialogs |

---

## 15. Appendix: Reference Dashboards

### 15.1 Dashboard Design Inspirations (External Links to Research)

These are not code references — they are visual inspiration sources. An AI agent generating the dashboard should study these before writing any code.

**Category: Professional Trading Terminals**
- Bloomberg Terminal — Multi-pane financial data. The reigning king of professional dashboards.
- TD Ameritrade Thinkorswim — Flexible pane layout, drag-rearrange panels, every screen has keyboard shortcuts.
- Robinhood — Beautifully minimal, but too simple for a GCS. Learn from their use of color and typography.
- TradingView — Excellent chart interaction: hover to see values, crosshairs, multi-timeframe.

**Category: GCS / Drone Mission Control**
- QGroundControl — The leading open-source GCS. Study its instrument panel and mapping. Note what it does well (preflight, mission planning) and poorly (cluttered UI, dated aesthetics).
- Mission Planner — Mature but ugly. Learn from its depth of telemetry data and FAIL on its visual design.
- UgCS — Professional drone software with a clean, modern desktop interface.
- DJI Pilot — The iPad-based controller app. Study its flight mode visualization and camera feed integration.

**Category: Aviation / Aerospace**
- Garmin G1000 — The glass cockpit standard. Learn how altitude tapes, heading strips, and HSI work.
- NASA MCC (Mission Control Center) — The original multi-pane monitoring system. Study how they group telemetry by subsystem.
- SpaceX Dragon Crew UI — The most beautiful aerospace UI ever built. Touch-screen rocket control. Study their use of deep space backgrounds and neon accents.
- F-35 Panoramic Cockpit Display — Full touchscreen cockpit with declutter modes.

**Category: Sci-Fi / Futuristic UI**
- *Minority Report* UI — Gesture-based, transparent, futuristic. Tone down for practicality.
- *Iron Man* HUD — Circular targeting, arcs, rings. Overdesigned for real use, but inspiring for animations and data density.
- *Prometheus* / *Alien: Covenant* — Weyland-Yutani UI designs: cold, precise, monochrome with orange/amber accents.

### 15.2 What NOT to Do

| Anti-pattern | Example | Why it fails |
|--------------|---------|--------------|
| Glassmorphism | Frosted glass panels | Reduces readability of critical telemetry |
| Heavy gradients | Vibrant multi-color backgrounds | Distracts from data, increases cognitive load |
| Animated backgrounds | Particle effects, floating elements | Creates visual noise, operator cannot focus |
| Overuse of icons | Icons replacing all text labels | Ambiguity — a wrench icon could mean "settings" or "maintenance" |
| Circular gauges | Clock-style speedometers | Less readable than vertical tapes for precise values |
| Auto-hiding panels | Panels that hide when not in use | Operator never knows when data might appear unexpectedly |
| Too many colors | Rainbow dashboards | Reduces the impact of actual warnings — everything screams |

### 15.3 The Golden Rules

1. **Data is the interface.** Every pixel should communicate information. If it doesn't carry data, it's decoration. Decoration has no place in a GCS.

2. **The map is not the dashboard.** The map is one tool among many. The PFD, telemetry, and controls are equally important and always visible.

3. **Design for inattention.** An operator might be looking away when something changes. The UI must tell them what they missed when they look back — through persistent state indicators, flash animations, and message logs.

4. **Design for stress.** When the battery is critically low and the drone is drifting, the operator shouldn't have to search for the RTL button. It should be in the same place it always was, now glowing red, the most prominent thing on screen.

5. **Consistency over creativity.** Every button works the same way. Every value is formatted the same way. Every tile has the same structure. The operator builds muscle memory, not puzzle-solving skills.

6. **The ESP32 constrains everything.** Every byte counts. Every animation costs CPU. The dashboard must be ruthlessly efficient — beautiful dark theme, smooth 60fps animations, all under 500KB total.

---

*End of design vision document. This is the blueprint. Build from here.*

*"Command the sky."*