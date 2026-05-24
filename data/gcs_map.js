// Skylink GCS — Leaflet map (Phase 2)
'use strict';

const SkylinkMap = (function () {
  const CFG = typeof SKYLINK_GCS_CONFIG !== 'undefined' ? SKYLINK_GCS_CONFIG : {};
  const TRAIL_MAX = CFG.mapTrailMaxPoints || 60;

  let map = null;
  let droneMarker = null;
  let homeMarker = null;
  let trailLine = null;
  let followDrone = CFG.mapFollowDrone !== false;
  let homeSet = false;
  const trail = [];
  let lastDronePos = null;

  function isValidCoord(lat, lng) {
    if (!Number.isFinite(lat) || !Number.isFinite(lng)) return false;
    if (Math.abs(lat) < 1e-6 && Math.abs(lng) < 1e-6) return false;
    return Math.abs(lat) <= 90 && Math.abs(lng) <= 180;
  }

  function normalizeHeading(deg) {
    let h = deg % 360;
    if (h < 0) h += 360;
    return h;
  }

  function droneIcon(headingDeg) {
    const h = normalizeHeading(headingDeg);
    return L.divIcon({
      className: 'drone-marker-wrap',
      html: '<div class="drone-arrow" style="transform:rotate(' + h + 'deg)">▲</div>',
      iconSize: [28, 28],
      iconAnchor: [14, 14],
    });
  }

  function homeIcon() {
    return L.divIcon({
      className: 'home-marker-wrap',
      html: '<div class="home-marker">H</div>',
      iconSize: [22, 22],
      iconAnchor: [11, 11],
    });
  }

  function init() {
    const el = document.getElementById('map');
    if (!el || typeof L === 'undefined') return;

    const lat = CFG.mapDefaultLat != null ? CFG.mapDefaultLat : -35.363261;
    const lng = CFG.mapDefaultLng != null ? CFG.mapDefaultLng : 149.16523;
    const zoom = CFG.mapDefaultZoom != null ? CFG.mapDefaultZoom : 17;

    map = L.map(el, { zoomControl: true, attributionControl: true }).setView([lat, lng], zoom);

    const tileUrl = CFG.mapTileUrl || 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
    L.tileLayer(tileUrl, {
      maxZoom: 19,
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OSM</a>',
    }).addTo(map);

    trailLine = L.polyline([], {
      color: '#00d4ff',
      weight: 2,
      opacity: 0.75,
    }).addTo(map);

    droneMarker = L.marker([lat, lng], { icon: droneIcon(0), zIndexOffset: 1000 }).addTo(map);

    const btnCenter = document.getElementById('btn-map-center');
    if (btnCenter) {
      btnCenter.addEventListener('click', () => {
        if (lastDronePos) map.setView(lastDronePos, Math.max(map.getZoom(), 17));
      });
    }

    const btnFollow = document.getElementById('btn-map-follow');
    if (btnFollow) {
      btnFollow.addEventListener('click', () => {
        followDrone = !followDrone;
        btnFollow.classList.toggle('active', followDrone);
        btnFollow.textContent = followDrone ? '◎ FOLLOW' : '○ FOLLOW';
      });
      btnFollow.classList.toggle('active', followDrone);
    }

    window.addEventListener('resize', () => {
      if (map) map.invalidateSize();
    });

    setTimeout(() => {
      if (map) map.invalidateSize();
    }, 200);
  }

  function pushTrail(lat, lng) {
    const last = trail[trail.length - 1];
    if (last && last[0] === lat && last[1] === lng) return;
    trail.push([lat, lng]);
    if (trail.length > TRAIL_MAX) trail.shift();
    trailLine.setLatLngs(trail);
  }

  function setHome(lat, lng) {
    if (!map || !isValidCoord(lat, lng)) return;
    if (homeMarker) {
      homeMarker.setLatLng([lat, lng]);
      homeSet = true;
      return;
    }
    homeSet = true;
    homeMarker = L.marker([lat, lng], { icon: homeIcon(), zIndexOffset: 500 }).addTo(map);
    homeMarker.bindTooltip('Home', { permanent: false, direction: 'top' });
  }

  function updateFromTelemetry(d) {
    if (!map || !droneMarker) return;

    const lat = Number(d.lat);
    const lng = Number(d.lng);
    if (!isValidCoord(lat, lng)) return;

    const yaw = Number(d.yaw);
    const heading = Number.isFinite(yaw) ? yaw : 0;

    droneMarker.setLatLng([lat, lng]);
    droneMarker.setIcon(droneIcon(heading));
    lastDronePos = [lat, lng];

    if (d.home_valid && isValidCoord(Number(d.home_lat), Number(d.home_lng))) {
      setHome(Number(d.home_lat), Number(d.home_lng));
    } else {
      const fix = Number(d.gps_fix) || 0;
      if (!homeSet && fix >= 2) setHome(lat, lng);
    }

    pushTrail(lat, lng);

    if (followDrone) {
      map.panTo([lat, lng], { animate: true, duration: 0.25 });
    }

    const hudAlt = document.getElementById('map-hud-alt');
    const hudSpd = document.getElementById('map-hud-spd');
    const hudSats = document.getElementById('map-hud-sats');
    if (hudAlt) hudAlt.textContent = 'ALT ' + (Number(d.altitude) || 0).toFixed(1) + ' m';
    if (hudSpd) hudSpd.textContent = 'SPD ' + (Number(d.speed) || 0).toFixed(1) + ' m/s';
    if (hudSats) hudSats.textContent = 'GPS ' + (Number(d.sats) || 0) + ' sats';
  }

  document.addEventListener('DOMContentLoaded', init);

  return { updateFromTelemetry, centerOnDrone: () => {
    if (map && lastDronePos) map.setView(lastDronePos, Math.max(map.getZoom(), 17));
  }};
})();
