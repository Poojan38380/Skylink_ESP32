# Skylink GCS — Option B: Micrographic Dashboard Execution Plan

> **Document purpose:** This is the step-by-step build plan for recreating the Skylink web dashboard from scratch using the **micrographic design language** (Option B). It combines the vision from `/docs/amazing-ui-option-b/skylink-gcs-micrographic-vision-and-design.md` with the data-driven recommendations from the `ui-ux-pro-max` skill (67 styles, 96 palettes, 57 font pairings, 99 UX guidelines, 25 chart types across the available stack).
>
> **Target:** A complete, production-ready GCS dashboard served from the ESP32's LittleFS. Total asset size <500KB.

---

## Table of Contents

1. [Phase 0: Project Setup & Tooling](#phase-0-project-setup--tooling)
2. [Phase 1: Design System — Foundation](#phase-1-design-system--foundation)
3. [Phase 2: The Micrographic Component Library](#phase-2-the-micrographic-component-library)
4. [Phase 3: Layout Shell & Grid Structure](#phase-3-layout-shell--grid-structure)
5. [Phase 4: Zone A — Certification Bar](#phase-4-zone-a--certification-bar)
6. [Phase 5: Zone B — Left Wing (Flight Data PFD)](#phase-5-zone-b--left-wing-flight-data-pfd)
7. [Phase 6: Zone C — Main Stage (Map + Overlays)](#phase-6-zone-c--main-stage-map--overlays)
8. [Phase 7: Zone D — Right Wing (Systems Status)](#phase-7-zone-d--right-wing-systems-status)
9. [Phase 8: Zone E — Command Bar (Bottom Dock)](#phase-8-zone-e--command-bar-bottom-dock)
10. [Phase 9: Zone F+G — Message Strip & Status Bar](#phase-9-zone-fg--message-strip--status-bar)
11. [Phase 10: WebSocket Integration & Data Flow](#phase-10-websocket-integration--data-flow)
12. [Phase 11: Micro-Interactions & Animation](#phase-11-micro-interactions--animation)
13. [Phase 12: Accessibility & Performance](#phase-12-accessibility--performance)
14. [Phase 13: Mobile Responsive](#phase-13-mobile-responsive)
15. [Phase 14: Testing & Verification](#phase-14-testing--verification)
16. [Phase 15: ESP32 Deployment](#phase-15-esp32-deployment)
17. [Design System Summary](#design-system-summary)
18. [Reference Index](#reference-index)

---

## Phase 0: Project Setup & Tooling

### 0.1 Directory Structure

Create the dashboard files in `data/` — same location as the current prototype. New files will replace the old ones after testing.

```
data/
├── index.html              ← New main HTML (replaces existing)
├── gcs.css                 ← New CSS (replaces existing) — imported by index.html
├── gcs.js                  ← New JS (replaces existing) — imported by index.html
├── gcs_config.js           ← Config (keep existing, update if needed)
├── gcs_map.js              ← Map module (rewrite with micrographic overlays)
├── favicon.ico             ← Keep existing
├── apple-icon.png          ← Keep existing
├── skylink_build.json      ← Keep existing
├── lib/
│   └── leaflet/            ← Keep existing, Leaflet 1.9+
```

### 0.2 Technology Stack

Derived from ui-ux-pro-max analysis — we need maximum performance on a constrained ESP32:

| Layer | Choice | Rationale |
|-------|--------|-----------|
| **Markup** | Semantic HTML5 | 0KB overhead, required for a11y |
| **Styling** | Vanilla CSS + CSS Custom Properties | No framework overhead, Tailwind would need purging but adds complexity. Use the design system tokens directly. |
| **JavaScript** | Vanilla JS (ES Modules pattern) | No bundler. Module pattern via IIFE/closures. The `gcs_config.js` + `gcs_map.js` + `gcs.js` pattern already works — keep it. |
| **Map** | Leaflet 1.9+ | Already in use. ~40KB gzipped. Must swap tile URL for custom dark monochrome tiles. |
| **Fonts** | JetBrains Mono + Inter (subsetted) | Self-host woff2 subsets. Google Fonts is blocked by ESP32 ecosystem — download and embed. |
| **Icons** | Inline SVG (hairline style) | SVGs embedded in HTML or as an SVG sprite. No icon library dependency. |
| **Build** | esbuild (offline on dev machine, deploy output) | For minification only. Run locally, deploy minified files to ESP32. |

**Anti-patterns to avoid** (from ui-ux-pro-max row 93 "Space Tech/Aerospace"):
- Generic design with no immersion
- Light mode by default
- Poor data visualization
- Standard web aesthetics (this needs to feel like a spacecraft console)

### 0.3 Tool Setup

```bash
# On dev machine (not ESP32):
# 1. Install Node.js if not present
# 2. Install esbuild for minification
npm install -g esbuild

# 3. Download and subset fonts:
#    - JetBrains Mono: subset to ASCII + basic Latin, woff2
#    - Inter: subset to ASCII + basic Latin, woff2
# 4. Create dev workflow:
#    - Write files in a dev/ directory
#    - Minify with esbuild: esbuild dev/gcs.js --minify --outfile=data/gcs.js
#    - Copy CSS: esbuild dev/gcs.css --minify --outfile=data/gcs.css
```

**Total budget tracking:**

| Asset | Budget | Target |
|-------|--------|--------|
| `index.html` | <12KB | Minimal, semantic, no inline JS |
| `gcs.css` | <30KB | All styles in one file |
| `gcs.js` | <60KB | All logic in one file |
| `gcs_map.js` | <15KB | Leaflet map module |
| `gcs_config.js` | <2KB | Config constants |
| Subtotal (HTML+CSS+JS) | <119KB | |
| JetBrains Mono subset (woff2) | <25KB | ASCII only |
| Inter subset (woff2) | <30KB | ASCII + common Latin |
| Leaflet JS | 40KB | Already in lib/ |
| Leaflet CSS | 5KB | Already in lib/ |
| Total | <219KB | Well under 500KB cap |

---

## Phase 1: Design System — Foundation

### 1.1 Design System Output from ui-ux-pro-max

The skill database recommends the following for a **"Space Tech/Aerospace" + "Autonomous Drone Fleet" + "Real-Time Monitoring Dashboard"** product (see ui-reasoning.csv rows 93, 97, 54, 19, 44, 31):

### 1.2 Recommended Style: HUD/Sci-Fi FUI + Dark Mode (OLED)

From the skill's style catalog (row 51: HUD/Sci-Fi FUI):

| Attribute | Value |
|-----------|-------|
| **Primary Colors** | Neon Cyan #00FFFF, Holographic Blue #0080FF, Alert Red #FF0000 |
| **Secondary Colors** | Transparent Black, Grid Lines #333333 |
| **Effects** | Glow effects, scanning animations, ticker text, blinking markers, fine line drawing |
| **Best For** | Sci-fi games, space tech, cybersecurity, immersive dashboards |
| **AI Prompt** | "Design a futuristic HUD. Use: thin lines (1px), neon cyan/blue on black, technical markers, decorative brackets, data visualization, monospaced tech fonts, glowing elements, transparency." |
| **CSS Tokens** | `border: 1px solid rgba(0,255,255,0.5)`, `color: #00FFFF`, `background: rgba(0,0,0,0.8)`, `text-shadow: 0 0 5px cyan` |

### 1.3 Blending with Micrographics

The ui-ux-pro-max HUD style meshes perfectly with our micrographic direction. Both share:
- Dark backgrounds (OLED black)
- Fine line work (1px hairline strokes)
- Technical/precision aesthetic
- Monospaced fonts
- Bracket/corner decorative elements
- High contrast

The micrographic layer **adds**: industrial safety palette (orange/amber), geometric framing (diamonds, squares), NFPA-style hazard diamonds, certification stamps, measurement scales with tick marks.

**The unified palette:**

```
// Backgrounds
--bg-void:         #050505    — Absolute black, OLED-optimized
--bg-panel:        #080A0E    — Panel surface
--bg-elevated:     #0C1016    — Hovered/active
--bg-hud:          rgba(0,10,20,0.92) — HUD overlay background

// Text
--text-primary:    #E8EDF5    — Main values, near-white
--text-secondary:  #8B98AA    — Labels, units
--text-muted:      #4A5A70    — Micro-detail, stamps
--text-cert:       #6B7C93    — Certification text

// Accents (blend of HUD cyan + micrographic amber)
--accent-cyan:     #00D4FF    — Telemetry, link health, active data (HUD glow)
--accent-amber:    #F59E0B    — Warnings, caution, simulation mode
--accent-green:    #34D399    — Safe, armed, nominal, success
--accent-red:      #EF4444    — Error, critical, danger, kill
--accent-yellow:   #EAB308    — Caution, non-critical warning

// Borders
--border-hairline: #1A1F2E    — 1px standard border
--border-active:   #2D3A5C    — Focused/selected
--border-readout:  #1E2D4A    — Measurement scale border

// HUD-specific
--hud-cyan:        #00D4FF    — Primary HUD color
--hud-glow:        0 0 6px rgba(0,212,255,0.3)
--hud-line:        1px solid rgba(0,212,255,0.15)

// Glows
--glow-cyan:       rgba(0,212,255,0.12)
--glow-red:        rgba(239,68,68,0.15)
--glow-green:      rgba(52,211,153,0.12)
--glow-amber:      rgba(245,158,11,0.12)
```

### 1.4 Font Pairing

From ui-ux-pro-max typography domain, the recommended pairing for "Space Tech/Aerospace" with "Technical/Functional" mood:

| Role | Font | Weight | Size (range) |
|------|------|--------|--------------|
| **Primary values** | JetBrains Mono | 700 (Bold) | 20-36px |
| **Secondary values** | JetBrains Mono | 600 (SemiBold) | 14-18px |
| **UI labels** | Inter | 500 (Medium) | 10-12px |
| **Section headers** | Inter or Press Start 2P | 600 (SemiBold) | 10-11px, uppercase |
| **Micro-detail** | JetBrains Mono | 400 (Regular) | 9-10px |
| **Button labels** | Inter | 600 (SemiBold) | 12-13px |

### 1.5 CSS Custom Properties File

Create `data/gcs.css` with this structure:

```css
/* ============================================================
   Skylink GCS — Micrographic Design System
   Option B: HUD + Industrial Spec Plate
   ============================================================ */

/* --- Fonts --- */
@font-face {
  font-family: 'JetBrains Mono';
  src: url('/fonts/jetbrains-mono-latin-subset.woff2') format('woff2');
  font-weight: 400 700;
  font-display: swap;
}

@font-face {
  font-family: 'Inter';
  src: url('/fonts/inter-latin-subset.woff2') format('woff2');
  font-weight: 400 600;
  font-display: swap;
}

/* --- Design Tokens --- */
:root {
  /* Backgrounds */
  --bg-void: #050505;
  --bg-panel: #080A0E;
  --bg-elevated: #0C1016;
  --bg-hud: rgba(0, 10, 20, 0.92);

  /* Text */
  --text-primary: #E8EDF5;
  --text-secondary: #8B98AA;
  --text-muted: #4A5A70;
  --text-cert: #6B7C93;

  /* Accents */
  --accent-cyan: #00D4FF;
  --accent-amber: #F59E0B;
  --accent-green: #34D399;
  --accent-red: #EF4444;
  --accent-yellow: #EAB308;

  /* Borders */
  --border-hairline: #1A1F2E;
  --border-active: #2D3A5C;
  --border-readout: #1E2D4A;

  /* HUD */
  --hud-line: 1px solid rgba(0, 212, 255, 0.15);
  --hud-glow: 0 0 6px rgba(0, 212, 255, 0.3);

  /* Typography */
  --font-mono: 'JetBrains Mono', ui-monospace, monospace;
  --font-sans: 'Inter', -apple-system, sans-serif;

  /* Spacing */
  --space-1: 4px;
  --space-2: 8px;
  --space-3: 12px;
  --space-4: 16px;
  --space-5: 24px;
  --space-6: 32px;

  /* Layout dimensions */
  --bar-height: 48px;        /* Certification bar */
  --wing-left-width: 280px;   /* Left PFD */
  --wing-right-width: 340px;  /* Right status */
  --command-bar-height: 140px; /* Bottom dock */
  --status-strip-height: 32px; /* Bottom strip */
  --map-controls-size: 40px;

  /* Transitions */
  --transition-snap: 100ms;
  --transition-fast: 200ms;
  --transition-normal: 300ms;

  /* Z-index */
  --z-base: 0;
  --z-content: 10;
  --z-overlay: 100;
  --z-dropdown: 200;
  --z-toast: 300;
  --z-modal: 400;
  --z-loading: 500;

  /* Scrollbar */
  scrollbar-width: thin;
  scrollbar-color: var(--accent-cyan) var(--bg-panel);
}
```

---

## Phase 2: The Micrographic Component Library

Before building the layout, create all reusable micrographic components as pure CSS + SVG. Each component is a reusable class.

### 2.1 Geometric Frames

```css
/* Diamond frame — hazard/critical */
.frame-diamond {
  width: 40px; height: 40px;
  transform: rotate(45deg);
  border: 1.5px solid var(--border-hairline);
  display: flex; align-items: center; justify-content: center;
}
.frame-diamond > * {
  transform: rotate(-45deg);
}
.frame-diamond.danger { border-color: var(--accent-red); }
.frame-diamond.warn { border-color: var(--accent-amber); }
.frame-diamond.ok { border-color: var(--accent-green); }

/* Circle frame — certification/approval */
.frame-circle {
  width: 32px; height: 32px;
  border-radius: 50%;
  border: 1.5px solid var(--border-hairline);
  display: flex; align-items: center; justify-content: center;
}

/* Square frame — data/specification */
.frame-square {
  border: 1px solid var(--border-hairline);
  padding: var(--space-2);
}

/* Stadium pill — feature highlight */
.frame-pill {
  border-radius: 999px;
  border: 1px solid var(--border-hairline);
  padding: 4px 12px;
  font-size: 10px;
  font-family: var(--font-sans);
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

/* Hexagon frame — system/network */
.frame-hexagon {
  width: 36px; height: 36px;
  clip-path: polygon(25% 0%, 75% 0%, 100% 50%, 75% 100%, 25% 100%, 0% 50%);
  border: 1.5px solid var(--border-hairline);
  display: flex; align-items: center; justify-content: center;
}
```

### 2.2 Corner Brackets (Panel Frames)

```css
/* Panel with corner brackets — standard data panel */
.panel {
  position: relative;
  border: 1px solid var(--border-hairline);
  background: var(--bg-panel);
}

/* Corner bracket decorators */
.panel::before,
.panel::after {
  content: '';
  position: absolute;
  width: 8px; height: 8px;
  border-color: var(--accent-cyan);
  border-style: solid;
  opacity: 0.4;
}
.panel::before {
  top: -1px; left: -1px;
  border-width: 1px 0 0 1px;
}
.panel::after {
  top: -1px; right: -1px;
  border-width: 1px 1px 0 0;
}

/* Critical panel (inverted brackets) */
.panel.critical::before {
  border-color: var(--accent-red);
  bottom: -1px; left: -1px;
  top: auto;
  border-width: 0 0 1px 1px;
}
.panel.critical::after {
  border-color: var(--accent-red);
  bottom: -1px; right: -1px;
  top: auto;
  border-width: 0 1px 1px 0;
}

/* Active panel — all four corners lit */
.panel.active::before,
.panel.active::after {
  opacity: 1;
}
```

### 2.3 Certification Stamp

```css
.cert-stamp {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  padding: 2px 8px;
  border: 0.5px solid var(--border-active);
  font-family: var(--font-mono);
  font-size: 9px;
  color: var(--text-cert);
  background: var(--bg-panel);
}
.cert-stamp .dot {
  width: 5px; height: 5px;
  border-radius: 50%;
}
.cert-stamp .dot.live { background: var(--accent-cyan); }
.cert-stamp .dot.stale { background: var(--accent-amber); }
.cert-stamp .dot.off { background: var(--accent-red); }
```

### 2.4 Measurement Scale

```css
.scale-horizontal {
  display: flex;
  align-items: flex-end;
  gap: 1px;
  height: 20px;
}
.scale-horizontal .tick {
  width: 1px;
  background: var(--scale-tick);
}
.scale-horizontal .tick.major {
  width: 1.5px;
  height: 12px;
  background: var(--scale-tick-major);
}
.scale-horizontal .indicator {
  position: absolute;
  width: 3px;
  height: 16px;
  background: var(--accent-cyan);
  transition: left var(--transition-fast) linear;
}

/* Vertical scale */
.scale-vertical {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 1px;
  width: 20px;
}
```

### 2.5 Micrographic Buttons

```css
/* Primary action */
.btn-primary {
  font-family: var(--font-sans);
  font-size: 12px;
  font-weight: 600;
  padding: 10px 20px;
  background: transparent;
  color: var(--accent-cyan);
  border: 1px solid var(--accent-cyan);
  cursor: pointer;
  transition: all var(--transition-snap);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.btn-primary:hover {
  background: var(--glow-cyan);
}
.btn-primary:active {
  background: var(--accent-cyan);
  color: var(--bg-void);
}
.btn-primary:disabled {
  opacity: 0.35;
  cursor: not-allowed;
}

/* Danger action */
.btn-danger {
  border-color: var(--accent-red);
  color: var(--accent-red);
}
.btn-danger:hover { background: var(--glow-red); }
.btn-danger:active { background: var(--accent-red); color: var(--bg-void); }

/* Arm action */
.btn-arm {
  border-color: var(--accent-green);
  color: var(--accent-green);
}
.btn-arm:hover { background: var(--glow-green); }
.btn-arm:active { background: var(--accent-green); color: var(--bg-void); }

/* Move direction button (D-pad) */
.btn-move {
  width: 56px; height: 56px;
  display: flex; align-items: center; justify-content: center;
  background: transparent;
  border: 1px solid var(--border-hairline);
  color: var(--text-secondary);
  cursor: pointer;
  font-size: 16px;
  font-family: var(--font-mono);
  transition: all var(--transition-snap);
}
.btn-move:hover {
  border-color: var(--accent-cyan);
  color: var(--accent-cyan);
}
.btn-move:active {
  background: var(--bg-elevated);
}
.btn-move:disabled {
  opacity: 0.3;
  cursor: not-allowed;
}

/* Inline stamp button (spec-plate style) */
.btn-stamp {
  font-family: var(--font-mono);
  font-size: 10px;
  padding: 6px 12px;
  border: 0.5px solid var(--border-hairline);
  background: transparent;
  color: var(--text-secondary);
  cursor: pointer;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.btn-stamp:hover {
  border-color: var(--border-active);
  color: var(--text-primary);
}
```

### 2.6 SVG Icon Library

Create a set of inline SVG icons at the bottom of index.html (hidden, used via `<use>` or direct embed):

```html
<svg style="display:none">
  <!-- Drone icon -->
  <symbol id="icon-drone" viewBox="0 0 24 24">
    <path d="M12 2L2 7v10l10 5 10-5V7l-10-5z" stroke="currentColor" stroke-width="1.5" fill="none"/>
    <circle cx="12" cy="12" r="3" stroke="currentColor" stroke-width="1.5" fill="none"/>
  </symbol>
  <!-- Home icon -->
  <symbol id="icon-home" viewBox="0 0 24 24">
    <path d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" stroke="currentColor" stroke-width="1.5" fill="none"/>
  </symbol>
  <!-- Crosshair -->
  <symbol id="icon-crosshair" viewBox="0 0 24 24">
    <circle cx="12" cy="12" r="8" stroke="currentColor" stroke-width="1.5" fill="none"/>
    <line x1="12" y1="2" x2="12" y2="6" stroke="currentColor" stroke-width="1.5"/>
    <line x1="12" y1="18" x2="12" y2="22" stroke="currentColor" stroke-width="1.5"/>
    <line x1="2" y1="12" x2="6" y2="12" stroke="currentColor" stroke-width="1.5"/>
    <line x1="18" y1="12" x2="22" y2="12" stroke="currentColor" stroke-width="1.5"/>
  </symbol>
  <!-- ... more icons: battery, signal, altitude, speed, compass, arm, mode, etc. -->
</svg>
```

**Icon design rules (from the micrographic vision doc):**
- Hairline stroke: 1.5px on 24×24 canvas
- No fills — outlined only
- Geometric construction — every curve is an arc, every corner is exact
- Consistent stroke weight across all icons

### 2.7 Measurement Bar (Battery, Signal)

```css
.measure-bar {
  height: 6px;
  background: var(--border-hairline);
  position: relative;
  overflow: hidden;
}
.measure-bar .fill {
  height: 100%;
  transition: width var(--transition-normal) linear;
}
.measure-bar .fill.green { background: var(--accent-green); }
.measure-bar .fill.amber { background: var(--accent-amber); }
.measure-bar .fill.red { background: var(--accent-red); }

/* With tick overlay */
.measure-bar.ticked::after {
  content: '';
  position: absolute;
  inset: 0;
  background: repeating-linear-gradient(
    90deg,
    transparent,
    transparent 20%,
    rgba(255,255,255,0.05) 20%,
    rgba(255,255,255,0.05) 21%
  );
}
```

---

## Phase 3: Layout Shell & Grid Structure

### 3.1 HTML Shell

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SKYLINK GCS | GROUND CONTROL STATION</title>
  <link rel="icon" href="/favicon.ico" type="image/x-icon">
  <link rel="stylesheet" href="/lib/leaflet/leaflet.css">
  <link rel="stylesheet" href="/gcs.css">
</head>
<body>
  <!-- Reconnect overlay (loading state) -->
  <div id="reconnect-overlay" class="loading-overlay" role="alert" aria-live="polite">
    <div class="loading-sandwich">
      <div class="sandwich-top">SKYLINK GCS</div>
      <div class="sandwich-center">
        <div class="loading-logo">◆</div>
        <div class="loading-text">CONNECTION LOST</div>
        <div class="loading-dots"><span>●</span><span>○</span><span>○</span></div>
      </div>
      <div class="sandwich-bottom">PROTOCOL v1 · CERTIFIED</div>
    </div>
  </div>

  <!-- Main layout grid -->
  <div class="gcs-grid">

    <!-- ZONE A: Certification Bar (top) -->
    <header id="cert-bar" class="cert-bar" role="banner">
      <!-- content in Phase 4 -->
    </header>

    <!-- ZONE B: Left Wing (Flight Data) -->
    <aside id="left-wing" class="wing wing-left" role="complementary" aria-label="Flight data">
      <!-- content in Phase 5 -->
    </aside>

    <!-- ZONE C: Main Stage (Map) -->
    <main id="main-stage" class="main-stage" role="main" aria-label="Map view">
      <!-- content in Phase 6 -->
    </main>

    <!-- ZONE D: Right Wing (Systems Status) -->
    <aside id="right-wing" class="wing wing-right" role="complementary" aria-label="Vehicle status">
      <!-- content in Phase 7 -->
    </aside>

    <!-- ZONE E: Command Bar (Bottom Dock) -->
    <footer id="command-bar" class="command-bar" role="toolbar" aria-label="Flight controls">
      <!-- content in Phase 8 -->
    </footer>

    <!-- ZONE F: Message Strip -->
    <div id="msg-strip" class="msg-strip" role="log" aria-live="polite" aria-label="System messages">
      <!-- content in Phase 9 -->
    </div>

    <!-- ZONE G: Status Bar (bottom) -->
    <div id="status-strip" class="status-strip" role="contentinfo">
      <!-- content in Phase 9 -->
    </div>

  </div>

  <script src="/lib/leaflet/leaflet.js"></script>
  <script src="/gcs_config.js"></script>
  <script src="/gcs_map.js"></script>
  <script src="/gcs.js"></script>
</body>
</html>
```

### 3.2 CSS Grid Layout

```css
/* ============================================================
   Main Grid — Micrographic 3-Column Layout
   ============================================================ */

* { box-sizing: border-box; margin: 0; padding: 0; }

html, body {
  height: 100%;
  overflow: hidden;
  background: var(--bg-void);
  color: var(--text-primary);
  font-family: var(--font-mono);
}

.gcs-grid {
  display: grid;
  grid-template-columns:
    var(--wing-left-width)
    1fr
    var(--wing-right-width);
  grid-template-rows:
    var(--bar-height)
    1fr
    var(--command-bar-height)
    auto
    var(--status-strip-height);
  grid-template-areas:
    "cert   cert   cert"
    "left   main   right"
    "left   cmd    right"
    "msg    msg    msg"
    "status status status";
  height: 100vh;
  min-height: 100vh;
  max-height: 100vh;
  overflow: hidden;
  gap: 0;
}

/* Zone assignments */
.cert-bar    { grid-area: cert; }
.wing-left   { grid-area: left; }
.main-stage  { grid-area: main; }
.wing-right  { grid-area: right; }
.command-bar { grid-area: cmd; }
.msg-strip   { grid-area: msg; }
.status-strip{ grid-area: status; }
```

---

## Phase 4: Zone A — Certification Bar

### 4.1 HTML

```html
<header id="cert-bar" class="cert-bar">
  <div class="cert-left">
    <span class="cert-logo">◆</span>
    <span class="cert-name">SKYLINK GCS</span>
    <span class="cert-divider">|</span>
    <span class="cert-version" id="build-tag">FW 12 · FS 15</span>
  </div>
  <div class="cert-center">
    <span class="link-chip ok" id="chip-ws">WS ●</span>
    <span class="link-chip" id="chip-sitl">SITL ○</span>
    <span class="link-chip" id="chip-mav">MAV ○</span>
    <span class="link-chip" id="chip-wifi">WiFi ○</span>
  </div>
  <div class="cert-right">
    <span class="cert-time" id="sys-time">--:--:--Z</span>
    <span class="cert-badge active" id="link-badge">
      <span class="badge-dot pulse"></span>
      <span id="badge-text">CONNECTING</span>
    </span>
  </div>
</header>
```

### 4.2 CSS

```css
.cert-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 var(--space-4);
  background: var(--bg-panel);
  border-bottom: 1px solid var(--border-hairline);
  font-family: var(--font-mono);
  font-size: 11px;
}

.cert-left {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}
.cert-logo {
  font-size: 18px;
  color: var(--accent-cyan);
}
.cert-name {
  font-weight: 700;
  letter-spacing: 1.5px;
  color: var(--accent-cyan);
}
.cert-divider { color: var(--text-muted); }
.cert-version { color: var(--text-cert); }

.cert-center {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.link-chip {
  padding: 3px 10px;
  border: 0.5px solid var(--border-hairline);
  font-size: 9px;
  font-weight: 600;
  letter-spacing: 0.5px;
  color: var(--text-muted);
}
.link-chip.ok { border-color: var(--accent-green); color: var(--accent-green); }
.link-chip.warn { border-color: var(--accent-amber); color: var(--accent-amber); }
.link-chip.bad { border-color: var(--accent-red); color: var(--accent-red); }

.cert-right {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}
.cert-time { color: var(--text-secondary); }
.cert-badge {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 12px;
  border: 0.5px solid var(--border-hairline);
  font-size: 9px;
  font-weight: 600;
}
.cert-badge.connected { border-color: var(--accent-green); color: var(--accent-green); }
.cert-badge.disconnected { border-color: var(--accent-red); color: var(--accent-red); }
.cert-badge.connecting { border-color: var(--accent-amber); color: var(--accent-amber); }

.badge-dot { width: 6px; height: 6px; border-radius: 50%; background: currentColor; }
.badge-dot.pulse { animation: pulse 1.5s ease-in-out infinite; }
@keyframes pulse { 50% { opacity: 0.4; } }
```

---

## Phase 5: Zone B — Left Wing (Flight Data PFD)

### 5.1 HTML

```html
<aside class="wing wing-left">
  <!-- Attitude Indicator -->
  <div class="panel" id="attitude-panel">
    <div class="panel-header">
      <span class="panel-title">○ ATTITUDE</span>
      <span class="cert-stamp"><span class="dot live"></span>5 Hz</span>
    </div>
    <div class="panel-body">
      <div id="attitude-indicator" class="attitude-indicator">
        <div class="att-roll-scale" id="att-roll-scale">
          <span>30</span><span>20</span><span>10</span><span>0</span>
          <span>10</span><span>20</span><span>30</span>
        </div>
        <div class="att-window">
          <div class="att-horizon" id="att-horizon">
            <div class="att-sky"></div>
            <div class="att-ground"></div>
          </div>
          <div class="att-pitch-ladder" id="pitch-ladder">
            <div class="pitch-line" style="top:30%"><span>10</span></div>
            <div class="pitch-line" style="top:50%"><span>0</span></div>
            <div class="pitch-line" style="top:70%"><span>-10</span></div>
          </div>
          <div class="att-drone">▲</div>
        </div>
        <div class="att-readout">
          <span id="att-roll">ROLL 0.0°</span>
          <span id="att-pitch">PITCH 0.0°</span>
          <span id="att-yaw">YAW 0°</span>
        </div>
      </div>
    </div>
  </div>

  <!-- Altitude Tape -->
  <div class="panel" id="altitude-panel">
    <div class="panel-header">
      <span class="panel-title">■ ALTITUDE</span>
      <span class="cert-stamp"><span class="dot live"></span>5 Hz</span>
    </div>
    <div class="panel-body alt-body">
      <div class="alt-primary" id="alt-value">--.- m</div>
      <div class="alt-scale" id="alt-scale">
        <!-- Scale generated by JS -->
      </div>
      <div class="alt-micro">
        <span id="alt-relative">REL: --.- m</span>
        <span id="alt-rate">RATE: -.– m/s</span>
      </div>
    </div>
  </div>

  <!-- Speed Tape -->
  <div class="panel" id="speed-panel">
    <div class="panel-header">
      <span class="panel-title">■ SPEED</span>
      <span class="cert-stamp"><span class="dot live"></span>5 Hz</span>
    </div>
    <div class="panel-body">
      <div class="spd-primary" id="spd-value">-.– m/s</div>
      <div class="spd-bar" id="spd-bar">
        <div class="measure-bar ticked">
          <div class="fill green" id="spd-fill" style="width:0%"></div>
        </div>
      </div>
    </div>
  </div>

  <!-- Heading Compass -->
  <div class="panel" id="heading-panel">
    <div class="panel-header">
      <span class="panel-title">◄ HEADING</span>
    </div>
    <div class="panel-body">
      <div class="compass-strip" id="compass-strip">
        <div class="compass-tape" id="compass-tape">
          <!-- Generated by JS -->
        </div>
        <div class="compass-indicator">▼</div>
      </div>
      <div class="heading-value" id="heading-value">---°</div>
    </div>
  </div>

  <!-- VSD (Vertical Situation Display) -->
  <div class="panel" id="vsd-panel">
    <div class="panel-header">
      <span class="panel-title">↕ VERTICAL</span>
    </div>
    <div class="panel-body">
      <canvas id="vsd-canvas" width="248" height="100"></canvas>
    </div>
  </div>
</aside>
```

### 5.2 CSS for Left Wing

```css
.wing-left {
  display: flex;
  flex-direction: column;
  gap: 0;
  overflow-y: auto;
  overflow-x: hidden;
  background: var(--bg-void);
  border-right: 1px solid var(--border-hairline);
}

.wing-left .panel {
  border: none;
  border-bottom: 1px solid var(--border-hairline);
  border-right: none;
}

/* Attitude Indicator */
.attitude-indicator {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: var(--space-2);
}
.attitude-indicator .att-window {
  width: 160px;
  height: 120px;
  position: relative;
  overflow: hidden;
  border: 1px solid var(--border-hairline);
}
.att-horizon {
  position: absolute;
  width: 100%;
  height: 200%;
  top: 0;
  transition: transform 0.1s linear;
}
.att-sky { height: 50%; background: linear-gradient(180deg, #0a1a2e, #1a3a5e); }
.att-ground { height: 50%; background: linear-gradient(0deg, #1a1008, #2d1a0a); }
.att-drone {
  position: absolute;
  top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  color: var(--accent-cyan);
  font-size: 14px;
  z-index: 2;
}
.att-pitch-ladder {
  position: absolute;
  inset: 0;
  pointer-events: none;
  transition: transform 0.1s linear;
}
.pitch-line {
  position: absolute;
  left: 10%;
  width: 80%;
  height: 1px;
  border-top: 1px dashed rgba(0,212,255,0.4);
}
.pitch-line span {
  position: absolute;
  right: 100%;
  margin-right: 4px;
  font-size: 8px;
  color: var(--text-muted);
}
.att-readout {
  display: flex;
  gap: 12px;
  font-size: 9px;
  color: var(--text-secondary);
}

/* Altitude */
.alt-primary {
  font-size: 28px;
  font-weight: 700;
  color: var(--text-primary);
  padding: 0 var(--space-3);
}
.alt-scale {
  height: 24px;
  position: relative;
  margin: var(--space-1) var(--space-3);
}
.alt-micro {
  display: flex;
  justify-content: space-between;
  padding: var(--space-1) var(--space-3);
  font-size: 8px;
  color: var(--text-muted);
}

/* Speed */
.spd-primary {
  font-size: 18px;
  font-weight: 600;
  padding: 0 var(--space-3);
}
.spd-bar {
  padding: var(--space-2) var(--space-3);
}

/* Compass */
.compass-strip {
  position: relative;
  height: 28px;
  overflow: hidden;
  margin: var(--space-1) var(--space-3);
}
.compass-tape {
  display: flex;
  gap: 0;
  transition: transform 0.15s linear;
  white-space: nowrap;
}
.compass-indicator {
  position: absolute;
  bottom: 0;
  left: 50%;
  transform: translateX(-50%);
  color: var(--accent-red);
  font-size: 10px;
}
.heading-value {
  text-align: center;
  font-size: 14px;
  font-weight: 600;
  padding-bottom: var(--space-2);
}

/* VSD canvas */
#vsd-canvas {
  width: 100%;
  height: 100px;
  display: block;
}
```

### 5.3 JS Logic for Left Wing

```javascript
// In gcs.js:

function updateAttitude(d) {
  const roll = Number(d.roll) || 0;
  const pitch = Number(d.pitch) || 0;
  const yaw = Number(d.yaw) || 0;

  const horizon = document.getElementById('att-horizon');
  const pitchLadder = document.getElementById('pitch-ladder');
  const rollEl = document.getElementById('att-roll');
  const pitchEl = document.getElementById('att-pitch');
  const yawEl = document.getElementById('att-yaw');

  if (horizon) {
    // Rotate entire horizon by roll, translate up/down by pitch
    const pitchOffset = pitch * 1.5; // px per degree
    horizon.style.transform =
      `translateY(${pitchOffset}px) rotate(${-roll}deg)`;
  }
  if (pitchLadder) {
    pitchLadder.style.transform = `translateY(${pitch * 1.5}px)`;
  }
  if (rollEl) rollEl.textContent = `ROLL ${roll.toFixed(1)}°`;
  if (pitchEl) pitchEl.textContent = `PITCH ${pitch.toFixed(1)}°`;
  if (yawEl) yawEl.textContent = `YAW ${Math.round(yaw)}°`;
}

function updateAltitude(d) {
  const alt = Number(d.altitude) || 0;
  const relAlt = Number(d.relative_alt) || 0;

  const altEl = document.getElementById('alt-value');
  const relEl = document.getElementById('alt-relative');
  const scaleEl = document.getElementById('alt-scale');

  if (altEl) altEl.textContent = `${alt.toFixed(1)} m`;
  if (relEl) relEl.textContent = `REL: ${relAlt.toFixed(1)} m`;

  // Update scale indicator position
  if (scaleEl) {
    const maxAlt = 50;
    const pct = Math.min(100, (alt / maxAlt) * 100);
    const indicator = scaleEl.querySelector('.indicator');
    if (!indicator) {
      const dot = document.createElement('div');
      dot.className = 'indicator';
      scaleEl.appendChild(dot);
    }
    scaleEl.querySelector('.indicator').style.left = `${pct}%`;
  }
}

function updateSpeed(d) {
  const spd = Number(d.speed) || 0;
  const el = document.getElementById('spd-value');
  const fill = document.getElementById('spd-fill');
  if (el) el.textContent = `${spd.toFixed(1)} m/s`;
  if (fill) fill.style.width = `${Math.min(100, (spd / 10) * 100)}%`;
}

function updateCompass(d) {
  const yaw = Number(d.yaw) || 0;
  const tape = document.getElementById('compass-tape');
  const val = document.getElementById('heading-value');
  if (!tape) return;

  // Generate compass markings: N, 045, 090, 135, S, 225, 270, 315
  const markings = ['N','045','090','135','S','225','270','315'];
  if (!tape.hasChildNodes()) {
    markings.forEach(m => {
      const span = document.createElement('span');
      span.textContent = m;
      span.style.cssText = 'font-size:9px;color:var(--text-muted);width:40px;text-align:center;flex-shrink:0;';
      tape.appendChild(span);
    });
  }

  // Scroll tape based on heading
  const offset = -((yaw / 360) * 320) + 140; // center current heading
  tape.style.transform = `translateX(${offset}px)`;

  if (val) val.textContent = `${Math.round(yaw)}°`;
}

function updateVSD(d) {
  const canvas = document.getElementById('vsd-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const w = canvas.width, h = canvas.height;
  const alt = Number(d.altitude) || 0;
  const relAlt = Number(d.relative_alt) || 0;

  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = '#2D354A';
  ctx.lineWidth = 0.5;

  // Ground line
  ctx.beginPath();
  ctx.moveTo(0, h - 20);
  ctx.lineTo(w, h - 20);
  ctx.stroke();

  // Altitude tick marks
  for (let a = 0; a <= 50; a += 5) {
    const y = h - 20 - (a / 50) * (h - 30);
    ctx.beginPath();
    ctx.moveTo(10, y);
    ctx.lineTo(a % 10 === 0 ? 30 : 20, y);
    ctx.stroke();
    if (a % 10 === 0) {
      ctx.fillStyle = '#4A5A70';
      ctx.font = '8px monospace';
      ctx.fillText(`${a}`, 32, y + 3);
    }
  }

  // Drone indicator
  const droneY = h - 20 - (relAlt / 50) * (h - 30);
  ctx.fillStyle = '#00D4FF';
  ctx.beginPath();
  ctx.moveTo(w / 2 - 6, droneY);
  ctx.lineTo(w / 2, droneY - 6);
  ctx.lineTo(w / 2 + 6, droneY);
  ctx.lineTo(w / 2, droneY + 6);
  ctx.closePath();
  ctx.fill();

  // Home altitude line
  ctx.strokeStyle = '#34D399';
  ctx.lineWidth = 0.5;
  ctx.setLineDash([3, 3]);
  ctx.beginPath();
  ctx.moveTo(0, h - 20);
  ctx.lineTo(w, h - 20);
  ctx.stroke();
  ctx.setLineDash([]);
}
```

---

## Phase 6: Zone C — Main Stage (Map + Overlays)

### 6.1 HTML

```html
<main id="main-stage" class="main-stage">
  <div class="map-container" id="map-container">
    <div id="map" class="map-element"></div>

    <!-- Map HUD Overlays (micrographic style) -->
    <div class="map-hud-top-left">
      <span class="frame-pill" id="hud-mode">MODE: ----</span>
    </div>
    <div class="map-hud-top-right">
      <span class="frame-pill armed" id="hud-arm">DISARMED</span>
    </div>

    <!-- Hazard Diamond (top-left of map) -->
    <div class="map-hazard" id="hazard-diamond">
      <div class="haz-top" id="haz-bat">87%</div>
      <div class="haz-left" id="haz-link">-65</div>
      <div class="haz-center" id="haz-sys">OK</div>
      <div class="haz-right" id="haz-gps">12</div>
      <div class="haz-bottom" id="haz-mode">GUIDED</div>
    </div>

    <!-- GOTO fly-to card -->
    <div class="callout-box" id="goto-sheet" hidden>
      <div class="callout-header">GOTO TARGET</div>
      <div class="callout-body">
        <div class="callout-row"><span>LAT</span><span id="goto-lat">--</span></div>
        <div class="callout-row"><span>LON</span><span id="goto-lon">--</span></div>
        <div class="callout-row"><span>DIST</span><span id="goto-dist">--</span></div>
        <div class="callout-row">
          <span>ALT</span>
          <input type="number" id="goto-alt" class="callout-input" value="5" min="2" max="50">
          <span>m</span>
        </div>
      </div>
      <div class="callout-actions">
        <button class="btn-stamp" id="goto-confirm">GO</button>
        <button class="btn-stamp" id="goto-cancel">CANCEL</button>
      </div>
    </div>

    <!-- Map Controls -->
    <div class="map-controls">
      <button class="map-ctrl-btn" id="btn-map-center" title="Center on drone">⊞</button>
      <button class="map-ctrl-btn active" id="btn-map-follow" title="Follow drone">⊟</button>
      <button class="map-ctrl-btn" id="btn-map-full" title="Fullscreen">⛶</button>
    </div>
  </div>
</main>
```

### 6.2 CSS

```css
.main-stage {
  position: relative;
  overflow: hidden;
  background: var(--bg-void);
}

.map-container {
  position: relative;
  width: 100%;
  height: 100%;
}
.map-element {
  width: 100%;
  height: 100%;
}

/* Map HUD */
.map-hud-top-left,
.map-hud-top-right {
  position: absolute;
  top: var(--space-2);
  z-index: 500;
  pointer-events: none;
}
.map-hud-top-left { left: var(--space-2); }
.map-hud-top-right { right: var(--space-2); }
.map-hud-top-left *,
.map-hud-top-right * { pointer-events: auto; }

/* Hazard Diamond — micrographic NFPA-inspired */
.map-hazard {
  position: absolute;
  top: 50px;
  left: var(--space-2);
  z-index: 500;
  display: grid;
  grid-template-columns: 28px 28px 28px;
  grid-template-rows: 28px 28px 28px;
  gap: 2px;
  transform: rotate(45deg);
  pointer-events: none;
}
.map-hazard > * {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 8px;
  font-weight: 700;
  font-family: var(--font-mono);
  border: 0.5px solid var(--border-hairline);
  transform: rotate(-45deg);
  pointer-events: none;
}
.haz-top { background: rgba(239,68,68,0.2); color: var(--accent-red); }
.haz-left { background: rgba(245,158,11,0.2); color: var(--accent-amber); }
.haz-right { background: rgba(52,211,153,0.2); color: var(--accent-green); }
.haz-bottom { background: rgba(0,212,255,0.15); color: var(--accent-cyan); }
.haz-center {
  background: rgba(0,212,255,0.08);
  color: var(--accent-cyan);
  font-size: 9px;
}

/* Call-out box (engineering blueprint style) */
.callout-box {
  position: absolute;
  bottom: 60px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 600;
  background: rgba(5,5,5,0.95);
  border: 1px solid var(--border-active);
  padding: var(--space-3);
  min-width: 200px;
  pointer-events: auto;
}
.callout-header {
  font-size: 9px;
  font-weight: 600;
  color: var(--accent-cyan);
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: var(--space-2);
  padding-bottom: var(--space-1);
  border-bottom: 0.5px solid var(--border-hairline);
}
.callout-row {
  display: flex;
  justify-content: space-between;
  gap: var(--space-3);
  font-size: 10px;
  padding: 2px 0;
}
.callout-row span:first-child { color: var(--text-muted); }
.callout-row span:last-child { color: var(--text-primary); }
.callout-input {
  width: 60px;
  background: transparent;
  border: 0.5px solid var(--border-hairline);
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: 10px;
  padding: 2px 4px;
  text-align: center;
}
.callout-actions {
  display: flex;
  gap: var(--space-2);
  margin-top: var(--space-2);
  padding-top: var(--space-2);
  border-top: 0.5px solid var(--border-hairline);
}

/* Map controls */
.map-controls {
  position: absolute;
  bottom: var(--space-3);
  right: var(--space-3);
  z-index: 500;
  display: flex;
  flex-direction: column;
  gap: 4px;
  pointer-events: none;
}
.map-ctrl-btn {
  width: var(--map-controls-size);
  height: var(--map-controls-size);
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(5,5,5,0.85);
  border: 0.5px solid var(--border-hairline);
  color: var(--text-secondary);
  font-size: 14px;
  cursor: pointer;
  pointer-events: auto;
  transition: all var(--transition-snap);
}
.map-ctrl-btn:hover {
  border-color: var(--accent-cyan);
  color: var(--accent-cyan);
  background: var(--bg-elevated);
}
.map-ctrl-btn.active {
  border-color: var(--accent-green);
  color: var(--accent-green);
}
```

### 6.3 Custom Map Tile Style

For the Leaflet map, replace `mapTileUrl` in `gcs_config.js` with a custom dark monochrome tile URL:

```javascript
// gcs_config.js update:
mapTileUrl: 'https://tiles.stadiamaps.com/tiles/alidade_smooth_dark/{z}/{x}/{y}{r}.png',
// OR for truly custom tiles:
// mapTileUrl: 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',
```

If neither works for the ESP32 (no external network), set the map to use a pre-rendered static tile or use a simpler gray canvas:

```javascript
// Fallback: no tiles, just a dark canvas with coordinate grid
mapTileUrl: null, // handled in gcs_map.js
```

### 6.4 Map Marker Styles (Micrographic)

```css
/* Drone marker — directional chevron with crosshair */
.drone-marker-wrap {
  background: transparent !important;
  border: none !important;
}
.drone-marker {
  position: relative;
  width: 28px; height: 28px;
}
.drone-marker .chevron {
  position: absolute;
  top: 50%; left: 50%;
  transform: translate(-50%, -50%);
  color: var(--accent-cyan);
  font-size: 20px;
  text-shadow: 0 0 8px rgba(0,212,255,0.5);
}
.drone-marker .crosshair {
  position: absolute;
  inset: 0;
  border: 0.5px solid rgba(0,212,255,0.3);
}
.drone-marker .crosshair::before,
.drone-marker .crosshair::after {
  content: '';
  position: absolute;
  background: rgba(0,212,255,0.2);
}
.drone-marker .crosshair::before {
  top: 50%; left: -4px;
  right: -4px; height: 0.5px;
}
.drone-marker .crosshair::after {
  left: 50%; top: -4px;
  bottom: -4px; width: 0.5px;
}

/* Home marker — "H" in diamond */
.home-marker-wrap {
  background: transparent !important;
  border: none !important;
}
.home-marker {
  width: 24px; height: 24px;
  transform: rotate(45deg);
  border: 1.5px solid var(--accent-green);
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(5,5,5,0.8);
}
.home-marker span {
  transform: rotate(-45deg);
  color: var(--accent-green);
  font-size: 10px;
  font-weight: 700;
}
```

---

## Phase 7: Zone D — Right Wing (Systems Status)

### 7.1 HTML

```html
<aside class="wing wing-right">
  <!-- Preflight Checklist -->
  <div class="panel" id="preflight-panel">
    <div class="panel-header">
      <span class="panel-title">▣ PREFLIGHT</span>
    </div>
    <div class="panel-body">
      <div class="preflight-list" id="preflight-list">
        <div class="pf-item" id="pf-wifi">
          <span class="pf-icon frame-circle">●</span>
          <span class="pf-label">WiFi</span>
          <span class="pf-status">WAITING</span>
        </div>
        <div class="pf-item" id="pf-gps">
          <span class="pf-icon frame-circle">●</span>
          <span class="pf-label">GPS 3D FIX</span>
          <span class="pf-status">WAITING</span>
        </div>
        <div class="pf-item" id="pf-mav">
          <span class="pf-icon frame-circle">●</span>
          <span class="pf-label">MAVLINK</span>
          <span class="pf-status">WAITING</span>
        </div>
        <div class="pf-item" id="pf-bat">
          <span class="pf-icon frame-circle">●</span>
          <span class="pf-label">BATTERY >20%</span>
          <span class="pf-status">WAITING</span>
        </div>
        <div class="pf-item" id="pf-safe">
          <span class="pf-icon frame-circle">●</span>
          <span class="pf-label">DISARMED</span>
          <span class="pf-status">WAITING</span>
        </div>
      </div>
    </div>
  </div>

  <!-- Telemetry Spec Cards -->
  <div class="panel" id="telem-panel">
    <div class="panel-header">
      <span class="panel-title">■ TELEMETRY</span>
    </div>
    <div class="panel-body">
      <div class="telem-grid">
        <div class="spec-card">
          <span class="spec-label">ALT</span>
          <span class="spec-value" id="tl-alt">--</span>
          <span class="spec-unit">m</span>
          <span class="spec-stamp">5 Hz</span>
        </div>
        <div class="spec-card">
          <span class="spec-label">SPD</span>
          <span class="spec-value" id="tl-spd">--</span>
          <span class="spec-unit">m/s</span>
          <span class="spec-stamp">5 Hz</span>
        </div>
        <div class="spec-card">
          <span class="spec-label">BAT</span>
          <span class="spec-value" id="tl-bat">--</span>
          <span class="spec-unit">%</span>
          <div class="spec-bar" id="bat-bar-wrap">
            <div class="measure-bar ticked">
              <div class="fill green" id="bat-fill" style="width:0%"></div>
            </div>
          </div>
        </div>
        <div class="spec-card">
          <span class="spec-label">HDG</span>
          <span class="spec-value" id="tl-hdg">--</span>
          <span class="spec-unit">°</span>
        </div>
        <div class="spec-card">
          <span class="spec-label">SATS</span>
          <span class="spec-value" id="tl-sats">--</span>
          <span class="spec-unit">sats</span>
        </div>
        <div class="spec-card">
          <span class="spec-label">DIST</span>
          <span class="spec-value" id="tl-dist">--</span>
          <span class="spec-unit">m</span>
        </div>
      </div>
    </div>
  </div>

  <!-- Sparkline Matrix (2×2) -->
  <div class="panel" id="sparkline-panel">
    <div class="panel-header">
      <span class="panel-title">▣ TRENDS</span>
    </div>
    <div class="panel-body">
      <div class="spark-matrix">
        <div class="spark-cell">
          <span class="spark-label">ALT</span>
          <canvas class="spark-canvas" id="spark-alt" width="120" height="36"></canvas>
        </div>
        <div class="spark-cell">
          <span class="spark-label">SPD</span>
          <canvas class="spark-canvas" id="spark-spd" width="120" height="36"></canvas>
        </div>
        <div class="spark-cell">
          <span class="spark-label">BAT</span>
          <canvas class="spark-canvas" id="spark-bat" width="120" height="36"></canvas>
        </div>
        <div class="spark-cell">
          <span class="spark-label">LINK</span>
          <canvas class="spark-canvas" id="spark-link" width="120" height="36"></canvas>
        </div>
      </div>
    </div>
  </div>

  <!-- Status Messages (compact) -->
  <div class="panel" id="msgs-panel">
    <div class="panel-header">
      <span class="panel-title">■ SYSTEM LOG</span>
    </div>
    <div class="panel-body msgs-body" id="fc-messages">
      <div class="fc-msg-empty">NO MESSAGES</div>
    </div>
  </div>
</aside>
```

### 7.2 CSS for Right Wing

```css
.wing-right {
  display: flex;
  flex-direction: column;
  gap: 0;
  overflow-y: auto;
  background: var(--bg-void);
  border-left: 1px solid var(--border-hairline);
}
.wing-right .panel {
  border: none;
  border-bottom: 1px solid var(--border-hairline);
  border-left: none;
}

/* Panel header */
.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-2) var(--space-3);
  background: var(--bg-elevated);
}
.panel-title {
  font-size: 10px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 1px;
  color: var(--text-secondary);
}
.panel-body { padding: var(--space-2); }

/* Preflight */
.preflight-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.pf-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: 6px var(--space-2);
  font-size: 10px;
  border: 0.5px solid transparent;
}
.pf-item.pass { border-color: var(--accent-green); }
.pf-item.pass .pf-icon { color: var(--accent-green); }
.pf-item.pass .pf-status { color: var(--accent-green); }
.pf-item.fail { border-color: var(--accent-red); }
.pf-item.fail .pf-icon { color: var(--accent-red); }
.pf-item.fail .pf-status { color: var(--accent-red); }
.pf-icon { font-size: 8px; width: 16px; text-align: center; color: var(--text-muted); }
.pf-label { color: var(--text-secondary); flex: 1; }
.pf-status { font-weight: 600; color: var(--text-muted); font-size: 8px; }

/* Telemetry Grid */
.telem-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 4px;
}
.spec-card {
  padding: var(--space-2);
  border: 0.5px solid var(--border-hairline);
  position: relative;
}
.spec-label {
  display: block;
  font-size: 8px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 2px;
}
.spec-value {
  display: block;
  font-size: 16px;
  font-weight: 700;
  color: var(--text-primary);
}
.spec-unit {
  font-size: 8px;
  color: var(--text-muted);
  margin-left: 2px;
}
.spec-stamp {
  position: absolute;
  top: 4px;
  right: 6px;
  font-size: 7px;
  color: var(--text-cert);
}
.spec-bar {
  margin-top: 4px;
}

/* Sparkline Matrix */
.spark-matrix {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 4px;
}
.spark-cell {
  padding: 4px;
}
.spark-label {
  font-size: 8px;
  color: var(--text-muted);
  display: block;
  margin-bottom: 2px;
}
.spark-canvas {
  width: 100%;
  height: 36px;
  display: block;
}

/* Status messages */
.msgs-body {
  max-height: 100px;
  overflow-y: auto;
  font-size: 9px;
  line-height: 1.6;
}
.fc-msg { color: var(--text-secondary); padding: 2px 0; border-bottom: 0.5px solid var(--border-hairline); }
.fc-msg.err { color: var(--accent-red); }
.fc-msg-empty { color: var(--text-muted); font-style: italic; text-align: center; padding: var(--space-2); }
```

---

## Phase 8: Zone E — Command Bar (Bottom Dock)

### 8.1 HTML

```html
<footer id="command-bar" class="command-bar">
  <div class="cmd-section">
    <span class="cmd-heading">FLIGHT COMMANDS</span>
    <div class="cmd-actions">
      <div class="cmd-group">
        <select id="mode-select" class="cmd-select">
          <option value="GUIDED" selected>GUIDED</option>
          <option value="LOITER">LOITER</option>
          <option value="STABILIZE">STABILIZE</option>
          <option value="LAND">LAND</option>
          <option value="RTL">RTL</option>
        </select>
        <button class="btn-stamp" id="btn-set-mode">APPLY</button>
      </div>
      <button class="btn-primary btn-arm" id="btn-arm" disabled>◆ ARM</button>
      <button class="btn-primary btn-danger" id="btn-disarm" disabled hidden>◆ DISARM</button>
      <div class="cmd-group">
        <button class="btn-stamp" id="btn-liftoff" disabled>TAKEOFF</button>
        <input type="number" id="takeoff-alt" class="cmd-alt-input" value="5" min="1" max="50" step="0.5">
        <span class="cmd-alt-unit">m</span>
      </div>
      <button class="btn-stamp" id="btn-land" disabled>LAND</button>
      <button class="btn-stamp btn-danger" id="btn-rtl" disabled>◆ RTL</button>
      <button class="btn-danger btn-emergency" id="btn-kill">⚡ EMERGENCY STOP</button>
    </div>
  </div>

  <div class="cmd-section">
    <span class="cmd-heading">PRECISION MOVEMENT</span>
    <div class="cmd-movement">
      <div class="move-pad">
        <button class="btn-move" data-move="x" data-sign="1" disabled>▲</button>
        <div class="move-row">
          <button class="btn-move" data-move="y" data-sign="-1" disabled>◄</button>
          <span class="move-center">BODY</span>
          <button class="btn-move" data-move="y" data-sign="1" disabled>►</button>
        </div>
        <button class="btn-move" data-move="x" data-sign="-1" disabled>▼</button>
      </div>
      <div class="move-extra">
        <div class="move-vert">
          <button class="btn-move" data-move="z" data-sign="-1" disabled>↑</button>
          <button class="btn-move" data-move="z" data-sign="1" disabled>↓</button>
        </div>
        <div class="move-yaw">
          <button class="btn-stamp" data-yaw="-90" disabled>YAW ◄ 90°</button>
          <button class="btn-stamp" data-yaw="90" disabled>YAW ► 90°</button>
        </div>
      </div>
      <div class="move-dist">
        <span class="dist-label">DIST</span>
        <div class="dist-presets">
          <button class="preset-btn active" data-dist="1">1m</button>
          <button class="preset-btn" data-dist="3">3m</button>
          <button class="preset-btn" data-dist="5">5m</button>
          <button class="preset-btn" data-dist="10">10m</button>
        </div>
        <input type="number" id="dist-custom" class="dist-input" min="0.5" max="200" step="0.5" placeholder="CUSTOM">
        <span class="dist-active" id="dist-label">1m</span>
      </div>
    </div>
    <div class="cmd-hint" id="move-hint">ARM IN GUIDED TO ENABLE MOVEMENT</div>
  </div>

  <div class="cmd-keys">
    <span>WASD=MOVE</span>
    <span>QE=YAW</span>
    <span>SPACE=ARM</span>
    <span>T=TAKEOFF</span>
    <span>L=LAND</span>
    <span>R=RTL</span>
    <span>?=HELP</span>
  </div>
</footer>
```

### 8.2 CSS

```css
.command-bar {
  background: var(--bg-panel);
  border-top: 1px solid var(--border-hairline);
  display: flex;
  flex-direction: column;
  padding: var(--space-2) var(--space-3);
  gap: var(--space-2);
  overflow-y: auto;
}

.cmd-heading {
  font-size: 8px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 1.5px;
  color: var(--text-muted);
  margin-bottom: var(--space-1);
}

.cmd-actions {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  flex-wrap: wrap;
}
.cmd-group {
  display: flex;
  align-items: center;
  gap: var(--space-1);
}
.cmd-select {
  background: var(--bg-void);
  border: 0.5px solid var(--border-hairline);
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: 10px;
  padding: 6px 8px;
}
.cmd-select:disabled { opacity: 0.35; }
.cmd-alt-input {
  width: 50px;
  background: transparent;
  border: 0.5px solid var(--border-hairline);
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: 10px;
  padding: 4px;
  text-align: center;
}
.cmd-alt-unit { color: var(--text-muted); font-size: 9px; }
.btn-emergency {
  padding: 10px 20px;
  background: rgba(239,68,68,0.15);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 1px;
  margin-left: auto;
}

/* Movement */
.cmd-movement {
  display: flex;
  gap: var(--space-4);
  align-items: flex-start;
}
.move-pad {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}
.move-row {
  display: flex;
  align-items: center;
  gap: 2px;
}
.move-center {
  width: 56px; height: 56px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 8px;
  color: var(--text-muted);
  letter-spacing: 1px;
}
.move-extra {
  display: flex;
  gap: var(--space-2);
  align-items: flex-start;
}
.move-vert {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.move-yaw {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}
.move-dist {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  flex-wrap: wrap;
}
.dist-label {
  font-size: 9px;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.dist-presets {
  display: flex;
  gap: 2px;
}
.preset-btn {
  padding: 4px 8px;
  font-size: 9px;
  font-family: var(--font-mono);
  border: 0.5px solid var(--border-hairline);
  background: transparent;
  color: var(--text-secondary);
  cursor: pointer;
}
.preset-btn.active {
  border-color: var(--accent-cyan);
  color: var(--accent-cyan);
}
.preset-btn:hover {
  border-color: var(--border-active);
}
.dist-input {
  width: 60px;
  background: transparent;
  border: 0.5px solid var(--border-hairline);
  color: var(--text-primary);
  font-family: var(--font-mono);
  font-size: 9px;
  padding: 4px;
}
.dist-active {
  font-size: 9px;
  color: var(--accent-cyan);
  font-weight: 600;
}

.cmd-hint {
  font-size: 9px;
  color: var(--text-muted);
  margin-top: var(--space-1);
}
.cmd-hint.ready { color: var(--accent-green); }

/* Keyboard shortcut strip */
.cmd-keys {
  display: flex;
  gap: var(--space-3);
  font-size: 8px;
  color: var(--text-muted);
  padding-top: var(--space-1);
  border-top: 0.5px solid var(--border-hairline);
}
```

---

## Phase 9: Zone F+G — Message Strip & Status Bar

### 9.1 HTML

```html
<!-- Message Strip -->
<div id="msg-strip" class="msg-strip">
  <span class="msg-icon">▶</span>
  <span id="msg-text" class="msg-text">SYSTEM INITIALIZING…</span>
  <button class="msg-expand" id="msg-expand">▼</button>
</div>

<!-- Expanded log overlay -->
<div id="log-overlay" class="log-overlay" hidden>
  <div class="log-overlay-header">
    <span class="panel-title">■ MESSAGE LOG</span>
    <button class="btn-stamp" id="log-close">CLOSE</button>
  </div>
  <div class="log-overlay-body" id="log-content"></div>
</div>

<!-- Status Strip -->
<div id="status-strip" class="status-strip">
  <span>BUILD: <span id="ss-build">FW12·FS15 ✓</span></span>
  <span class="ss-divider">|</span>
  <span>UPTIME: <span id="ss-uptime">00:00:00</span></span>
  <span class="ss-divider">|</span>
  <span><span id="ss-hz">5</span> Hz</span>
  <span class="ss-divider">|</span>
  <span>RSSI: <span id="ss-rssi">--</span> dBm</span>
  <span class="ss-divider">|</span>
  <span>IP: <span id="ss-ip">--</span></span>
  <span class="ss-divider">|</span>
  <span><span id="ss-sats">--</span> SATS</span>
</div>
```

### 9.2 CSS

```css
/* Message Strip */
.msg-strip {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: 4px var(--space-4);
  background: var(--bg-panel);
  border-top: 0.5px solid var(--border-hairline);
  border-bottom: 0.5px solid var(--border-hairline);
  font-size: 9px;
  cursor: pointer;
}
.msg-icon { color: var(--accent-cyan); font-size: 7px; }
.msg-text {
  flex: 1;
  color: var(--text-secondary);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.msg-expand {
  background: none;
  border: none;
  color: var(--text-muted);
  font-size: 8px;
  cursor: pointer;
}

/* Log Overlay */
.log-overlay {
  position: fixed;
  inset: 0;
  z-index: 400;
  background: rgba(5,5,5,0.95);
  display: flex;
  flex-direction: column;
}
.log-overlay-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-3) var(--space-4);
  border-bottom: 1px solid var(--border-hairline);
}
.log-overlay-body {
  flex: 1;
  overflow-y: auto;
  padding: var(--space-3);
  font-size: 10px;
  font-family: var(--font-mono);
  line-height: 1.7;
}
.log-entry {
  display: grid;
  grid-template-columns: 60px 50px 1fr;
  gap: var(--space-2);
  padding: 4px 0;
  border-bottom: 0.5px solid var(--border-hairline);
}
.log-ts { color: var(--text-muted); }
.log-tag { font-weight: 600; }
.log-tag.sys { color: var(--accent-cyan); }
.log-tag.fc { color: var(--accent-green); }
.log-tag.err { color: var(--accent-red); }
.log-tag.ack { color: var(--text-cert); }
.log-msg { color: var(--text-secondary); }

/* Status Strip */
.status-strip {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: 0 var(--space-4);
  font-size: 9px;
  color: var(--text-muted);
  background: var(--bg-void);
  border-top: 0.5px solid var(--border-hairline);
}
.ss-divider { color: var(--border-hairline); font-size: 7px; }
.status-strip span { white-space: nowrap; }
```

---

## Phase 10: WebSocket Integration & Data Flow

### 10.1 Data Architecture

The `gcs.js` file manages:
1. WebSocket connection lifecycle
2. Telemetry update dispatch to all UI zones
3. Command sending
4. Keyboard shortcut handling
5. Connection recovery

Keep the existing WebSocket protocol from the current prototype — it works. Adapt the message handler to update the new micrographic UI.

### 10.2 WebSocket Handler (gcs.js core)

```javascript
// Existing connection logic — keep from prototype, adapt message handler:
ws.onmessage = (e) => {
  try {
    const d = JSON.parse(e.data);
    switch (d.event) {
      case 'HEARTBEAT':
        // Dispatch to all micrographic UI functions
        updateCertBar(d);     // Zone A
        updateAttitude(d);    // Zone B
        updateAltitude(d);    // Zone B
        updateSpeed(d);       // Zone B
        updateCompass(d);     // Zone B
        updateVSD(d);         // Zone B
        updatePreflight(d);   // Zone D
        updateTelemetry(d);   // Zone D
        updateSparklines(d);  // Zone D
        updateMapActions(d);  // Zone C
        updateStatusStrip(d); // Zone G
        updateMessageStrip(d);// Zone F
        updateHazard(d);      // Zone C — hazard diamond

        // Push to sparkline data buffers
        pushSparkline('alt', d.altitude);
        pushSparkline('spd', d.speed);
        pushSparkline('bat', d.battery);
        pushSparkline('link', d.wifi_rssi);

        // Update map
        if (typeof SkylinkMap !== 'undefined')
          SkylinkMap.updateFromTelemetry(d);

        updateBuildTag(d);
        break;

      case 'ACK':
        handleAck(d);
        break;

      case 'PONG':
        log('PING', 'ack', `RTT ${Math.round(performance.now() - pingTimestamp)} ms`);
        break;

      case 'ERROR':
        log('FC', 'err', d.message || 'Unknown error');
        break;
    }
  } catch (err) {
    log('SYS', 'err', 'Invalid message');
  }
};
```

### 10.3 Command Sending

Keep the existing `sendCmd()` function from the prototype. It already works:

```javascript
function sendCmd(command, extra) {
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  const msg = {
    v: CFG.protocolVersion || 1,
    type: 'command',
    command,
    ...(extra || {})
  };
  ws.send(JSON.stringify(msg));
  log('TX', 'sys', `${command} ${extra ? JSON.stringify(extra) : ''}`);
}
```

### 10.4 Button Event Wiring

```javascript
function initCommandBar() {
  // Mode selector
  document.getElementById('btn-set-mode')?.addEventListener('click', () => {
    const mode = document.getElementById('mode-select').value;
    sendCmd('SET_FLIGHT_MODE', { mode });
  });

  // Arm / Disarm
  document.getElementById('btn-arm')?.addEventListener('click', () => {
    sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
    setTimeout(() => sendCmd('ARM_DRONE'), 400);
  });
  document.getElementById('btn-disarm')?.addEventListener('click', () => {
    sendCmd('DISARM_DRONE');
  });

  // Takeoff
  document.getElementById('btn-liftoff')?.addEventListener('click', () => {
    const alt = parseFloat(document.getElementById('takeoff-alt').value) || 5;
    sendCmd('SET_FLIGHT_MODE', { mode: 'GUIDED' });
    setTimeout(() => sendCmd('ARM_DRONE'), 400);
    setTimeout(() => sendCmd('TAKEOFF', { altitude: alt }), 900);
  });

  // Land / RTL
  document.getElementById('btn-land')?.addEventListener('click', () => sendCmd('LAND'));
  document.getElementById('btn-rtl')?.addEventListener('click', () => sendCmd('RTL'));

  // Emergency Stop
  document.getElementById('btn-kill')?.addEventListener('click', () => {
    if (confirm('EMERGENCY STOP — confirm?')) {
      sendCmd('DISARM_DRONE');
    }
  });

  // Movement buttons
  document.querySelectorAll('.btn-move').forEach(btn => {
    btn.addEventListener('click', () => {
      const axis = btn.dataset.move;
      const sign = parseFloat(btn.dataset.sign) || 1;
      sendMoveBody(axis, sign * getMoveDistance());
    });
  });

  // Yaw buttons
  document.querySelectorAll('[data-yaw]').forEach(btn => {
    btn.addEventListener('click', () => {
      sendYawRelative(parseFloat(btn.dataset.yaw));
    });
  });

  // Distance presets
  document.querySelectorAll('.preset-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const dist = parseFloat(btn.dataset.dist);
      selectDistance(dist);
    });
  });

  // Custom distance
  document.getElementById('dist-custom')?.addEventListener('input', (e) => {
    const v = parseFloat(e.target.value);
    if (v > 0 && v <= 200) selectDistance(v, true);
  });

  // GOTO handlers
  document.getElementById('goto-confirm')?.addEventListener('click', confirmGoto);
  document.getElementById('goto-cancel')?.addEventListener('click', closeGotoSheet);
}
```

### 10.5 Keyboard Shortcuts

```javascript
function initKeyboard() {
  document.addEventListener('keydown', (e) => {
    if (isTypingInField()) return;
    if (document.getElementById('log-overlay').hidden === false) return;

    const dist = getMoveDistance();
    let handled = true;

    switch (e.code) {
      case 'ArrowUp':    sendMoveBody('x', dist); break;
      case 'ArrowDown':  sendMoveBody('x', -dist); break;
      case 'ArrowLeft':  sendMoveBody('y', -dist); break;
      case 'ArrowRight': sendMoveBody('y', dist); break;
      case 'KeyW':       sendMoveBody('x', dist); break;
      case 'KeyS':       sendMoveBody('x', -dist); break;
      case 'KeyA':       sendMoveBody('y', -dist); break;
      case 'KeyD':       sendMoveBody('y', dist); break;
      case 'ShiftLeft':
      case 'ShiftRight':
        if (e.shiftKey && e.code.startsWith('Key')) {
          // handled by shift+W/S for vertical
        }
        break;
      case 'KeyQ':       sendYawRelative(-90); break;
      case 'KeyE':       sendYawRelative(90); break;
      case 'Space':      document.getElementById('btn-arm')?.click(); break;
      case 'KeyT':       document.getElementById('btn-liftoff')?.click(); break;
      case 'KeyL':       sendCmd('LAND'); break;
      case 'KeyR':       sendCmd('RTL'); break;
      case 'KeyF':       toggleFollow(); break;
      case 'KeyC':       centerDrone(); break;
      case 'Digit1':     selectDistance(1); break;
      case 'Digit2':     selectDistance(3); break;
      case 'Digit3':     selectDistance(5); break;
      case 'Digit4':     selectDistance(10); break;
      case 'Slash':      showHelp(); break;
      default:           handled = false;
    }

    if (handled) e.preventDefault();
  });
}
```

---

## Phase 11: Micro-Interactions & Animation

### 11.1 Animation Catalog

All animations use CSS transitions and brief keyframe animations. No animation library needed.

```css
/* Mechanical counter effect — value transitions */
.value-counter {
  transition: opacity var(--transition-snap) linear;
}
.value-counter.old {
  opacity: 0;
  transform: translateY(-4px);
}
.value-counter.new {
  opacity: 1;
  transform: translateY(0);
}

/* Corner bracket morph on state change */
.panel::before,
.panel::after {
  transition: opacity var(--transition-normal), border-color var(--transition-normal);
}

/* Link chip state transition */
.link-chip {
  transition: border-color var(--transition-fast), color var(--transition-fast);
}

/* Button inversion on press */
.btn-primary {
  transition: background var(--transition-snap), color var(--transition-snap);
}

/* Map marker glow pulse */
@keyframes marker-pulse {
  0%, 100% { text-shadow: 0 0 4px rgba(0,212,255,0.3); }
  50% { text-shadow: 0 0 12px rgba(0,212,255,0.6); }
}
.drone-marker .chevron.active {
  animation: marker-pulse 1.5s ease-in-out infinite;
}

/* Loading dots animation */
@keyframes loading-dots {
  0%, 20% { opacity: 0.2; }
  50% { opacity: 1; }
  80%, 100% { opacity: 0.2; }
}
.loading-dots span {
  animation: loading-dots 1.4s ease-in-out infinite;
}
.loading-dots span:nth-child(1) { animation-delay: 0s; }
.loading-dots span:nth-child(2) { animation-delay: 0.2s; }
.loading-dots span:nth-child(3) { animation-delay: 0.4s; }

/* Reconnect overlay */
.loading-overlay {
  display: none;
  position: fixed;
  inset: 0;
  z-index: var(--z-loading);
  background: rgba(5,5,5,0.92);
  backdrop-filter: blur(4px);
  flex-direction: column;
  align-items: center;
  justify-content: center;
}
.loading-overlay.show { display: flex; }
.loading-sandwich {
  text-align: center;
}
.sandwich-top, .sandwich-bottom {
  font-size: 9px;
  color: var(--text-muted);
  letter-spacing: 2px;
  text-transform: uppercase;
}
.sandwich-top { margin-bottom: var(--space-6); }
.sandwich-bottom { margin-top: var(--space-6); }
.sandwich-center {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--space-3);
}
.loading-logo {
  font-size: 48px;
  color: var(--accent-cyan);
  animation: pulse 2s ease-in-out infinite;
}
.loading-text {
  font-size: 14px;
  font-weight: 600;
  color: var(--accent-amber);
  letter-spacing: 2px;
}

/* Preflight item pass animation */
.pf-item {
  transition: border-color var(--transition-normal);
}
.pf-item.pass {
  border-color: var(--accent-green);
}
.pf-item.fail {
  border-color: var(--accent-red);
}

/* Command sent feedback — bracket flash */
.cmd-bracket-flash {
  animation: bracket-flash 0.3s ease-out;
}
@keyframes bracket-flash {
  0% { opacity: 0.3; }
  50% { opacity: 1; }
  100% { opacity: 0.4; }
}

/* Telem value flash on update */
.spec-value.flash-green {
  animation: flash-green var(--transition-fast) ease-out;
}
@keyframes flash-green {
  0% { color: var(--accent-green); }
  100% { color: var(--text-primary); }
}
.spec-value.flash-red {
  animation: flash-red var(--transition-fast) ease-out;
}
@keyframes flash-red {
  0% { color: var(--accent-red); }
  100% { color: var(--text-primary); }
}
```

### 11.2 JS Animation Triggers

```javascript
// Flash a value element on change
function flashValue(el, newVal, oldVal) {
  if (newVal > oldVal) {
    el.classList.remove('flash-green', 'flash-red');
    void el.offsetWidth; // reflow
    el.classList.add('flash-green');
  } else if (newVal < oldVal) {
    el.classList.remove('flash-green', 'flash-red');
    void el.offsetWidth;
    el.classList.add('flash-red');
  }
}

// Command sent feedback
function flashBrackets(panel) {
  const brackets = panel.querySelectorAll('::before, ::after');
  // Can't target pseudo-elements directly, so add a class to the panel
  panel.classList.remove('cmd-bracket-flash');
  void panel.offsetWidth;
  panel.classList.add('cmd-bracket-flash');
  setTimeout(() => panel.classList.remove('cmd-bracket-flash'), 300);
}
```

---

## Phase 12: Accessibility & Performance

### 12.1 Accessibility Checklist (from ui-ux-pro-max)

- [ ] All interactive elements have `cursor: pointer`
- [ ] Focus rings visible on keyboard navigation (`:focus-visible`)
- [ ] ARIA labels on all panels (`role`, `aria-label`, `aria-live`)
- [ ] Color is never the sole indicator — use geometric frames + text
- [ ] `prefers-reduced-motion` respected
- [ ] Minimum contrast ratio 4.5:1 for all text
- [ ] Tab order: Command Bar → Map → Left Wing → Right Wing → Status Bar
- [ ] Semantic HTML elements (`header`, `main`, `aside`, `footer`, `nav`)
- [ ] Screen reader friendly — log has `aria-live="polite"`

### 12.2 Performance Budget

| Metric | Target | How |
|--------|--------|-----|
| Initial paint | <200ms | Minimal CSS, no blocking JS |
| First meaningful paint | <400ms | Above-fold content (cert bar + left wing) in initial HTML |
| Time to interactive | <800ms | Leaflet + gcs.js loaded async |
| Telemetry update render | <10ms | Batch DOM updates, avoid layout thrashing |
| Map interaction | 60fps | Leaflet is already optimized |
| Animation frame budget | <8ms | No JS animations — use CSS transitions |
| Memory | <60MB | No canvas memory leaks, prune sparkline buffers |

### 12.3 Performance Techniques

```javascript
// Batch DOM updates — only write once per heartbeat
function updateTelemetry(d) {
  // All DOM reads first
  const altEl = document.getElementById('tl-alt');
  const spdEl = document.getElementById('tl-spd');
  // All DOM writes after
  if (altEl) altEl.textContent = (Number(d.altitude) || 0).toFixed(1);
  if (spdEl) spdEl.textContent = (Number(d.speed) || 0).toFixed(1);
  // Use requestAnimationFrame if needed for complex updates
}

// Sparkline rendering — use canvas, throttle to 10fps max
let lastSparkRender = 0;
function renderSparklines() {
  const now = performance.now();
  if (now - lastSparkRender < 100) return;
  lastSparkRender = now;
  // draw on canvases...
}

// Lazy panel rendering — only update visible panels
const visiblePanels = new Set(['attitude', 'altitude', 'speed']);
function setPanelVisible(name) {
  visiblePanels.add(name);
}
```

### 12.4 Animation Performance

- All animations use `transform` and `opacity` only (GPU-composited)
- No `top`, `left`, `width`, `height` animations (triggers layout)
- `will-change: transform` on animated elements
- `prefers-reduced-motion` disables everything except the pulse on the reconnection dot

---

## Phase 13: Mobile Responsive

### 13.1 CSS Breakpoints

```css
/* Tablet landscape (1024-1600px) */
@media (max-width: 1600px) {
  .gcs-grid {
    grid-template-columns: 240px 1fr 280px;
  }
  :root {
    --wing-left-width: 240px;
    --wing-right-width: 280px;
  }
}

/* Tablet portrait (768-1024px) — 2-column */
@media (max-width: 1024px) {
  .gcs-grid {
    grid-template-columns: 1fr 1fr;
    grid-template-rows:
      var(--bar-height)
      auto 1fr
      auto
      var(--status-strip-height);
    grid-template-areas:
      "cert   cert"
      "main   main"
      "left   right"
      "cmd    cmd"
      "msg    msg"
      "status status";
  }
  .wing-left {
    max-height: 50vh;
    overflow-y: auto;
  }
  .wing-right {
    max-height: 50vh;
    overflow-y: auto;
  }
  .main-stage {
    min-height: 30vh;
  }
}

/* Mobile (<768px) — single column */
@media (max-width: 768px) {
  .gcs-grid {
    grid-template-columns: 1fr;
    grid-template-rows:
      auto
      40vh
      auto
      auto
      auto
      auto
      auto;
    grid-template-areas:
      "cert"
      "main"
      "left"
      "right"
      "cmd"
      "msg"
      "status";
  }
  .cert-bar {
    flex-wrap: wrap;
    gap: var(--space-1);
    padding: var(--space-1) var(--space-2);
  }
  .cert-center .link-chip:nth-child(n+3) { display: none; }
  .cmd-keys { display: none; }
  .cmd-movement {
    flex-direction: column;
  }
  .telem-grid {
    grid-template-columns: 1fr 1fr;
  }
  .panel-header {
    padding: var(--space-1) var(--space-2);
  }
  .panel-body {
    padding: var(--space-1);
  }
  .alt-primary { font-size: 20px; }
  .spec-value { font-size: 14px; }
  .map-hazard { display: none; }
  #vsd-panel { display: none; }
  .cmd-group { flex-wrap: wrap; }
}
```

### 13.2 Mobile-Specific Adaptations

- All touch targets minimum 44×44px
- Map fills 40% viewport, scrollable below
- Command bar becomes a compact bottom sticky bar
- Hazard diamond hidden on mobile
- VSD hidden on mobile
- Links chips collapse to 2 visible on mobile

---

## Phase 14: Testing & Verification

### 14.1 Unit Tests (Manual, Browser Console)

```javascript
// Test 1: WebSocket connection
// Open browser console on the ESP32's IP, check:
// 'WS ●' shows green in cert bar
// Telemetry values update every 200ms

// Test 2: Command sending
// Click ARM button, check log shows:
// "TX: SET_FLIGHT_MODE" then "TX: ARM_DRONE"

// Test 3: Keyboard shortcuts
// Press W → log shows "TX: MOVE_BODY"
// Press Space → log shows "TX: SET_FLIGHT_MODE" + "TX: ARM_DRONE"

// Test 4: Connection recovery
// Unplug ESP32 → overlay shows within 5s
// Plug back → overlay disappears, data resumes

// Test 5: Map interaction
// Click on map → callout box appears with coordinates
// Click GO → log shows "TX: GOTO_LATLON"
```

### 14.2 Visual Verification Checklist (from ui-ux-pro-max pre-delivery)

- [ ] No emojis used as UI icons (use SVG sprite instead)
- [ ] All icons from consistent set
- [ ] Hover states don't cause layout shift
- [ ] `cursor: pointer` on all clickable elements
- [ ] Hover states provide clear visual feedback
- [ ] Transitions are 100-300ms
- [ ] Focus states visible for keyboard navigation (`:focus-visible`)
- [ ] Text contrast 4.5:1 minimum (WCAG AA for micro-detail text)
- [ ] No content hidden behind fixed elements
- [ ] Responsive at 375px, 768px, 1024px, 1440px
- [ ] No horizontal scroll on mobile
- [ ] `prefers-reduced-motion` respected
- [ ] All images have alt text
- [ ] Color is not the only indicator (geometric shapes + text)

### 14.3 Anti-Patterns to Verify Against

From the micrographic vision doc + ui-ux-pro-max (row 93 "Space Tech/Aerospace"):

| Anti-Pattern | Severity | How to Check |
|--------------|----------|--------------|
| Glassmorphism on data panels | HIGH | No frosted glass on telemetry cards |
| Rounded corners on data panels | HIGH | Only the map container gets rounded corners |
| Overuse of icons replacing text | MEDIUM | Every icon has a text label nearby |
| Sound/motion without user initiation | HIGH | No auto-playing animations |
| Light mode default | HIGH | Must be dark by default with no light mode toggle |
| Generic web aesthetic | HIGH | Must feel like a spacecraft console, not a blog |
| Slow telemetry updates | HIGH | DOM updates must complete within 10ms per heartbeat |
| Hidden controls | MEDIUM | All buttons visible, disabled with explanation tooltip |
| Confusing emergency button | HIGH | RTL and STOP always visible, prominently red |

---

## Phase 15: ESP32 Deployment

### 15.1 Build & Deploy Sequence

```bash
# Step 1: Build the dashboard files
# On dev machine, run esbuild to minify:
esbuild dev/gcs.js --minify --outfile=data/gcs.js
esbuild dev/gcs.css --minify --outfile=data/gcs.css

# Step 2: Copy fonts to data/fonts/
cp -r dev/fonts/ data/fonts/

# Step 3: Update build version
# Edit include/skylink_config.h:
#   #define SKYLINK_FS_BUILD 16  (increment from previous)
# Edit data/gcs_config.js:
#   fsBuild: 16,  (must match)

# Step 4: Upload filesystem to ESP32
pio run --target uploadfs

# Step 5: Upload firmware (if changed)
pio run --target upload

# Step 6: Verify
# Open http://<ESP32_IP>/ in browser
# Check cert bar shows correct build version
# Wait for WebSocket connection
# Verify all zones render correctly
```

### 15.2 Build Version Tracking

```javascript
// gcs_config.js — update this every time the FS changes
const SKYLINK_GCS_CONFIG = {
  protocolVersion: 1,
  fsBuild: 16,           // bump before uploadfs
  version: 'micrographic-b', // which design variant

  // ESP32 constraints
  moveMinM: 0.5,
  moveMaxM: 200,
  geofenceRadiusM: 1000,
  gotoAltMinM: 2,
  gotoAltMaxM: 50,
  gotoAltOffsetM: 5,
  keyboardMoveThrottleMs: 500,
  flightStateStableSamples: 3,
  defaultTab: 'main',

  // Map defaults
  mapDefaultLat: -35.363261,
  mapDefaultLng: 149.16523,
  mapDefaultZoom: 17,
  mapFollowDrone: true,
  mapTrailMaxPoints: 120,
  mapTileUrl: 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png',

  // Connection
  wsReconnectInitialMs: 1500,
  wsReconnectMaxMs: 20000,

  // Safety limits (must match skylink_config.h)
  takeoffAltMinM: 1,
  takeoffAltMaxM: 50,
  defaultTakeoffAltM: 5,

  // UI
  commsLogMaxEntries: 40,
  movePresetsM: [1, 3, 5, 10],
  simulationBanner: true,

  // Sparkline buffer
  sparklineMaxPoints: 60,
};
```

---

## Design System Summary

### One-line Summary

**Micrographic HUD + Industrial Spec Plate.** A Bloomberg-grade 3-column ground control station that looks like a spacecraft certification plate crossed with an NFPA hazard diamond. Cyan-on-black telemetry values. Hairline borders. Geometric frames. Certification stamps on every data point.

### Design Quick Reference

| Aspect | Decision |
|--------|----------|
| Layout | 3-column Bloomberg grid, no tabs |
| Palette | Black (#050505) + cyan (#00D4FF) + amber (#F59E0B) |
| Data density | High — every panel has label, value, micro-detail, stamp |
| Visual style | Square corners, corner brackets, hairline borders |
| Typography | JetBrains Mono (numbers) + Inter (labels) |
| Icons | Hairline SVG, outlined only, in geometric frames |
| Brand | Certification plate with monogram-in-diamond |
| Map | Custom monochrome dark tiles |
| Mobile | Full responsive, simplified on <768px |
| Tech | Vanilla JS + CSS + Leaflet, no frameworks |
| Size budget | <220KB total (well under 500KB ESP32 cap) |

### Key File Manifest

| File | Content | Size Target |
|------|---------|-------------|
| `data/index.html` | Semantic HTML shell + SVG icon sprite | <12KB |
| `data/gcs.css` | Full design system + all component styles | <30KB |
| `data/gcs.js` | All application logic + animations | <60KB |
| `data/gcs_config.js` | Configuration constants | <2KB |
| `data/gcs_map.js` | Leaflet map module | <15KB |
| `data/fonts/jetbrains-mono-subset.woff2` | Subsetted monospace | <25KB |
| `data/fonts/inter-subset.woff2` | Subsetted sans-serif | <30KB |
| `include/skylink_config.h` | Firmware safety limits + build version | Already exists |
| `README.md` | Build & usage instructions | Already exists |

---

## Reference Index

### Files Referenced

| File | Location |
|------|----------|
| Vision design doc (Option A) | `/docs/amazing-ui/skylink-gcs-vision-and-design.md` |
| Micrographic vision doc (Option B) | This directory: `skylink-gcs-micrographic-vision-and-design.md` |
| Micrographic reference images | `ref-images/` (10 images in this directory) |
| Current prototype HTML | `/data/index.html` |
| Current prototype CSS | `/data/gcs.css` |
| Current prototype JS | `/data/gcs.js` |
| Current prototype config | `/data/gcs_config.js` |
| Current map module | `/data/gcs_map.js` |
| ui-ux-pro-max skill | `/.agent/skills/ui-ux-pro-max/SKILL.md` |
| Skill styles database | `/.agent/skills/ui-ux-pro-max/data/styles.csv` (67 styles) |
| Skill colors database | `/.agent/skills/ui-ux-pro-max/data/colors.csv` (96 palettes) |
| Skill reasoning database | `/.agent/skills/ui-ux-pro-max/data/ui-reasoning.csv` (101 rules) |
| Firmware config | `/include/skylink_config.h` |
| ESP32 web server | `/src/web_server.cpp` |
| Flight controller | `/include/flight_controller.h` |

### Skill Rules Applied

From ui-ux-pro-max (`ui-reasoning.csv`):

| Row | Rule | Applied As |
|-----|------|------------|
| 93 | Space Tech/Aerospace → HUD/Sci-Fi FUI + Dark Mode | Primary style choice |
| 97 | Autonomous Drone Fleet → HUD + Real-Time | Confirms telemetry-first design |
| 54 | Smart Home/IoT Dashboard → Dark Mode + Status indicators | Secondary validation |
| 19 | SaaS Dashboard → Data-Dense | Telemetry spec card grid approach |
| 31 | Real-Time Monitoring → Alert pulse + streaming | Hazard diamond + flash animations |
| 44 | Financial Dashboard → Dark bg + Red/Green alerts | Color semantics for criticality |

### UI-UX Anti-Patterns Checked

From `ui-reasoning.csv` anti-pattern columns:
- ✗ Ornate design + Slow rendering (HUD style)
- ✗ Generic design + No immersion (micrographic spec plate avoids this)
- ✗ Light mode default (dark only)
- ✗ Playful design + Hidden credentials (authoritative certification style)
- ✗ Poor spatial viz (VSD canvas + hazard diamond)

---

*End of execution plan. Follow the phases in order. Each phase produces files that the next phase depends on. Test each phase before moving to the next.*

*"Every detail has been tested. Every pixel is certified."*