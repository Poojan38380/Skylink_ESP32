# Map Feature Changelog

## May 19 — Module Skeleton
- Created gcs_map.js IIFE module
- Leaflet map init with OSM tiles
- Default view set to SITL home coordinates

## May 20 — Drone Marker
- Custom divIcon with CSS arrow (▲)
- Heading rotation via transform
- droneIcon() factory function

## May 21 — Trail & Home
- Polyline trail with max 60 points
- Home marker (H) with tooltip
- pushTrail() deduplication

## May 22 — Follow & Center
- Follow drone toggle button
- Center on drone button
- panTo with smooth animation

## May 23 — Geofence Circle
- L.circle overlay around home
- Configurable radius (default 1000m)
- Dashed stroke, low-opacity fill

## May 24 — Click-to-Fly (GOTO)
- Map click handler → openGotoSheet()
- Distance validation against geofence
- Altitude input with min/max bounds
- Confirm/cancel bottom sheet

## May 25 — Waypoint Movement & Polish
- Body-frame move controls below map
- Keyboard arrow bindings
- Distance presets (1/3/5/10m)
- Attitude bubble overlay
- Live strip (mode/arm/alt/speed)

## Implementation Notes
- Drone icon uses CSS transform for zero-dependency rotation
- Trail polyline color: #00d4ff (cyan accent)
- Attitude bubble uses CSS transform for roll indication
- Live strip shows mode, arm status, altitude, speed in real-time
- GOTO sheet validates target is within geofence radius
- Move controls use body-frame reference (forward/back/left/right)
- Distance presets: 1m, 3m, 5m, 10m + custom input field
- Keyboard throttled at 500ms to prevent command flooding
- Geofence tooltip shows radius in meters

## Status: COMPLETE
All 5 phases implemented and tested with SITL.
