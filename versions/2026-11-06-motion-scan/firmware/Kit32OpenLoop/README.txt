KIT32 OPEN-LOOP SCANNING EXPERIMENT

Open Kit32OpenLoop.ino in Arduino IDE. Board: M5StampC3.
This is independent of the existing path-correcting firmware.

All motor commands are fixed PWM and fixed duration. No heading correction,
path correction, wheel-speed regulation, encoder-distance endpoint, or
automatic reorientation is used. Encoders are read ONLY for trajectory display.

Edit OpenLoopPlan.h to tune the experiment:
  Forward / sideways PWM: 50 on both wheels
  Each sweep: 1587 ms (nominal 200 mm)
  Each lateral shift: 397 ms (nominal 50 mm)
  Turn PWM: left/right +40/-40 for clockwise; -40/+40 for counterclockwise
  Clockwise 90-degree estimate: 750 ms
  Counterclockwise 90-degree estimate: 761 ms
  Stop between movements: 200 ms
These times use Oct 18 UNLOADED measurements and nominal wheel geometry.
They are initial estimates, not measured ground distances or turn angles.
The schedule has 7 sweeps, 6 shifts and 12 turns, then stops automatically.

Wi-Fi: configure RobotNetwork.local.h (see the version README).
Robot IP: 192.168.4.1. Do not open multiple TCP clients simultaneously.
Connect does not start motion. START or the GPIO3 button starts the plan.
STOP or pressing GPIO3 while running stops the plan. Power-up remains idle.
Motor communication failure or missing surface readings for 500 ms stops motion.
This retains stop/fault handling, but does not perform motion correction.

DN3 DATA / MAP
Only fresh DN3 readings during the seven sweeps enter the 41x7 matrix.
Matrix row is assigned by elapsed sweep time; column is the scheduled lane.
Readings are real, but matrix coordinates are NOMINAL TIME-BASED coordinates.
With no correction, actual samples may be displaced, rotated or overlap.
The transmitted per-cell x/y values are encoder-estimated sample positions.
Turns and shifts are logged in telemetry but not inserted into the matrix.
Missing bins remain empty. No synthetic readings or interpolation are used.

The existing web console supports START / STOP / STATUS / DUMP and exporting
the real readings. It logs INFO,OPEN_LOOP,TIME_INDEXED_MAP on connection.
Use the browser matrix as a time-indexed record, not an accurate ground map.
The existing Processing Live mode can display the actual estimated trajectory;
its OFFLINE simulation still represents the separate path-correcting version.

No upload was performed while preparing this sketch.
