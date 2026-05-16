# Map Feature Research

## Overview
Evaluating mapping libraries for the Skylink GCS web dashboard.

## Options Considered
1. **Leaflet.js** — lightweight, open-source, mobile-friendly
2. **Mapbox GL JS** — vector tiles, 3D, requires API key
3. **OpenLayers** — full-featured, heavier bundle size
4. **Google Maps API** — requires billing, proprietary

## Decision
**Leaflet.js** selected for:
- Zero API key requirement (uses OSM tiles)
- Tiny bundle (~40KB gzip)
- Runs on ESP32-served pages (no build step)
- Plugin ecosystem (heatmaps, draw, measure)

## Requirements
- Real-time drone position marker with heading indicator
- GPS trail / breadcrumb polyline
- Home position marker
- Click-to-fly (GOTO) interaction
- Geofence visualization
- Attitude overlay (roll/pitch bubble)

## Default Config Values
- mapDefaultLat: -35.363261 (SITL home)
- mapDefaultLng: 149.16523
- mapDefaultZoom: 17
- mapFollowDrone: true
- mapTrailMaxPoints: 60
- geofenceRadiusM: 1000
