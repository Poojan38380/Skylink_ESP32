# RC_CHANNELS_OVERRIDE (#70) — Channel Mapping

| Channel | Function | Neutral | Range |
|---------|----------|---------|-------|
| CH1 | Roll | 1500 | 1000-2000 |
| CH2 | Pitch | 1500 | 1000-2000 |
| CH3 | Throttle | 1500 | 1000-2000 |
| CH4 | Yaw | 1500 | 1000-2000 |

## Current Usage — emergencyStop() only:
```cpp
sendRCOverride(1500, 1500, 1000, 1500);  // neutral + min throttle
sendArmDisarm(false);                     // then disarm
```

## Safety: RC_OVERRIDE can fight the real RC transmitter.
ArduPilot `RC_OVERRIDE_TIME` timeout default 3s, then reverts to physical RC.
