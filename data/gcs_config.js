/**
 * Skylink GCS — browser-side tunables (UI only).
 * Safety limits are enforced on the ESP32 (see include/skylink_config.h).
 */
const SKYLINK_GCS_CONFIG = {
  protocolVersion: 1,
  fsBuild: 7,  // Must match data/skylink_build.json — bump before uploadfs
  defaultTab: 'map',
  preflightMinGpsFix: 3,
  preflightMinBatteryPct: 20,
  simulationBanner: true,

  mapDefaultLat: -35.363261,
  mapDefaultLng: 149.16523,
  mapDefaultZoom: 17,
  mapFollowDrone: true,
  mapTrailMaxPoints: 60,
  mapTileUrl: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',

  wsReconnectInitialMs: 1500,
  wsReconnectMaxMs: 20000,

  commsLogMaxEntries: 40,
  defaultTakeoffAltM: 5,
  takeoffAltMinM: 1,
  takeoffAltMaxM: 50,

  armModeDelayMs: 400,
  takeoffArmDelayMs: 500,

  movePresetsM: [1, 3, 5, 10],
  yawPresetsDeg: [45, 90, 180],

  sitlPortDefault: 5763,
  armHoldMs: 1500,
};
