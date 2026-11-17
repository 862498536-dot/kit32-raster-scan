# Closed-loop motion and scanning

This version retains the encoder-based path controller and DN3 mapping, with the
computer-hotspot connection restored on 2026-11-17. The separate timed open-loop
firmware has been removed. The original project remains at the repository root.

## Firmware

Open `firmware/Kit32_StampC3/Kit32_StampC3.ino` with its entire folder. Select
M5StampC3; use M5Stack ESP32 core 3.3.10, LIS3MDL 2.0.0 and LSM6 2.0.1.

Copy `RobotNetwork.local.example.h` to `RobotNetwork.local.h`, and enter the
computer's 2.4 GHz hotspot SSID and password. Actual local settings are ignored
by Git. The computer remains connected to its Internet Wi-Fi while sharing the
hotspot; the robot joins it and reports its DHCP address as `INFO,WIFI_IP,...`.
The TCP port is 80. The hotspot must be enabled for a wireless connection.

Power-up explicitly requests zero motor output and remains idle. START or GPIO3
starts a run; STOP stops it. STATUS includes the firmware identifier
`KIT32_CLOSED_LOOP_20261117`. The route uses seven 200 mm sweeps at 50 mm spacing,
with position and heading feedback derived from encoders. Only DN3 is recorded
into the 41-by-7 matrix. Missing cells remain missing; they are not interpolated.

The path controller uses a nonzero PWM floor of 40 and a startup/reversal floor
of 50 for 120 ms. Turn tolerance is 1.5 degrees. Excess path error, encoder stall,
sensor read failure, and segment timeout stop the run. These settings and wheel
geometry still require ground calibration on each physical car.

## Web console

From this version folder, with Node.js 20 or later:

```text
npm test
npm start
```

Open http://127.0.0.1:8766/ and enter the IP reported by the robot. The default
192.168.137.250 is a placeholder; DHCP can change it. Real records are saved under
`web_console/results/` with matrix, trajectory and CSV/JSON/PNG export support.
Connecting does not start movement. Disconnecting does not stop an active scan;
use STOP. The robot supports one TCP client; disconnect the web console before
connecting Processing.

## Processing and printed map

Open `processing/Kit32MotionTwin/Kit32MotionTwin.pde` in Processing 4 / Java mode.
The offline simulator models the path controller; no fake sensor map is drawn.
For Live mode, set ROBOT_IP to the DHCP address and leave the computer hotspot on.

Print `print/A3_Scan_Test_Map.pdf` on A3 landscape at Actual size / 100 percent.
Check the 100 mm ruler. The scan field is 300 by 200 mm; leave room for the body
and turns. The PDF generator requires Python, reportlab and pypdf.

## Verification limits

See VALIDATION.md for prior software checks. Compilation and a stationary USB
connection do not prove physical scan accuracy. Full real-robot scans, loaded
motor behavior, slip, sensor offset and turn/distance calibration remain pending.
Original course and library attribution is retained.
