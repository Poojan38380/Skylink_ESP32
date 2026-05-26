# improve-codebase-architecture — Reference

**Source:** https://github.com/mattpocock/skills/tree/main/skills/engineering/improve-codebase-architecture  
**Author:** Matt Pocock  
**Saved:** May 26, 2026 — for Skylink BTP project reference

This is a verbatim reference of Matt Pocock's architecture improvement skill. It defines a disciplined process for surfacing architectural friction and deepening shallow modules.

---

## Overview

Surface architectural friction and propose **deepening opportunities** — refactors that turn shallow modules into deep ones. The aim is testability and AI-navigability.

### Glossary (from LANGUAGE.md)

Use these terms exactly in every suggestion. Consistent language is the point.

| Term | Definition | Avoid |
|------|------------|-------|
| **Module** | Anything with an interface and an implementation (function, class, package, slice) | unit, component, service |
| **Interface** | Everything a caller must know to use the module: types, invariants, error modes, ordering, config. Not just the type signature. | API, signature |
| **Implementation** | What's inside a module — its body of code | — |
| **Depth** | Leverage at the interface — a lot of behaviour behind a small interface. **Deep** = high leverage. **Shallow** = interface nearly as complex as the implementation. | — |
| **Seam** | Where an interface lives; a place behaviour can be altered without editing in place. | boundary |
| **Adapter** | A concrete thing satisfying an interface at a seam | — |
| **Leverage** | What callers get from depth | — |
| **Locality** | What maintainers get from depth: change, bugs, knowledge concentrated in one place | — |

### Key Principles

1. **Deletion test**: Imagine deleting the module. If complexity vanishes, it was a pass-through. If complexity reappears across N callers, it was earning its keep.
2. **The interface is the test surface.** Callers and tests cross the same seam.
3. **One adapter = hypothetical seam. Two adapters = real seam.** Don't introduce a seam unless something actually varies across it.
4. **Depth is a property of the interface, not the implementation.** A deep module can be internally composed of small, swappable parts.

### Rejected Framings

- **Depth as ratio of implementation-lines to interface-lines** — rewards padding the implementation. Use depth-as-leverage instead.
- **"Interface" as the TypeScript `interface` keyword** — too narrow.
- **"Boundary"** — overloaded with DDD's bounded context. Say **seam** or **interface**.

---

## Process

### Phase 1: Explore

1. Read the project's domain glossary (`CONTEXT.md`) and any ADRs in the area you're touching.
2. Walk the codebase organically. Note where you experience friction:
   - Where does understanding one concept require bouncing between many small modules?
   - Where are modules shallow?
   - Where have pure functions been extracted "just for testability" but real bugs hide in how they're called (no locality)?
   - Where do tightly-coupled modules leak across their seams?
   - Which parts are untested, or hard to test through their current interface?
3. Apply the **deletion test** to anything suspect.

### Phase 2: Present Candidates as an HTML Report

Write a self-contained HTML file to the OS temp directory. The report uses:
- **Tailwind via CDN** for layout and styling
- **Mermaid via CDN** for diagrams (call graphs, dependencies, sequences)
- Hand-built CSS/SVG for editorial visuals (mass diagrams, cross-sections)

Each candidate card contains:
- **Files** — which files/modules are involved
- **Problem** — why the current architecture is causing friction
- **Solution** — plain English description of what would change
- **Benefits** — in terms of locality and leverage
- **Before / After diagram** — side-by-side
- **Recommendation strength** — `Strong`, `Worth exploring`, or `Speculative`

End with a **Top recommendation** section.

### Phase 3: Grilling Loop

Once the user picks a candidate, walk the design tree with them. Side effects happen inline:
- **New term not in `CONTEXT.md`?** Add it.
- **Sharpening a fuzzy term?** Update `CONTEXT.md`.
- **User rejects candidate with good reason?** Offer to record an ADR.
- **Want to explore alternative interfaces?** Use interface design patterns.

---

## Deepening Strategy (from DEEPENING.md)

### Dependency Categories

| Category | Description | Testing Strategy |
|----------|-------------|-----------------|
| **In-process** | Pure computation, no I/O | Test through new interface directly. No adapter needed. |
| **Local-substitutable** | Dependencies with local test stand-ins (PGLite, in-memory FS) | Test with stand-in. Seam is internal. |
| **Remote but owned (Ports & Adapters)** | Your own services across network | Define port at seam. In-memory adapter for tests. |
| **True external (Mock)** | Third-party services (Stripe, Twilio) | Injected port; mock adapter for tests. |

### Seam Discipline

- **One adapter = hypothetical. Two adapters = real.** Don't introduce a port unless at least two adapters are justified.
- **Internal vs external seams.** A deep module can have internal seams (private, used by its own tests) and an external seam at its interface. Don't expose internal seams through the interface.

### Testing Strategy: Replace, Don't Layer

- Delete old unit tests on shallow modules once tests at the deepened module's interface exist.
- Write new tests at the deepened module's interface.
- Tests assert on observable outcomes through the interface, not internal state.
- Tests should survive internal refactors.

---

## Visual Pattern Examples (from HTML-REPORT.md)

### Diagram Types

1. **Mass diagram** — two boxes side by side: current (many small boxes connected by dashed lines) vs proposed (one large box with clean arrows). Shows weight of complexity collapsing.
2. **Cross-section box** — zoomed-in view of one module showing internal seams. Used to illustrate depth without exposing internals to callers.
3. **Before/After call graph** — Mermaid flowchart. Left side: 7 nodes with arrows between them. Right side: 3 nodes with clean hierarchy. The message: "this became deep."
4. **Leak detection** — Mermaid graph with a red-highlighted arrow crossing from one module to another. The fix is a dashed seam line and a new adapter node catching the leak.
5. **Seam map** — a module with two adapters plugged into it (production + test), plus callers all going through the interface.

### HTML Report Structure

```html
<!doctype html>
<html lang="en">
  <head>
    <!-- Tailwind + Mermaid from CDN -->
  </head>
  <body>
    <main>
      <header><!-- repo name, date, legend --></header>
      <section id="candidates"><!-- article cards --></section>
      <section id="top-recommendation"><!-- pick one --></section>
    </main>
  </body>
</html>
```

---

## When to Use This Skill

- Before major refactors
- When a codebase feels "hard to change"
- When tests are brittle or missing
- When modules are tightly coupled
- Every few days during active development to fight entropy
- When onboarding to a new codebase

## When NOT to Use

- Throwaway prototypes
- One-line fixes
- When you're in the middle of a bug hunt (use debugging first)
