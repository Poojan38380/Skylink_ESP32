# Map Integration Plan

## Phase 1 — Basic Map Display
- Add #map div to index.html
- Initialize Leaflet with OSM tiles
- Default center: SITL home (-35.363, 149.165)

## Phase 2 — Drone Marker & Trail
- Custom drone icon with heading rotation
- Polyline trail from GPS breadcrumbs
- Home marker (H icon)

## Phase 3 — Interactive Controls
- Center/Follow buttons
- Click-to-fly (GOTO_LATLON)
- Geofence circle overlay

## Phase 4 — Attitude HUD
- Roll/pitch bubble overlay on map
- Live strip (mode, arm, alt, speed)

## Phase 5 — Flight Actions Dock
- Map-level Loiter/Land/RTL buttons
- Distance picker for body-frame moves
- Keyboard arrow key support
