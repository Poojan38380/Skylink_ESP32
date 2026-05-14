# SET_MODE Command Flow

## User Action → MAVLink Packet
```
User clicks "GUIDED" → WS: {"action":"SET_MODE","mode":4}
  → web_server.cpp: handleWsAction()
  → flight_controller.cpp: setFlightMode(4)
    → setCopterMode(COPTER_MODE_GUIDED)
      → mavlink_msg_command_long_pack(MAV_CMD_DO_SET_MODE, MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 4)
      → sendMavlinkPacket() → UART2/TCP
  → Autopilot ACKs with COMMAND_ACK (#77)
  → HEARTBEAT confirms mode in custom_mode field
```

### ArduCopter Mode IDs
| Mode | ID | Description |
|------|------|-------------|
| STABILIZE | 0 | Manual angle control |
| ALT_HOLD | 2 | Altitude hold |
| AUTO | 3 | Mission waypoints |
| GUIDED | 4 | Accept position targets |
| LOITER | 5 | GPS position hold |
| RTL | 6 | Return to launch |
| LAND | 9 | Controlled descent |
