# Motion and scanning experiment - 2026-11-06

这是独立保存的新版实验包。原项目文件保留在仓库原位置；本目录不替换原固件。

## 选择要运行的版本

| 目录 | 用途 |
|---|---|
| `firmware/Kit32_StampC3/Kit32_StampC3.ino` | 固定路线、位置与朝向纠偏版，采集 DN3 |
| `firmware/Kit32OpenLoop/Kit32OpenLoop.ino` | 独立开环对照实验：固定 PWM + 时间，不做运动纠偏 |
| `processing/Kit32MotionTwin/Kit32MotionTwin.pde` | Processing 4 Java 草图；全英文面板，离线运动模拟和实车轨迹 |
| `web_console/` | DN3 实时矩阵、轨迹、CSV/JSON/PNG 导出与自动保存 |
| `print/A3_Scan_Test_Map.pdf` | A3 横向黑色图案测试底图，300×200 mm 扫描区 |

## Wi-Fi configuration

Both firmware sketches create a robot hotspot at **192.168.4.1:80**. The computer
joins the robot hotspot. Actual personal Wi-Fi settings are intentionally excluded.

In the chosen firmware folder:

1. Copy `RobotNetwork.local.example.h` to `RobotNetwork.local.h`.
2. Set `ROBOT_WIFI_SSID` and `ROBOT_WIFI_PASSWORD` in that local file.
3. Keep the password at least 8 characters. The local file is ignored by Git.

Without a local configuration file, the sketch compiles with the explicit demo
hotspot `Kit32-Scan` and demo password `CHANGE_ME_123`; change it before deployment.
Existing local working sketches on the author's computer retain their original settings.

## Arduino

Open one `.ino` with its entire folder; select **M5StampC3**. The verified local
toolchain is M5Stack ESP32 core **3.3.10**. The corrected firmware also uses
LIS3MDL 2.0.0 and LSM6 2.0.1. The open-loop sketch does not require those IMU libraries.

Power-up remains idle. GPIO3 or START starts a run; STOP stops it. The open-loop
experiment also uses GPIO3 as a stop button while running. No flashing was done
as part of saving this version to GitHub.

**Path tracking:** fixed x=0,50,...,300 mm lanes and y=0/200 mm horizontal lines;
position error steers the heading reference. Nonzero PWM floor40, startup/reversal
floor50 for120 ms, 1.5-degree turn tolerance and path-error stopping. These power
settings are initial engineering choices, not validated ground-load thresholds.

**Open loop:** fixed forward PWM50, turn PWM40; estimated durations in
`OpenLoopPlan.h`. Seven sweeps, six shifts, twelve turns and stops total about
27.357 seconds. Durations derive from unloaded motor measurements; real distances
and turn angles may differ. Encoders are for reporting only. The 41×7 DN3 matrix
uses nominal time-based coordinates, not accurate surveyed ground positions.

## Processing

Open `processing/Kit32MotionTwin/Kit32MotionTwin.pde` in Processing 4, Java mode,
and press Run. No extra libraries are needed.

- Default **Simulation** works offline and draws the motion trail with no fake map.
- **Start / Stop / Speed / Clear Trail** operate on the simulator in Simulation mode.
- **Connect Live** connects to the robot. Connection itself does not start motion.
- In Live mode, Start and Stop send actual START/STOP commands to the robot.

The offline simulation represents the **path-tracking** firmware, not the open-loop
experiment. Live mode can display either firmware's encoder-estimated trajectory.
The unloaded left/right motor model is documented in the sketch README and
`docs/motor-reference.csv`; ground load, slip and battery behavior are not calibrated.

## Web console

From this version folder, with Node.js 20+ installed:

```text
node web_console/scan_model.test.js
node web_console/bridge.test.js
node web_console/wifi_bridge.js
```

Open http://127.0.0.1:8766/ and connect to 192.168.4.1. Real records are saved under
`web_console/results/` (ignored by Git). The robot supports one TCP client: disconnect
the web console before connecting Processing. Disconnecting a client does not
automatically stop an active robot run.

For open-loop data, the console logs `INFO,OPEN_LOOP,TIME_INDEXED_MAP`. Interpret
the displayed matrix as real measurements assigned to nominal scheduled cells.

## Print

Print `print/A3_Scan_Test_Map.pdf` on A3 landscape at **Actual size / 100%**.
Do not use Fit to page. The 100 mm check ruler should measure 100 mm after printing.
Route directions are outside the sensing field to avoid contaminating DN3 readings.
Leave space for the robot body and turns. The optional PDF generator needs
Python, reportlab and pypdf.

## Validation and limitations

See [VALIDATION.md](VALIDATION.md). Firmware compilation, software tests and
Processing runs passed. Complete physical scans and ground calibration remain
to be performed. Original course/library attribution is retained; no new license
is asserted over third-party/course code.
