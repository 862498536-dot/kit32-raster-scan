# Verification performed before publication

- Before the 2026-11-17 update, the path-tracking firmware compiled for M5StampC3 using M5Stack ESP32 core3.3.10.
- Packaged network defaults are non-personal demo settings; actual local Wi-Fi
  credentials and binaries were excluded from this added version directory.
- Processing4.5.5 opened and ran the single PDE sketch in the actual editor.
  The corrected model completed all seven lanes without a runtime error.
- MotionTrackingCheck exercised no disturbance and +/-8 mm lateral disturbances
  during the second lane. All three runs completed. Largest lane-end lateral
  error observed was approximately2.6 mm; final position approximately
  (298.6,199.8) mm. All nonzero PWM values were at least40.
- Checks also covered steering direction, excess path-error stopping and live
  telemetry parsing. This is model-level verification, not measured robot accuracy.
- Existing scan-model and fake-TCP/web-bridge tests passed.
- A3 PDF is one420x297 mm page; the scan field is300x200 mm. The100 mm calibration
  ruler is drawn in physical PDF units. A rendered page was visually inspected.

Not yet verified: full real-robot scanning, loaded motor rates, start/stop dynamics,
wheel slip, exact body/sensor geometry and achieved real turn angles/distances.

Java checks under tests/ use Processing's preprocessor and core library to compile
and exercise the actual PDE source. PrepareMotionSketch takes the input PDE path
and generated Java output path. Generated Java must be compiled as Kit32MotionTwin
together with MotionTrackingCheck. These checks do not communicate with hardware.

2026-11-17 change: removed the timed open-loop firmware; restored PC-hotspot station mode; added an explicit zero motor request at boot and a queryable firmware identifier. Deployment results are recorded separately after verification.

The 2026-11-17 build, owner-car restore, final upload and stationary serial/Wi-Fi/dashboard verification passed. See DEPLOYMENT.md for the observed results and physical-test limits.
