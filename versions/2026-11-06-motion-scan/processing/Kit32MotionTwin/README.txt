Kit32 Motion Twin (Processing 4 / Java mode)

Open Kit32MotionTwin.pde in Processing and press Run.
No extra libraries are required.

SIMULATION (default)
Press Start to run the offline motion model.
The robot follows 7 lanes of 200 mm with 6 lateral shifts of 50 mm.
Only the motion trail is drawn. There is no terrain or sensor simulation.
Stop stops the model; Start resets and starts a new run.
Speed cycles through x1, x2 and x4. Clear Trail clears drawn points only.

LIVE
Join the robot Wi-Fi configured in RobotNetwork.local.h network first.
Disconnect the robot from the web console or other telemetry applications.
Click Connect Live to connect to 192.168.4.1:80.
Connecting sends PING and STATUS only; it does not start movement.
Start and Stop send START and STOP to the real robot in Live mode.
Disconnect does not stop an already running robot.
After disconnecting, click Mode: Live to return to Simulation.

CONTROLLER (matched to firmware/Kit32_StampC3/PathFollower.h)
Fixed vertical lines x=0,50,...,300 mm; horizontal lines y=0 or 200 mm.
Cross-track error changes desired heading using a 25 mm lookahead, limited
to 30 degrees. Heading feedback generates differential wheel commands.
Endpoint tolerances: 1 mm along-track and 3 mm cross-track.
Turn tolerance: 1.5 degrees for 3 samples, then 200 ms settling.
Nonzero PWM >=40; startup/reversal floor50 for120 ms. These are initial
engineering settings, not ground-validated motor thresholds or boost duration.
Path error >30 mm or endpoint overshoot >8 mm causes a fault stop.

MODEL LIMITS
Wheelbase: 90 mm. Wheel speeds use separate left/right, forward/reverse
unloaded encoder-rate measurements at PWM40 and PWM50 from Oct 18.
Rates are linearly interpolated/extrapolated locally, then converted using
16 mm nominal wheel radius and 358.3 counts/revolution.
No measured loaded dynamics, inertia, wheel slip or battery model.
Simulation uses 10 ms control updates, matching the firmware timer.
This is a functional motion model, not a calibrated accuracy prediction.
Live positions are encoder estimates, not independent ground truth.
