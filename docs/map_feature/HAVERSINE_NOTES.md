# Haversine Distance Calculation

Used for:
- Geofence boundary check (drone vs home distance)
- GOTO distance validation
- Trail distance filtering

Formula implemented in both:
- gcs_map.js (browser side — UI geofence display)
- flight_controller.cpp (firmware — safety enforcement)

Both use the standard haversine formula with R = 6371000m.
