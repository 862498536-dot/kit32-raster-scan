# Owner-car deployment: 2026-11-17

The timed open-loop firmware directory has been removed. The encoder-based
closed-loop path controller is the deployable firmware in this version.

Deployment order:

1. Identify the owner's connected car on COM7 and read its entire 4 MB ESP32-C3
   flash into a local backup with a SHA256 checksum before writing anything.
2. Restore the saved 2026-11-03 RasterScan-v1 project that had previously run on
   the borrowed car. Verify all written segment hashes, live sensors, WAITING
   state, zero requested motor output, and TCP PING/STATUS replies.
3. Build this closed-loop version using M5Stack ESP32 3.3.10, LIS3MDL 2.0.0 and
   LSM6 2.0.1. Restore the original computer-hotspot station transport and local
   network configuration; keep personal settings out of this version's Git files.
4. Upload the new firmware to the owner's car and verify written hashes.

No borrowed-car original flash or borrowed NVS data was copied to the owner's
car. Only the saved project's bootloader, partition, OTA-selector and application
segments were used for the intermediate restore.

Observed verification after final upload:

- Firmware identifier: `KIT32_CLOSED_LOOP_20261117`, `PC_HOTSPOT`.
- Serial: 206 fresh five-sensor telemetry samples after a normal reset;
  `SCAN,STATE,0,WAITING,0`; requested left/right motor PWM 0/0.
- Direct Wi-Fi TCP: firmware identity and PING/STATUS confirmed, with 80 fresh
  telemetry samples, all reporting zero requested motor output.
- The DHCP address after the final reset was `192.168.137.250`; the dashboard
  default was updated accordingly. This address may change on another restart.
- Live localhost dashboard bridge: 96 real telemetry samples, WAITING, PWM 0/0.
- Scan-model and synthetic TCP/WebSocket persistence tests passed.
- The actual Processing source was compiled with UTF-8 using Processing 4.5.7
  libraries. Model runs at 0, +8 and -8 mm lateral disturbances completed; the
  maximum observed lane-end cross-track error was about 2.576 mm in the model.

No START command was sent to the real car. No physical scan, wheel-geometry
calibration, loaded motor calibration or measured mapping-accuracy test was run.
Reported PWM values are requested outputs, not measured wheel speed.

The original-car flash, build outputs, source snapshot including local network
settings, and deployment logs are kept locally in
`Documents/Kit32_Backups/2026-11-17-OwnCar-ClosedLoop/`.
