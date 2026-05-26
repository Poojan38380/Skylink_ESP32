# Skylink GCS — Option B: The Micrographic Dashboard

> **Document purpose:** This is the companion to Option A (`/docs/amazing-ui/skylink-gcs-vision-and-design.md`). It takes everything Option A established — the Bloomberg Terminal multi-pane layout, keyboard-first operation, PFD, telemetry grid, dark brand — and layers in a **micrographic design language**. This document is Option B: the same functional foundation, but expressed through the visual grammar of industrial labels, engineering schematics, hazard warnings, and modular vector graphics.
>
> Use this alongside Option A to decide which visual direction to build. The reference images live in `ref-images/` alongside this file.

---

## Table of Contents

1. [What Is Micrographic Design?](#1-what-is-micrographic-design)
2. [Why Micrographics for a GCS?](#2-why-micrographics-for-a-gcs)
3. [The Reference Images — What We Learned](#3-the-reference-images)
4. [Brand & Identity — The Micrographic Lens](#4-brand--identity)
5. [The Complete Layout Blueprint](#5-the-complete-layout-blueprint)
6. [Visual Language — Micrographic UI Components](#6-visual-language)
7. [Typography System](#7-typography-system)
8. [Color System](#8-color-system)
9. [Iconography & Symbols](#9-iconography--symbols)
10. [Data Visualization — The Micrographic Way](#10-data-visualization)
11. [Interactions & Animation](#11-interactions--animation)
12. [The Modular Graphic System](#12-the-modular-graphic-system)
13. [Micrographic Dashboard Zones — Detailed](#13-micrographic-dashboard-zones)
14. [Mobile & Responsive](#14-mobile--responsive)
15. [Technical Constraints](#15-technical-constraints)
16. [Differences from Option A](#16-differences-from-option-a)
17. [Reference Image Catalog](#17-reference-image-catalog)

---

## 1. What Is Micrographic Design?

Micrographic design (also known as "micro-typography," "industrial graphic design," or "neo-industrial" styling) is a visual language that draws inspiration from:

- **Industrial safety labels** — Warning diamonds, GHS hazard symbols, ECE regulatory markings, fire diamonds
- **Engineering blueprints** — GD&T symbols, tolerance callouts, dimension lines, section views
- **Chemical & pharmaceutical packaging** — CAS numbers, dosage matrices, NFPA 704 diamonds, barcode matrices
- **Technical schematics** — Wireframe 3D projections, coordinate grids, isometric diagrams, measurement scales
- **Aviation/aerospace spec plates** — Serial number plates, certification stamps, material composition labels
- **Scientific instruments** — Oscilloscope grids, spectrometer readouts, calibration scales

**The core philosophy:** Information is beautiful. Small details — a tiny serial number, a diamond warning symbol, a hairline crosshair — carry as much visual weight as the headline. The aesthetic emerges from the density and precision of functional artifacts.

### Key visual characteristics

| Characteristic | Description |
|----------------|-------------|
| **High information density** | Every square millimeter carries meaning. Empty space is deliberate, not accidental. |
| **Scale contrast** | Very large numbers sit next to very tiny labels. The viewer's eye moves between macro and micro. |
| **Precision linework** | Hairline strokes (0.5-1px), perfectly straight, mathematically aligned. No hand-drawn warmth. |
| **Geometric framing** | Circles, diamonds, squares, stadiums — shapes carry semantic meaning (diamond = hazard, circle = certification, square = data) |
| **Functional hierarchy** | Information is ordered not by aesthetic preference but by operational importance — just like a real spec plate. |
| **Monochrome + accent** | Base palette is high-contrast black/white, with a single functional accent (safety orange, warning yellow, certification cyan) |
| **Modularity** | Every element is a component that could exist independently. The dashboard is assembled from these components. |

### NOT micrographic design

Micrographics is often confused with other styles. It is NOT:
- **Cyberpunk glitch art** — Those use distortion and noise for atmosphere. Micrographics uses precision and clarity.
- **Brutalist web design** — Brutalism uses oversized, awkward elements intentionally. Micrographics uses tiny, refined elements.
- **Skeuomorphism** — Skeuomorphism mimics physical textures. Micrographics mimics functional documentation.
- **Data visualization** — Charts and graphs are data tools. Micrographics is a typographic and symbolic language.

---

## 2. Why Micrographics for a GCS?

A Ground Control Station is, at its heart, a **safety-critical information system**. Micrographic design was born from the same world:

**Aircraft certification plates** literally use micrographics. The spec plate riveted to every aircraft frame contains type certificates, serial numbers, weight limits, and regulatory approvals — all in tiny, precise type. A GCS dashboard should feel like that plate, but alive.

**Industrial hazard labels** (the GHS diamonds, NFPA 704 squares) are designed for split-second recognition. A fire diamond tells you everything you need to know about a chemical's danger profile in under one second. A drone GCS needs the same: a battery icon that instantly tells you the voltage is critically low, a link indicator that shows signal strength like an NFPA diamond shows hazard level.

**The "spec sheet" aesthetic** conveys authority. When an operator opens the Skylink GCS, they should feel like they're at a NASA console or in a certified aircraft cockpit — not browsing a web app. Micrographics creates that feeling through authentic visual language borrowed from aerospace and industrial engineering.

### Where micrographics solves GCS problems

| GCS Problem | Micrographic Solution |
|-------------|----------------------|
| "All data looks equally important" | Diamond+square+circle framing immediately groups data by criticality |
| "I can't tell if this value is normal" | Measurement scales with redline bands, like a real gauge |
| "The UI feels like a web app, not a tool" | Spec-plate aesthetic creates tool-like authority |
| "Too much screen real estate wasted" | Micro-detail fills space with purposeful information |
| "I don't know what state the system is in" | Status encoded in geometric shapes — not just colors |
| "The dashboard looks generic" | Unique visual language creates memorable brand identity |

---

## 3. The Reference Images — What We Learned

We studied 10 reference images. Each contributed specific insights to the design language.

### 3.1 Industrial Safety Label (orange-on-black)

**File:** `micrographics-industrial-safety-label-orange-on-black.jpeg`

**What it teaches:**
- Orange-on-black is an authoritative, urgent palette. Safety orange communicates "pay attention" without the panic of red.
- The "HANDCRAFTED" pill badge shows how to highlight one feature among many dense elements.
- ECE R43 regulatory code format can be adapted for Skylink protocol/firmware version stamps.
- The monogram-in-diamond (RDS logo) shows how a brand mark can feel like a certification stamp.

**Apply to dashboard:** Use an orange-accented certification badge near the Skylink logo showing "FW12 · FS15 ✓ PROTOCOL v1" like a regulatory stamp. Link status chips can borrow the diamond framing.

### 3.2 Perplexity Labs Brutalist Brand Identity

**File:** `micrographics-perplexity-labs-brutalist-brand-identity.jpeg`

**What it teaches:**
- Monochrome (pure black/white) creates the most serious, technical tone possible.
- Hairline-thin icons (monogram, star, orbit, burst) communicate precision engineering.
- The "sandwich layout" — identical text blocks top and bottom framing the central content — is a strong compositional device for dashboard headers and footers.
- Technical test URLs and version codes (PL-2023, LABS.PERPLEXITY.AI) feel authentic when placed as footer stamps.

**Apply to dashboard:** Use the "sandwich" layout for the connection overlay and loading screens. Adopt hairline SVG icons for telemetry tiles.

### 3.3 Technical Engineering Schematic Blueprint

**File:** `micrographics-technical-engineering-schematic-blueprint.jpeg`

**What it teaches:**
- GD&T symbols (parallelism, flatness, diameter) have specific geometric shapes that carry meaning. A circle with a cross (position symbol) means "target." A right-angle symbol means "squareness."
- The "call-out box" with a leader line pointing to a diagram element is a perfect pattern for map markers and annotation.
- Chemical/element symbols (CO2, H2O) mixed with measurements create a scientific texture.
- Fragmented, non-linear layouts suggest a complex system being documented from multiple angles.

**Apply to dashboard:** Use call-out boxes with leader lines for map annotations (drone position, home, GOTO target). Use measurement scales with tick marks for altitude and speed tapes instead of plain numbers.

### 3.4 The North Face Basecamp — Geodesic Dome Coordinates

**File:** `micrographics-north-face-basecamp-geodesic-dome-coordinates.jpeg`

**What it teaches:**
- Listing GPS coordinates as part of the brand identity is powerful for a GCS. Every dashboard panel should feel "anchored" to a location.
- A wireframe overlay on a solid field (the geodesic dome) creates depth without gradients.
- The vertical sidebar with rotated text is a space-efficient pattern for secondary data.

**Apply to dashboard:** Embed current GPS coordinates in a subtle wireframe-framed box near the map. Use vertical sidebars for altitude tapes and status messages.

### 3.5 Chemical Lab Labels — Acid Grotesk Hazard Symbols

**File:** `micrographics-chemical-lab-labels-acid-grotesk-hazard-symbols.jpeg`

**What it teaches:**
- The NFPA 704 "fire diamond" is the most information-dense icon in industrial design. Four colored quadrants communicate health, flammability, reactivity, and special hazards — all in a 40×40px square.
- GHS hazard icons (skull, flame, corrosive) are universally understood. They can be adapted for drone system states.
- Barcode matrices add authentic "product" texture. A scannable-looking data matrix near the build tag reinforces the tool aesthetic.
- The pharmaceutical dosage table (Hyaluronic Acid, Lidocaine) shows how to display structured multi-field data densely.

**Apply to dashboard:** Create a "Drone Hazard Diamond" — a 4-quadrant indicator showing Battery (health), Link (flammability/propagation), GPS (reactivity/stability), and Mode (special). Use GHS-style icons adapted for drone subsystems.

### 3.6 Occult/Cyberpunk Symbolic Collage

**File:** `micrographics-occult-cyberpunk-symbolic-collage.jpeg`

**What it teaches:**
- **Not** directly applicable to a GCS. This image shows what micrographics is NOT — it's a collage, not a system.
- The halftone eye and sacred geometry elements are interesting texture but inappropriate for operational software.
- What we can learn: The "sticker sheet" layout (many elements scattered on one canvas) is how you might design a modular graphic system.

**Apply to dashboard:** Use the "sticker sheet" concept when designing the SVG icon library — each icon should work as a standalone sticker file.

### 3.7 VOIDS — Mathematical Physics Poster (Yellow-on-Black)

**File:** `micrographics-voids-mathematical-physics-poster-yellow-on-black.jpeg`

**What it teaches:**
- Yellow-on-black is the caution palette. It's distinct from both the orange (warning) and red (danger) families.
- Wireframe 3D grids (isometric projections) are a stunning way to represent drone position in 3D space.
- The continuous scale with numerical tick marks (0, 5, 10) can be adapted for altitude/speed tapes.
- Mathematical notation (empty set symbol ∅, division expressions) as decorative elements adds intellectual texture.

**Apply to dashboard:** Yellow accent for the altitude tape — let it glow yellow during normal operation, shift to orange at caution threshold. Use isometric wireframe grids in the Vertical Situation Display (VSD).

### 3.8 Fashion Brand Tech Spec Sheets — AI Metadata Grid

**File:** `micrographics-fashion-brand-tech-spec-sheets-ai-metadata-grid.jpeg`

**What it teaches:**
- The 2×4 grid of stand-alone spec cards is a strong pattern for presenting telemetry on a dashboard.
- Each card has: brand logo + technical metadata + icons + small text. This maps perfectly to a telemetry tile: vehicle type + value + sparkline + label.
- The "stamp" aesthetic (circular badges, serial numbers, dates) gives each data point a sense of being certified.

**Apply to dashboard:** Each telemetry tile becomes a "certified spec card" — with a tiny stamp/badge in the corner showing the freshness of the data (5 Hz, 500ms ago, etc.).

### 3.9 The North Face Basecamp — Chamonix Coordinates

**File:** `micrographics-north-face-basecamp-chamonix-coordinates.jpeg`

**What it teaches:**
- Extreme minimalism: one logo, one wireframe element, one text block. The vast black space makes the small details monumental.
- The flush-left text block with coordinate precision (45° 55' 25.3092'' N) communicates extreme accuracy.

**Apply to dashboard:** Use a similar minimal block for the GPS coordinate display pad — flush-left, tiny, precise. The vast space around it communicates "this is the only number that matters right now."

### 3.10 MODU Studio — 20 Industrial Vector Elements Catalog

**File:** `micrographics-modu-studio-20-industrial-vector-elements-catalog.avif`

**What it teaches:**
- Modular vector elements (crosshairs, orbital rings, measurement scales, progress arcs, dot-matrix text, data badges) can be combined to build any dashboard component.
- The "catalog" layout — preview + spec sidebar — is the ideal format for a design system documentation page.
- Dot-matrix fonts for section headers create a retro-technical feel that pairs well with modern sans-serif labels.
- The sidebar format (title → package contents → resolution → dimensions) maps to: Panel Name → Data Items → Update Rate → Size.

**Apply to dashboard:** This is the most directly applicable reference. Every dashboard component should be built from MODU-style modular elements. The GCS becomes a "MODU-MICRO GRAPHIC" — a collection of micro-graphic components arranged on a dark canvas.

---

## 4. Brand & Identity — The Micrographic Lens

### Brand Name
**Skylink** — stays the same. But presented differently.

Instead of a glossy logo mark, the Skylink brand is presented as a **certification plate**:

```
┌──────────────────────────────────────┐
│  ╱╲  SKYLINK                        │
│  ╲╱  GROUND CONTROL STATION          │
│                                      │
│  TYPE: GCS-1    SERIES: SKYLINK      │
│  FW: v12.0      FS: v15.0            │
│  PROTOCOL: SKYLINK-v1                │
│  CERT: ARDUPILOT-COMPATIBLE          │
└──────────────────────────────────────┘
```

### Tagline (micrographic variants)
- *"SKYLINK GCS — TECHNICAL SPECIFICATION"* — reads like a document title
- *"SYSTEM: GROUND CONTROL | VEHICLE: ANY"* — reads like a spec header
- *"COMMAND THE SKY | CERTIFIED v1.0"* — aspirational meets technical

### Brand Personality — Micrographic

| Attribute | Micrographic Manifestation |
|-----------|---------------------------|
| **Authoritative** | Every element looks like it was approved by a regulatory body. |
| **Precise** | Hairline strokes, perfectly aligned grids, no pixel out of place. |
| **Calm** | Dense but organized. The density itself communicates control. |
| **Engineered** | Looks designed by a systems engineer, not a visual artist. |
| **Certified** | Every panel feels like it passed inspection. Stamps and badges everywhere. |

### The Logo — Spec-Plate Style

Instead of a single logo, Skylink GCS uses a **family of brand marks** that appear in different contexts:

1. **Full certification plate** (header, settings panel) — rectangular plate with all metadata
2. **Monogram in diamond** (favicon, status bar) — "SL" in a diamond frame, like a certification stamp
3. **Minimal text** (loading screens, connection overlay) — "SKYLINK GCS" in dot-matrix font

---

## 5. The Complete Layout Blueprint

The same 3-column Bloomberg foundation from Option A, but expressed through a micrographic lens.

### 5.1 Overall Grid

```
┌──────────────────────────────────────────────────────────────────┐
│  CERTIFICATION BAR  (formerly Status Bar)   48px                 │
│  [SL in diamond] [FW12·FS15 ✓] [PROTOCOL v1] [SYSTIME: UTC]    │
├───────────────────┬────────────────────────┬────────────────────┤
│                   │                        │                    │
│  LEFT WING        │   MAIN STAGE            │  RIGHT WING       │
│  FLIGHT DATA      │   SITUATIONAL           │  SYSTEMS STATUS   │
│  280px            │   AWARENESS             │  340px            │
│                   │   (flex)                │                    │
│  ┌─────────────┐  │                        │  ┌─────────────┐  │
│  │ ATTITUDE    │  │  ┌──────────────────┐   │  │ PREFLIGHT   │  │
│  │ INDICATOR   │  │  │  MAP (Leaflet)   │   │  │ CHECKLIST   │  │
│  │ [horizon]   │  │  │  dark tiles      │   │  │ [5 items]   │  │
│  │ [pitch      │  │  │                  │   │  └─────────────┘  │
│  │  ladder]    │  │  │  Drone marker    │   │                    │
│  └─────────────┘  │  │  Trail           │   │  ┌─────────────┐  │
│                   │  │  Geofence        │   │  │ TELEMETRY   │  │
│  ┌─────────────┐  │  │  GOTO pin        │   │  │ SPEC CARDS  │  │
│  │ ALTITUDE    │  │  │                  │   │  │ [6 tiles]   │  │
│  │ TAPE        │  │  │  MAP OVERLAYS:   │   │  └─────────────┘  │
│  │ [scale +    │  │  │  ● Mode badge    │   │                    │
│  │  tick marks]│  │  │  ● Arm state     │   │  ┌─────────────┐  │
│  │ [====●=]    │  │  │  ● Hazard dia.   │   │  │ SPARKLINE   │  │
│  └─────────────┘  │  │  ● Call-out box  │   │  │ MATRIX      │  │
│                   │  │                  │   │  │ [4 trends]  │  │
│  ┌─────────────┐  │  │  MAP CONTROLS:   │   │  └─────────────┘  │
│  │ SPEED TAPE  │  │  │  ⊞ zoom          │   │                    │
│  │ [====●░░]   │  │  │  ⊟ follow        │   │  ┌─────────────┐  │
│  └─────────────┘  │  │  ⊞ center        │   │  │ STATUS      │  │
│                   │  │                  │   │  │ MESSAGES    │  │
│  ┌─────────────┐  │  │                  │   │  │ [scroll]    │  │
│  │ HEADING     │  │  │                  │   │  └─────────────┘  │
│  │ COMPASS     │  │  │                  │   │                    │
│  │ ◄--N--►     │  │  │                  │   │                    │
│  └─────────────┘  │  │                  │   │                    │
│                   │  │                  │   │                    │
│  ┌─────────────┐  │  │                  │   │                    │
│  │ VSD         │  │  │                  │   │                    │
│  │ [3D side    │  │  │                  │   │                    │
│  │  view]      │  │  │                  │   │                    │
│  └─────────────┘  │  │                  │   │                    │
├───────────────────┴────────────────────────┴────────────────────┤
│  COMMAND BAR  —  SPECIFICATION PLATE STYLE          ~120px      │
│  ┌──────┐ ┌──────┐ ┌──────────┐ ┌──────┐ ┌──────┐ ┌──────────┐ │
│  │MODE  │  │ARM   │  │TAKEOFF  │  │LAND  │  │RTL   │  │EMERG.│ │
│  │GUIDED│  │      │  │5m       │  │      │  │      │  │STOP   │ │
│  └──────┘ └──────┘ └──────────┘ └──────┘ └──────┘ └──────────┘ │
│  ┌──────── MOVE PAD ────────┐  ┌──── YAW ────┐  ┌─ DIST ──┐  │
│  │   [▲ FWD] [▼ BACK]       │  │ [◄ -90°]    │  │ 1m 3m   │  │
│  │   [◄ LEFT] [► RIGHT]     │  │ [► +90°]    │  │ 5m 10m  │  │
│  └───────────────────────────┘  └─────────────┘  └─────────┘  │
├──────────────────────────────────────────────────────────────────┤
│  STATUS STRIP  —  MICROGRAPHIC FOOTER              32px         │
│  UPTIME: 01:23:45 | 5 Hz | -65 dBm | 12 sats | 192.168.1.100  │
└──────────────────────────────────────────────────────────────────┘
```

### 5.2 What Changed from Option A

The foundation is identical — 3 columns, no tabs, Bloomberg-inspired density. The differences are in **visual execution**:
- Panels have **hairline borders** with corner brackets instead of rounded rectangles
- Headers use **dot-matrix font** or capped sans-serif
- Data values are framed in **geometric bounds** (diamonds for critical, squares for nominal)
- The status bar is now a **certification bar** with spec-plate styling
- Every panel has a **tiny stamp/badge** showing data freshness
- The connection overlay uses a **sandwich layout** (top + bottom text blocks)

---

## 6. Visual Language

### 6.1 The Micrographic Component System

Every UI element follows a strict visual grammar derived from industrial documentation:

**Geometric framing:**

| Shape | Meaning | Used For |
|-------|---------|----------|
| **Diamond** | Hazard, critical attention | Emergency controls, link loss indicator, critical alerts |
| **Circle** | Certification, approval, OK | Health indicators, mode badges, data freshness stamps |
| **Square** | Data, specification, measurement | Telemetry tiles, spec cards, data fields |
| **Stadium/Pill** | Feature highlight, action | Buttons, link chips, command labels |
| **Hexagon** | System, network, connection | Link status, WiFi/RSSI, network indicators |

**Borders and frames:**

```
// Standard data panel
┌──────────────────────────┐
│  LABEL                    │  ← 1px border, corners not rounded
│  ● 23.4 m                │     Corner brackets optional (see below)
└──────────────────────────┘

// Critical data panel  
┌──────────────────────────┐
╲  WARNING                  ╱  ← Corner brackets: top-left, top-right
 ╲  ⚠ LOW BATTERY         ╱     Only on panels that need attention
  └──────────────────────────┘
```

**Corner bracket style:**
```
┌──┐  ┌──────┐  ┌──────┐  ┌──────┐
│  │  │      │  ╲      ╱  │  ░░░░│  
│  │  │      │   ╲    ╱   │  ░░░░│
└──┘  └──────┘    └──┘    └──────┘
normal  active    critical  loading/sleep
```

### 6.2 Panel Design

Every primary panel has this structure:

```
  ┌────────────────────────────────────┐
  │ ○ PANEL LABEL          [stamp]     │  ← Header bar: dot-matrix or sans-serif caps
  │────────────────────────────────────│  ← Hairline divider (0.5px)
  │                                    │
  │  Content area                      │  ← Data, controls, visualizations
  │                                    │
  │────────────────────────────────────│  ← Hairline divider (optional footer)
  │  Status: ACTIVE · 5 Hz             │  ← Micro-detail footer
  └────────────────────────────────────┘
```

Key rules:
- No rounded corners on data panels (corners imply softness; a GCS is not soft)
- Only the map container can have rounded corners (it's a window into the real world)
- Panel spacing: exactly 8px between panels (the micrographic grid unit)
- No shadows — micrographic design lives on a flat plane

### 6.3 Buttons (Micrographic Style)

```
// Primary action (ARM, TAKEOFF, GO)
┌──────────────────────────────┐
│  ⬡  ARM DRONE                │  ← hexagon-framed icon + text
│     ONLY IN GUIDED MODE      │  ← condition text
└──────────────────────────────┘

// Hazard action (DISARM, RTL, STOP)
┌──────────────────────────────┐
│  ◆  RTL                      │  ← diamond-framed icon
│     RETURN TO HOME           │
└──────────────────────────────┘

// Mode selector
┌──────────────────────────────┐
│  ■  GUIDED        ▼          │  ← square-framed, dropdown caret
│     MODE                      │
└──────────────────────────────┘

// Move direction (D-pad)
  ┌──────────┐
  │  ▲       │  ← hairline triangle, no fill
  │  FWD     │
  └──────────┘
```

### 6.4 The Certification Stamp

Every data panel includes a data freshness stamp in the top-right corner:

```
┌──────────────────────────────┐
│ ALTITUDE            [5 Hz]  │  ← green = live
│ 23.4 m                      │
└──────────────────────────────┘

┌──────────────────────────────┐
│ BATTERY           [2s ago]  │  ← amber = stale but recent
│ 87%                         │
└──────────────────────────────┘

┌──────────────────────────────┐
│ LINK               [OFF]    │  ← red = no data
│ ---                         │
└──────────────────────────────┘
```

The stamp format: `[frequency | time-ago | state]` in 10px font, rotated -90° in a sidebar OR placed inline. The MODU STUDIO reference shows this — a tiny metadata badge in the corner of every element.

---

## 7. Typography System

### 7.1 Font Stack

Micrographic design demands specific font choices:

| Role | Font | Rationale |
|------|------|-----------|
| **Primary data** | JetBrains Mono Bold | Monospace for numeric alignment. Already used in prototype. |
| **Secondary data** | JetBrains Mono SemiBold | Consistent family, different weight. |
| **Labels + UI text** | Inter Medium | Clean sans-serif for body. Readable at 10px. |
| **Section headers** | "Dot Matrix" or Press Start 2P | Retro-technical feel. Use only at 10-12px for authentic "dot matrix printer" look. |
| **Certification stamps** | JetBrains Mono Regular | Same monospace, smaller size. |
| **Micro-detail text** | Inter Regular at 9-10px | The "fine print." Must be readable but obviously tiny. |

### 7.2 Type Sizes

| Size | Usage | Context |
|------|-------|---------|
| 36px | Primary telemetry (altitude) | Bold, monospace, the hero number |
| 28px | Secondary telemetry (speed, battery) | Bold, monospace |
| 20px | Tertiary data | SemiBold, monospace |
| 14px | Button labels, section content | Medium, sans-serif |
| 12px | Labels, panel headers (capped) | Medium, sans-serif, 1px letter-spacing |
| 11px | Link chips, stamp badges | Medium, sans-serif |
| 10px | Certification details, micro-detail | Regular, sans-serif |
| 9px | Tiny stamps, version info (optional) | Regular, monospace |

### 7.3 Typographic Micro-Details

Micrographic typography uses **decorative technical formatting**:

```
ALTITUDE          ← uppercase label, 1.5px letter-spacing
   23.4 m         ← value, right-aligned to decimal point
  ───────
  234.0  ← home   ← reference line
  ───────
  RATE: +2.1 m/s  ← micro-detail below

BATTERY
   87%            ← big value
   12.4 V         ← secondary micro-detail
  [████████░░]    ← progress bar as measurement line
  RATE: -0.3V/min ← trend micro-detail
  42min LEFT      ← projected micro-detail
```

The density of small supporting details around the primary value is what creates the micrographic texture.

---

## 8. Color System

### 8.1 Micrographic Palette

The micrographic palette is derived from industrial warning labels, aviation instruments, and electronic component color coding.

```
// Backgrounds
--bg-void:         #050505  — absolute black, the spec plate background
--bg-panel:        #080A0E  — barely lighter, for panel surfaces
--bg-elevated:     #0C1016  — hovered/active surfaces
--bg-input:        #000000  — input fields, text areas

// Lines and borders
--border-hairline:  #1A1F2E  — 0.5-1px, very subtle
--border-active:    #2D3A5C  — focused, selected
--border-readout:   #1E2D4A  — measurement scale borders

// Text
--text-primary:     #E8EDF5  — main values
--text-secondary:   #8B98AA  — labels, units
--text-muted:       #4A5A70  — micro-detail, version stamps
--text-cert:        #6B7C93  — certification text

// Functional accents (from industrial color coding)
--accent-cyan:     #00D4FF  — certification, approved, telemetry (like ECE/EASA certification green-blue)
--accent-green:    #34D399  — safe, armed, nominal
--accent-amber:     #F59E0B  — warning, caution (safety orange/amber)
--accent-red:       #EF4444  — error, critical, danger
--accent-yellow:    #EAB308  — caution, simulation, non-critical warning
--accent-purple:    #8B5CF6  — command acknowledgment, mode change

// Industrial accent colors for geometric elements
--diamond-health:  #EF4444  — red diamond for health hazard
--diamond-fire:    #EAB308  — yellow diamond for flammability
--diamond-react:   #34D399  — green diamond for reactivity (or stability)
--diamond-special: #00D4FF  — cyan diamond for special/notice

--scale-tick:      #2D354A  — measurement scale tick marks
--scale-tick-major:#3D4A6A  — major tick marks (every 5th or 10th)
--glow-cyan:       rgba(0,212,255,0.15) — cyan glow for active panels
--glow-red:        rgba(239,68,68,0.15) — red glow for alarm panels
```

### 8.2 Color Application Rules

| Element | Color Rule |
|---------|------------|
| Panel backgrounds | `--bg-panel` — never pure white |
| Hairline borders | `--border-hairline` — 1px |
| Panel header text | `--text-secondary` — uppercase, letter-spaced |
| Primary value text | `--text-primary` — large, bold, monospace |
| Unit text | `--text-muted` — 30% smaller than value |
| Fresh data stamp | `--accent-cyan` — with dot indicator |
| Stale data stamp | `--accent-amber` — with warning dot |
| No data stamp | `--accent-red` — with cross indicator |
| Warning backgrounds | 15% opacity of the warning color, no text on warning bg |
| Button primary | `--accent-cyan` border, no fill, inverted on hover |
| Button danger | `--accent-red` border, `--accent-red` fill on hover |

### 8.3 The "Safety Orange" Option

One of our reference images used safety orange on black instead of cyan. If Option B needs a warmer accent:

```
--accent-orange:   #FF6B35  — safety orange, as on the industrial label
--accent-warm:     #FF8C42  — slightly lighter for text
--glow-orange:    rgba(255,107,53,0.15)
```

This replaces the cyan accent entirely. The dashboard becomes warm-amber instead of cool-cyan. Both options work — cyan feels more "aviation glass cockpit," orange feels more "industrial safety equipment."

---

## 9. Iconography & Symbols

### 9.1 Icon Style

All icons in the micrographic system follow strict rules:

- **Hairline stroke:** 1.5px on a 24×24 or 20×20 canvas
- **No fills:** Outlined only (filled only for status indicators)
- **Geometric construction:** Every curve is an arc, every corner is exact
- **Consistent weight:** Same stroke width across all icons
- **Semantic framing:** Icons sit inside geometric frames (circle, diamond, square, hexagon) based on their meaning

### 9.2 Drone System Hazard Diamond

Adapted from the NFPA 704 standard (from `micrographics-chemical-lab-labels-acid-grotesk-hazard-symbols.jpeg`):

```
      ┌─────────┐
      │  BAT    │  ← Top: Battery (red quad) — current charge level
      │  87%    │
  ┌───┼─────────┼───┐
  │   │         │   │
  │LNK│   MODE  │GPS│  ← Left: Link, Right: GPS, Bottom: Mode
  │-65│  GUIDED │12 │
  │dBm│         │sat│
  └───┼─────────┼───┘
      │  SYS    │  ← Center/Bevel: System overall
      │  OK     │
      └─────────┘
```

This 5-zone diamond sits on the map overlay, giving the operator a split-second system health assessment. Each quadrant mirrors the NFPA color code.

### 9.3 GCS-Specific Icons (Micrographic Style)

| Icon | Description | Frame | Meaning |
|------|-------------|-------|---------|
| Drone | Simplified top-down drone silhouette | None (floating) | Vehicle position on map |
| Home | House symbol | Circle | Home position |
| Link | Two dots with connecting arc | Diamond | Signal/connection |
| GPS | Satellite dish with orbiting dots | Hexagon | GPS fix quality |
| Battery | Rectangle with fill level | Square | Battery remaining |
| Altitude | Vertical arrow with measurement line | Square | Altitude |
| Speed | Horizontal arrow with speed lines | Square | Airspeed/ground speed |
| Mode | Gear symbol | Circle | Flight mode indicator |
| Arm | Shield symbol | Diamond | Armed/disarmed state |
| Warning | Exclamation mark | Diamond (filled) | Alert/critical |
| ACK | Checkmark | Circle | Command accepted |
| Takeoff | Upward arrow | Square | Takeoff action |
| Land | Downward arrow | Square | Land action |
| RTL | Arrow curving back to dot | Diamond (outlined) | Return to launch |
| Compass | Rose with N indicator | None | Heading reference |
| Crosshair | Crosshair with center dot | Circle | GOTO target |
| Geofence | Circle with dashed line | None | Geofence indicator |
| Emergency | Lightning bolt | Diamond (filled red) | Emergency stop |

### 9.4 The "Sticker Sheet" Approach

Following the MODU STUDIO reference (`micrographics-modu-studio-20-industrial-vector-elements-catalog.avif`), all icons and micro-graphic elements should be designed as standalone assets that can be assembled like a sticker sheet:

```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│    ⬡ LINK    │  │    ◆ ARM     │  │    ■ ALT     │  │    ● SATS    │
│  ┌──────┐    │  │  ┌──────┐    │  │  ┌──────┐    │  │  ┌──────┐    │
│  │ ●─●  │    │  │  │ ⬛   │    │  │  │ ═══  │    │  │  │ ○○○  │    │
│  └──────┘    │  │  └──────┘    │  │  └──────┘    │  │  └──────┘    │
│ OK          │  │ ARMED        │  │ 23.4m        │  │ 12 sats      │
└──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘
```

---

## 10. Data Visualization

### 10.1 The Altitude Display (Micrographic)

```
  ┌──────────────────────┐
  │ ■ ALTITUDE  [5 Hz]   │
  │──────────────────────│
  │   23.4  m            │  ← Primary value, 36px JetBrains Mono Bold
  │   REL: 22.1m         │  ← Secondary micro-detail
  │──────────────────────│
  │  ┌─┬─┬─┬─┬─┬─┬─┐   │
  │  │ │ │ │●│ │ │ │   │  ← Measurement scale with moving indicator
  │  └─┴─┴─┴─┴─┴─┴─┘   │     Tick marks at 5m intervals
  │  0  5 10 15 20 25 30│
  │──────────────────────│
  │ RATE: +2.1 m/s  CLB  │  ← Rate indicator with arrow direction
  │ HOME: 1.3m AGL       │  ← Micro-detail footer
  │ ALT LIMIT: 50m MAX   │
  └──────────────────────┘
```

### 10.2 The Battery Display (Micrographic)

```
  ┌──────────────────────┐
  │ ◈ BATTERY  [5 Hz]    │  ← Diamond for hazard-level data
  │──────────────────────│
  │   87  %              │  ← Primary, 28px
  │   12.4 V             │  ← Micro-detail
  │──────────────────────│
  │  ▓▓▓▓▓▓▓▓░░░░       │  ← Measurement bar with tick overlay
  │  ▔▔▔▔▔▔▔▔▔▔▔▔       │
  │  0%        50%   100%│  ← Scale labels
  │──────────────────────│
  │ DISCHARGE: -0.3V/min │  ← Trend micro-detail
  │ EST: 42min @ cur.rate│  ← Projection
  └──────────────────────┘
```

### 10.3 The Attitude Indicator (Micrographic)

Instead of a rounded artificial horizon (Option A), Option B uses a **square-framed attitude instrument** with engineering markings:

```
  ┌──────────────────────────┐
  │ ○ ATTITUDE      [5 Hz]   │
  │──────────────────────────│
  │     ┌──────────────┐    │
  │  30 │ ╱╲╱╲╱╲╱╲╱╲│ -30   │  ← Roll scale at top
  │     │              │    │
  │  20 │  SKY (blue)  │ -20  │  ← Pitch ladder
  │     │              │    │
  │  10 │   ────●────  │ -10  │  ← Horizon line + drone symbol
  │     │              │    │
  │   0 │ GROUND (brn) │  0   │
  │     │              │    │
  │ -10 │              │  10  │
  │     └──────────────┘    │
  │──────────────────────────│
  │ ROLL: 2.3°  PITCH: -1.1°│
  │ YAW: 134°  HDG: 134°    │
  └──────────────────────────┘
```

The instrument frame is square with hairline markings — exactly like a real aircraft ADI (Attitude Director Indicator).

### 10.4 The Measurement Scale System

Every continuous variable gets a measurement scale, not just a number:

```
// Altitude scale (vertical, left wing)
   ┌──────┐
 30│      │
   │      │
 25│      │
   │  ●   │← moving indicator
 20│      │
   │      │
 15│      │
   │      │
 10│      │
   └──────┘

// Speed scale (horizontal compact)
   ┌─┬─┬─┬─┬─┬─┬─┐
   │ │ │●│ │ │ │ │
   └─┴─┴─┴─┴─┴─┴─┘
   0 1 2 3 4 5 6 m/s
```

### 10.5 The Sparkline Matrix

Instead of individual sparklines (Option A), Option B uses a **2×2 micrographic matrix**:

```
  ┌──────────────────────────┐
  │ ▣ TRENDS       [5 Hz]    │
  │──────────────────────────│
  │  ╱╲╱╲╱╲╱╲╱╲╱   ALT      │
  │  ╱╲╱╲╱╲╱╲╱╲╱  +23.4m   │
  │                          │
  │  ╲╱╲╱╲╱╲╱╲╱╲╱   SPD     │
  │  ╲╱╲╱╲╱╲╱╲╱╲╱  0.8m/s  │
  │                          │
  │  ╲╱╲╱╲╱╲╱╲╱╲╱   BAT     │
  │  ▔▔▔▔▔▔▔▔▔▔▔▔  12.4V   │
  │                          │
  │  ▔▔▔▔╱╲╱╲╱╲╱╲   LINK    │
  │  ▔▔▔▔▔▔▔▔▔▔▔▔  -65dBm  │
  └──────────────────────────┘
```

Each sparkline has a thin axis line and uses the same stroke weight as all other lines.

---

## 11. Interactions & Animation

### 11.1 Micrographic Animation Principles

Same as Option A: purposeful, brief, informative. But expressed differently:

- **No opacity transitions** — micrographic elements don't fade. They snap.
- **No scale transforms** — buttons don't shrink on press. They invert (white → black → white).
- **No glow as primary feedback** — use geometric changes instead.

### 11.2 Interaction Feedback (Micrographic Style)

| Action | Micrographic Feedback | Duration |
|--------|----------------------|----------|
| Button press | Inverse colors (fill swaps with stroke) | Instant |
| Command sent | Corner brackets animate: `┌→ ╱→ └` in sequence | 200ms |
| Command ACK | Small checkmark stamp appears in corner of button | 300ms |
| Command ERROR | Diamond frame pulses around the button | 800ms |
| Telemetry update | Value cell flashes with `--bg-elevated` background | 100ms |
| Connection lost | Panel headers shift from `○` to `◆` to `●` (circle to diamond progression) | 500ms |
| Value transition | Old value fades out, new value slides up from below (like a mechanical counter) | 150ms |
| Preflight item pass | Circle fills in clockwise: `○ → ◔ → ◑ → ◕ → ●` | 400ms |
| Emergency state | All non-critical panels reduce to 40% — only emergency panels stay full opacity | 300ms |

### 11.3 The Mechanical Counter Animation

Telemetry values animate like mechanical odometers when changing:

```
// Before:
   23.4 m

// During (value changes to 24.1):
   23.4 → 23.5 → 23.6 → ... → 24.0 → 24.1

// Each digit snaps independently
   23.4
    ↓
   24.1  ← digits roll independently

// Duration: 200ms for small changes, 400ms for large jumps
// Only shown for primary values (altitude, speed, battery)
```

This creates a satisfying mechanical precision that reinforces the "instrument panel" feel.

---

## 12. The Modular Graphic System

### 12.1 What It Is

Following the MODU STUDIO approach, the entire dashboard is built from a **library of modular micro-graphic elements**. Each element is a standalone SVG component that can be:

- Used standalone
- Composed with other elements
- Placed in any panel
- Exported for print or documentation

### 12.2 Element Catalog

| Element | Category | Dimensions | Used In |
|---------|----------|------------|---------|
| `micro-horizon` | Visual | 160×160 | Attitude indicator |
| `micro-scale` | Visual | variable×24 | Altitude/speed tapes |
| `micro-compass` | Visual | 280×24 | Heading strip |
| `micro-sparkline` | Visual | 100×40 | Trend display |
| `micro-hazard-diamond` | Icon | 80×80 | System health |
| `micro-cert-stamp` | Badge | 48×16 | Data freshness |
| `micro-pill-badge` | Badge | variable×20 | Mode/state display |
| `micro-callout` | Container | variable | Map annotations |
| `micro-spec-card` | Container | 140×100 | Telemetry tiles |
| `micro-dpad` | Control | 160×160 | Movement control |
| `micro-button-primary` | Control | variable×40 | ARM, TAKEOFF, GO |
| `micro-button-danger` | Control | variable×40 | RTL, STOP, DISARM |
| `micro-button-move` | Control | 56×56 | Direction arrows |
| `micro-progress-bar` | Visual | variable×8 | Battery, signal |
| `micro-waveform` | Visual | 120×30 | Link signal viz |
| `micro-crosshair` | Icon | 24×24 | Map marker |
| `micro-drone-icon` | Icon | 28×28 | Map marker |
| `micro-home-icon` | Icon | 24×24 | Home marker |

### 12.3 Composition Rules

Elements combine following strict rules:

```
// A telemetry tile is composed of:
micro-spec-card {
  header: micro-cert-stamp (top-right)
  icon: <category icon> (left side)
  value: <primary value> (center, large)
  scale: micro-scale (below value)
  micro-detail: <secondary info> (footer)
}

// The command bar is composed of:
micro-button-primary (ARM)
micro-button-danger (DISARM) 
micro-button-danger (RTL)
micro-dpad (movement)
micro-pill-badge (current mode)
```

Every element is a reusable SVG component with CSS custom property overrides for color, size, and stroke width.

---

## 13. Micrographic Dashboard Zones — Detailed

### 13.1 Certification Bar (Top)

```
 ┌─────────────────────────────────────────────────────────────┐
 │  ◆ SKYLINK              CERTIFICATION PLATE  SKYLINK-GCS-1 │
 │  GCS · v1 · P-2026      ╔═══╗  ╔═══╗  ╔═══╗  ╔═══╗       │
 │  FW: 12  FS: 15  OK      ║WS ║  ║MAV║  ║SIT║  ║GPS║       │
 │  PROTOCOL: SKYLINK/v1    ╚═══╝  ╚═══╝  ╚═══╝  ╚═══╝       │
 │                           ●OK  ●OK  ○CONN  ○3D            │
 │                          ───────────────────── 14:32:05Z │
 └─────────────────────────────────────────────────────────────┘
```

- Left: Skylink logo + certification details (type, version, firmware)
- Center: Link status as square-framed stamp indicators
- Right: UTC time in Z-suffix format (aviator-style)

### 13.2 Left Wing — Flight Data

```
 ┌────────────────────────┐
 │ ○ FLIGHT DATA          │  ← Panel header
 │────────────────────────│
 │ ╔═ ATTITUDE ════════╗ │
 │ ║  ┌──────────────┐ ║ │
 │ ║  │   SKY (30°)  │ ║ │  ← Square-framed ADI
 │ ║  │     ──●──    │ ║ │
 │ ║  │ GROUND (-10°)│ ║ │
 │ ║  └──────────────┘ ║ │
 │ ║ ROLL: 2.3°        ║ │
 │ ║ PITCH: -1.1°      ║ │
 │ ╚═══════════════════╝ │
 │────────────────────────│
 │ ╔═ ALTITUDE ════════╗ │
 │ ║   23.4 m          ║ │  ← Large value + scale
 │ ║   ┌─┬─┬─┬─┬─┬─┐  ║ │
 │ ║   │ │ │●│ │ │ │  ║ │  ← Moving scale indicator
 │ ║   └─┴─┴─┴─┴─┴─┘  ║ │
 │ ║   0 10 20 30 40 50║ │
 │ ║   +2.1m/s RATE    ║ │
 │ ╚═══════════════════╝ │
 │────────────────────────│
 │ ╔═ SPEED ═══════════╗ │
 │ ║   0.8 m/s         ║ │
 │ ╚═══════════════════╝ │
 │────────────────────────│
 │ ◄───── N ────► 134°  │  ← Compass strip
 │────────────────────────│
 │ VSD: [isometric 3D    │  ← Vertical Situation Display
 │        wireframe       │     using yellow wireframe 
 │        of drone +      │     (from VOIDS reference)
 │        ground plane]  │
 └────────────────────────┘
```

### 13.3 Main Stage — Map

The map uses the same Leaflet foundation as Option A, but with micrographic overlay elements:

**Custom tile style:** Pure monochrome dark tiles — no color anywhere except the drone trail. Land: `#0D1117`, Water: `#050505`, Roads: hairline `#1A1F2E`, Labels: none or dot-matrix.

**Map overlays (micrographic style):**
- **Drone marker:** A drone silhouette inside a circle frame, with four crosshair lines extending to the edges of the frame
- **Home marker:** "H" inside a square frame with corner brackets
- **Trail:** Hairline connecting dots — no gradient, just a pure 1.5px line
- **Geofence:** Dashed circle with tick marks at cardinal directions (N, S, E, W stations)
- **GOTO target:** A crosshair with distance ring, measurements shown in call-out box
- **Hazard diamond overlay:** Top-right, showing system health (from section 9.2)

**Map annotation call-out box (from engineering blueprint reference):**

```
          ┌──────────────────────────────┐
          │ GOTO TARGET                  │
          │ ─────────────────────────── │
          │ LAT: 28.6139°N              │
          │ LON: 77.2090°E              │
          │ DIST: 45.2m                 │
          │ ALT: 10.0m                  │
          │ STATUS: PENDING             │
          └──────────────────────────────┘
                ↑
                │ leader line
                ● (target point)
```

### 13.4 Right Wing — Systems Status

```
 ┌──────────────────────────┐
 │ ▣ SYSTEMS STATUS         │
 │──────────────────────────│
 │ ╔═ PREFLIGHT ═════════╗ │
 │ ║ ● WiFi...........OK ║ │  ← Each item: geometry + label + state
 │ ║ ◆ GPS............OK ║ │
 │ ║ ■ MAVLink........OK ║ │  ← Different frames for different checks
 │ ║ ▲ Battery........OK ║ │
 │ ║ ⬡ Disarmed.......OK ║ │
 │ ╚═════════════════════╝ │
 │──────────────────────────│
 │ ╔═ TELEMETRY SPEC ═══╗ │
 │ ║ ┌────────┬────────┐║ │
 │ ║ │ ALT    │ SPD    │║ │  ← 2-column spec card grid
 │ ║ │  23.4m │ 0.8m/s │║ │
 │ ║ │ [5Hz]  │ [5Hz]  │║ │
 │ ║ ├────────┼────────┤║ │
 │ ║ │ BAT    │ SATS   │║ │
 │ ║ │  87%   │  12    │║ │
 │ ║ │ [5Hz]  │ [5Hz]  │║ │
 │ ║ ├────────┼────────┤║ │
 │ ║ │ HDG    │ DIST   │║ │
 │ ║ │ 134°   │ 1.3m   │║ │
 │ ║ │ [5Hz]  │ [*]    │║ │
 │ ║ └────────┴────────┘║ │
 │ ╚═════════════════════╝ │
 │──────────────────────────│
 │ ╔═ TRENDS (4) ════════╗ │
 │ ║ ╱╲╱╲╱╲╱ ╱╲╱╲╱╲╱╲ ║ │  ← 2×2 sparkline matrix
 │ ║ ╱╲╱╲╱╲╱ ╱╲╱╲╱╲╱╲ ║ │
 │ ║ ALT      SPD       ║ │
 │ ║ ╱╲╱╲╱╲╱ ▔▔▔▔▔▔▔ ║ │
 │ ║ BAT      LINK      ║ │
 │ ╚═════════════════════╝ │
 │──────────────────────────│
 │ ╔═ SYSTEM LOG ════════╗ │
 │ ║ 14:32:05 ACK: OK   ║ │  ← 4 most recent messages
 │ ║ 14:32:04 FC: ARM   ║ │
 │ ║ 14:32:03 SYS: RDY  ║ │
 │ ║ 14:32:02 FC: INIT  ║ │
 │ ╚═════════════════════╝ │
 └──────────────────────────┘
```

### 13.5 Command Bar (Bottom Dock)

The command bar uses spec-plate styling with individual action "stamps":

```
 ┌─────────────────────────────────────────────────────────────────┐
 │ FLIGHT COMMANDS                                                 │
 │ ─────────────────────────────────────────────────────────────── │
 │ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────────┐ │
 │ │MODE  │ │ ◆ARM │ │TAKEOFF│ │ LAND │ │ ◆RTL │ │ ⚡ EMERGENCY │ │
 │ │GUIDED│ │      │ │  5m   │ │      │ │      │ │    STOP      │ │
 │ │  ▼   │ │      │ │  ▼    │ │      │ │      │ │              │ │
 │ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────────────┘ │
 │ ─────────────────────────────────────────────────────────────── │
 │ PRECISION MOVEMENT                                              │
 │ ┌────┐                                                         │
 │ │ ▲  │        Move distance: [●1m] [3m] [5m] [10m] [__ ╲]    │
 │ │FWD │                                                        │
 │ ├────┤        Yaw: [◄ 90°] [► 90°]                            │
 │ │◄ ► │                                                        │
 │ │L  R│        Active: 1m  |  WASD: move  |  Q/E: yaw          │
 │ ├────┤                                                        │
 │ │ ▼  │                                                        │
 │ │BAK │        Condition: Must be GUIDED + armed + >2m alt      │
 │ └────┘                                                        │
 └─────────────────────────────────────────────────────────────────┘
```

### 13.6 Status Strip (Bottom)

```
 ┌─────────────────────────────────────────────────────────────┐
 │ UPTIME: 01:23:45  │  5Hz  │  RSSI: -65dBm  │  12 SATS      │
 │ FW: 12 // FS: 15  │  IP: 192.168.1.100  │  BUILD: OK       │
 └─────────────────────────────────────────────────────────────┘
```

A pure micro-detail strip. Each item separated by a hairline `│` pipe. No visual hierarchy — all data is equal in the footer.

---

## 14. Mobile & Responsive

### 14.1 Strategy

Same breakpoints as Option A (full → laptop → tablet → phone). The micrographic style actually **scales better** to small screens because:

- Hairline strokes are thin even at 1x
- Monochrome palette doesn't lose impact on small screens
- The sticker-sheet layout naturally collapses into a scrollable feed
- Dot-matrix headers remain readable at small sizes

### 14.2 Mobile Adaptations

On mobile (<768px), micrographics adapts differently from Option A:

- The certification bar collapses to: `◆SK [FW12] [5Hz] [14:32Z]`
- Left wing becomes a horizontal scrollable instrument strip
- Map fills 40% viewport
- Right wing spec cards collapse into a 1-column scrollable list
- Command bar becomes a fixed bottom sheet with icon-only buttons
- The hazard diamond becomes a single-row health indicator

---

## 15. Technical Constraints

Same as Option A: ESP32 LittleFS (~1.5MB total, <500KB target for dashboard). All the same performance targets apply.

**Additional micrographic considerations:**
- All micro-graphic elements are SVG — ensure SVG optimization (SVGO) removes unused viewbox data
- Dot-matrix fonts must be subsetted to ASCII only (no CJK, no extended Latin)
- Hairline strokes (0.5px) render differently on Windows vs Mac — use `vector-effect="non-scaling-stroke"` on all SVG elements
- Measurement scales should be rendered as CSS grid with pseudo-elements, not image assets
- The hazard diamond and certification stamps can be pure CSS — no images needed
- Corner brackets are CSS `::before` / `::after` with `border` — zero overhead

---

## 16. Differences from Option A

| Aspect | Option A (Bloomberg Terminal) | Option B (Micrographic) |
|--------|-------------------------------|------------------------|
| **Primary inspiration** | Bloomberg Terminal, NASA, trading dashboards | Industrial labels, engineering blueprints, NFPA/GHS |
| **Visual tone** | Cool, futuristic, glass-cockpit | Warm-amber or monochrome, spec-plate, industrial |
| **Panel shape** | Rounded corners (12px), subtle shadows | Square corners, corner brackets, no shadows |
| **Borders** | Colored borders, `--border` variable | Hairline (0.5px), color depends on panel type |
| **Typography** | JetBrains Mono + Inter | Same + dot-matrix accent font |
| **Icon style** | Feather-style outlined icons | Hairline geometric icons in semantic frames |
| **Data presentation** | Clean tiles with labels + values | Spec cards with measurement scales + stamps |
| **Feedback method** | Color changes, opacity, glow | Geometric changes, inversion, odometer roll |
| **Key visual trope** | "Glass cockpit" HUD | "Certification plate" / "industrial label" |
| **Animation style** | Smooth CSS transitions, opacity fades | Snap transitions, mechanical counter, bracket morph |
| **Color accent** | Cyan (`#00D4FF`) as primary | Orange (`#FF6B35`) or Yellow (`#F59E0B`) as primary |
| **Map tiles** | CartoDB Dark Matter | Custom pure monochrome dark tiles |
| **Altitude display** | Vertical tape with color zones | Vertical tape with measurement scale + mechanical counter |
| **Battery display** | Bar + percentage | Hazard diamond quadrant + bar + micro-detail |
| **Brand execution** | Logo mark + tagline | Certification plate + monogram-in-diamond |
| **Status bar** | Minimal, functional | Certification-style metadata strip |
| **Loading/offline** | Fade overlay, pulse dot | Sandwich layout (top/bottom text), bracket morph |
| **Mobile feel** | Simplified but recognizable | Sticker-sheet collapse, still recognizable |
| **Design complexity** | Lower — cleaner, more minimal | Higher — more visual detail per element |

---

## 17. Reference Image Catalog

The following images in `ref-images/` were used to build this design language:

| # | File | Source | Key Takeaway |
|---|------|--------|-------------|
| 1 | `micrographics-industrial-safety-label-orange-on-black.jpeg` | Zachary Winterton (LinkedIn) | Orange-on-black safety palette, ECE regulatory stamp format, "HANDCRAFTED" pill badge |
| 2 | `micrographics-perplexity-labs-brutalist-brand-identity.jpeg` | Perplexity Labs branding | Monochrome brutalist brand, sandwich layout, hairline icons |
| 3 | `micrographics-technical-engineering-schematic-blueprint.jpeg` | Unknown | GD&T symbols, call-out boxes with leader lines, measurement scales |
| 4 | `micrographics-north-face-basecamp-geodesic-dome-coordinates.jpeg` | The North Face (Instagram) | Wireframe geodesic dome, coordinate-based branding, vertical sidebar |
| 5 | `micrographics-chemical-lab-labels-acid-grotesk-hazard-symbols.jpeg` | Acid Grotesk / chemical labels | NFPA 704 fire diamond, GHS hazard icons, pharmaceutical micro-detail |
| 6 | `micrographics-occult-cyberpunk-symbolic-collage.jpeg` | Unknown | Counter-example — what micrographics is NOT |
| 7 | `micrographics-voids-mathematical-physics-poster-yellow-on-black.jpeg` | Jimbo Barbu for Andreu Serra | Yellow-on-black caution palette, isometric wireframe grids, mathematical notation |
| 8 | `micrographics-fashion-brand-tech-spec-sheets-ai-metadata-grid.jpeg` | Fashion/AI branding (Instagram) | 2×4 spec card grid, certification stamp aesthetic, AI metadata badges |
| 9 | `micrographics-north-face-basecamp-chamonix-coordinates.jpeg` | The North Face (Instagram) | Extreme minimalism with micro-detail, coordinate precision |
| 10 | `micrographics-modu-studio-20-industrial-vector-elements-catalog.avif` | MODU STUDIO | Modular vector element system, catalog layout, dot-matrix headers, sidebar spec format |

---

*End of Option B — The Micrographic Dashboard. Compare with Option A and choose the visual direction that best serves the operator.*

*"Every detail has been tested. Every pixel is certified."*