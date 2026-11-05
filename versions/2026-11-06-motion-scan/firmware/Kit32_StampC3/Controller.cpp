#include "Controller.h"
#include <math.h>
#include <string.h>

constexpr float Controller_c::LENGTH_MM;
constexpr float Controller_c::LANE_STEP_MM;
constexpr float Controller_c::SAMPLE_STEP_MM;
constexpr float Controller_c::SENSOR_FORWARD_MM;
constexpr float Controller_c::DRIVE_PWM;
constexpr float Controller_c::TURN_PWM;
constexpr uint8_t Controller_c::LANES;
constexpr uint8_t Controller_c::ROWS;

Controller_c::Controller_c() : controlTimer(10), telemetryTimer(50) {
  memset(sums, 0, sizeof(sums)); memset(counts, 0, sizeof(counts));
  memset(positionX, 0, sizeof(positionX)); memset(positionY, 0, sizeof(positionY));
}
bool Controller_c::active() const {
  return mode == SWEEP || mode == TURN_OUT || mode == SHIFT ||
         mode == TURN_IN || mode == SETTLING;
}
const char* Controller_c::modeName() const {
  switch (mode) {
    case SWEEP: return "SWEEP"; case TURN_OUT: return "TURN_OUT";
    case SHIFT: return "SHIFT"; case TURN_IN: return "TURN_IN";
    case SETTLING: return "SETTLING"; case COMPLETED: return "COMPLETED";
    case ABORTED: return "ABORTED"; case FAULT: return "FAULT";
    default: return "WAITING";
  }
}
void Controller_c::reset() {
  pendingStart = false; signal = 0; mode = WAITING;
}
bool Controller_c::start() {
  if (active() || pendingStart || stopping) return false;
  pendingStart = true; pendingAbort = false; dumping = false; return true;
}
void Controller_c::setSignal(uint8_t value) {
  if (value) start(); else pendingAbort = true;
}
uint8_t Controller_c::getSignal() const { return signal; }
bool Controller_c::isStartComplete() const { return mode == COMPLETED; }
void Controller_c::stop(Robot_c &robot) {
  pendingStart = false; pendingAbort = true;
  stopping = !robot.setMotorPWM(0, 0);
  signal = 0;
}
void Controller_c::requestDump() { pendingDump = true; }
float Controller_c::angleError(float target, float measured) {
  float error = target - measured;
  while (error > PI) error -= TWO_PI;
  while (error < -PI) error += TWO_PI;
  return error;
}
void Controller_c::emit(RobotWifiAP_c &server, const char* message) {
  Serial.println(message); server.sendLine(message);
}
void Controller_c::metadata(RobotWifiAP_c &server, bool newRun) {
  char line[180];
  snprintf(line, sizeof(line), "SCAN,%s,%lu,300,200,50,5,7,41,%.1f",
           newRun ? "BEGIN" : "META", (unsigned long)runId, SENSOR_FORWARD_MM);
  emit(server, line);
}
void Controller_c::reportStatus(RobotWifiAP_c &server) {
  metadata(server, false);
  char line[100];
  snprintf(line, sizeof(line), "SCAN,STATE,%lu,%s,%u", (unsigned long)runId, modeName(), lane);
  emit(server, line);
}
void Controller_c::publishCell(RobotWifiAP_c &server, uint8_t col, uint8_t row) {
  uint16_t n = counts[col][row]; if (!n) return;
  char line[180];
  snprintf(line, sizeof(line), "SCAN,CELL,%lu,%u,%u,%.2f,%u,%.2f,%.2f",
           (unsigned long)runId, col, row, float(sums[col][row]) / n, n,
           positionX[col][row] / n, positionY[col][row] / n);
  emit(server, line);
}
void Controller_c::sample(Robot_c &robot, RobotWifiAP_c &server) {
  // Use global sensor y, not a segment-relative distance that hides endpoint drift.
  float y = odometry.pose.y + SENSOR_FORWARD_MM * sinf(odometry.pose.theta);
  int row = int(lroundf(y / SAMPLE_STEP_MM));
  if (row < 0 || row >= ROWS || y < -2.5f || y > LENGTH_MM + 2.5f) return;
  if (row != lastSampleRow && lastSampleRow >= 0) publishCell(server, lane, lastSampleRow);
  lastSampleRow = row;
  if (counts[lane][row] >= 60000) return;
  sums[lane][row] += robot.surface.reading[2]; counts[lane][row]++;
  positionX[lane][row] += odometry.pose.x + SENSOR_FORWARD_MM * cosf(odometry.pose.theta);
  positionY[lane][row] += odometry.pose.y + SENSOR_FORWARD_MM * sinf(odometry.pose.theta);
}
void Controller_c::enter(Mode target, uint32_t now, RobotWifiAP_c &server) {
  mode = target; stageSince = now; lastMotionMs = now; stableHits = 0;
  segmentX = lastX = odometry.pose.x; segmentY = lastY = odometry.pose.y;
  lastTheta = odometry.pose.theta;
  if (target == SWEEP) {
    headingTarget = lane % 2 == 0 ? HALF_PI : -HALF_PI;
    lastSampleRow = -1;
  } else if (target == TURN_OUT || target == SHIFT) headingTarget = 0;
  else if (target == TURN_IN) headingTarget = ((lane + 1) % 2 == 0) ? HALF_PI : -HALF_PI;
  reportStatus(server);
}
bool Controller_c::motor(Robot_c &robot, RobotWifiAP_c &server, float l, float r) {
  unsigned long now = robot.getMillis();
  l = PathFollower::powered(l, leftPowerSign, leftPowerSince, now);
  r = PathFollower::powered(r, rightPowerSign, rightPowerSince, now);
  if (robot.setMotorPWM(l, r)) return true;
  fail(robot, server, "MOTOR_I2C"); return false;
}
void Controller_c::fail(Robot_c &robot, RobotWifiAP_c &server, const char* reason) {
  mode = FAULT; signal = 0; pendingStart = false;
  stopping = !robot.setMotorPWM(0, 0);
  if (lastSampleRow >= 0) { publishCell(server, lane, lastSampleRow); lastSampleRow = -1; }
  char line[120]; snprintf(line, sizeof(line), "SCAN,END,%lu,FAULT,%s", (unsigned long)runId, reason);
  emit(server, line);
}
void Controller_c::settle(Robot_c &robot, RobotWifiAP_c &server, Mode target, uint32_t now) {
  if (!motor(robot, server, 0, 0)) return;
  nextMode = target; mode = SETTLING; stageSince = now;
}
void Controller_c::control(Robot_c &robot, RobotWifiAP_c &server, uint32_t now, bool freshSurface) {
  if (mode == SETTLING) {
    if (now - stageSince >= 200) {
      if (nextMode == SWEEP) lane++;
      enter(nextMode, now, server);
    }
    return;
  }
  if (!active()) return;
  bool turn = mode == TURN_OUT || mode == TURN_IN;
  if (now - stageSince > (turn ? 10000u : 20000u)) { fail(robot, server, "SEGMENT_TIMEOUT"); return; }
  if (hypotf(odometry.pose.x-lastX, odometry.pose.y-lastY) > 0.5f ||
      fabsf(odometry.pose.theta-lastTheta) > 0.01f) {
    lastMotionMs = now; lastX = odometry.pose.x; lastY = odometry.pose.y; lastTheta = odometry.pose.theta;
  }
  if (now - lastMotionMs > 2000) { fail(robot, server, "ENCODER_STALL"); return; }
  float error = angleError(headingTarget, odometry.pose.theta);
  if (turn) {
    if (fabsf(error) < PathFollower::TURN_TOL_RAD) {
      if (!motor(robot, server, 0, 0)) return;
      if (++stableHits >= 3) settle(robot, server, mode == TURN_OUT ? SHIFT : SWEEP, now);
    } else {
      stableHits = 0;
      float turnPwm = error > 0 ? TURN_PWM : -TURN_PWM;
      motor(robot, server, -turnPwm, turnPwm);
    }
    return;
  }
  const PathFollower::Line line = PathFollower::reference(mode == SWEEP, lane,
                                                        odometry.pose.x, odometry.pose.y);
  if (fabsf(line.cross) > PathFollower::MAX_CROSS_MM ||
      line.remaining < -PathFollower::MAX_OVERSHOOT_MM) {
    fail(robot, server, "PATH_ERROR"); return;
  }
  if (mode == SWEEP && freshSurface) sample(robot, server);
  if (line.remaining <= PathFollower::END_TOL_MM && fabsf(line.cross) <= PathFollower::CROSS_TOL_MM) {
    if (mode == SWEEP) {
      if (lastSampleRow >= 0) { publishCell(server, lane, lastSampleRow); lastSampleRow = -1; }
      if (lane + 1 == LANES) {
        if (!motor(robot, server, 0, 0)) return;
        mode = COMPLETED; signal = 0;
        char line[100]; snprintf(line, sizeof(line), "SCAN,END,%lu,COMPLETE,OK", (unsigned long)runId);
        emit(server, line); return;
      }
      settle(robot, server, TURN_OUT, now);
    } else settle(robot, server, TURN_IN, now);
    return;
  }
  // Fixed world lines plus heading feedback: bring the estimated position back
  // to the lane, rather than accepting the previous stop as the new line origin.
  error = PathFollower::wrap(line.heading-odometry.pose.theta);
  float left, right;
  PathFollower::drive(error, line.remaining, left, right);
  motor(robot, server, left, right);
}
void Controller_c::update(Robot_c &robot, RobotWifiAP_c &server) {
  uint32_t now = robot.getMillis();
  if (!controlTimer.isReady(now)) return; controlTimer.resetTimer(now);
  if (stopping) stopping = !robot.setMotorPWM(0, 0);
  bool button = digitalRead(3) == LOW;
  if (button && !lastButton) start();
  lastButton = button;
  if (pendingAbort) {
    if (lastSampleRow >= 0) { publishCell(server, lane, lastSampleRow); lastSampleRow = -1; }
    mode = ABORTED; signal = 0; pendingAbort = false;
    stopping = !robot.setMotorPWM(0, 0);
    char line[100]; snprintf(line, sizeof(line), "SCAN,END,%lu,ABORTED,USER_STOP", (unsigned long)runId);
    emit(server, line);
  }
  bool encoderOk = robot.getEncoders();
  bool surfaceOk = robot.getSurfaceSensors();
  if (surfaceOk) lastSurfaceMs = now;
  if (pendingStart) {
    if (!encoderOk) { fail(robot, server, "ENCODER_READ_AT_START"); }
    else if (motor(robot, server, 0, 0)) {
      runId++; lane = 0; signal = 1; pendingStart = false; lastSampleRow = -1;
      lastSurfaceMs = now;
      memset(sums, 0, sizeof(sums)); memset(counts, 0, sizeof(counts));
      memset(positionX, 0, sizeof(positionX)); memset(positionY, 0, sizeof(positionY));
      odometry.reset(0, -SENSOR_FORWARD_MM, HALF_PI);
      odometry.update(robot.getLeftEncoderCount(), robot.getRightEncoderCount());
      metadata(server, true); enter(SWEEP, now, server);
    }
  } else if (encoderOk) {
    odometry.update(robot.getLeftEncoderCount(), robot.getRightEncoderCount());
  } else if (active()) fail(robot, server, "ENCODER_READ");
  if (active() && now - lastSurfaceMs > 500) fail(robot, server, "SURFACE_READ");
  control(robot, server, now, surfaceOk);
  if (telemetryTimer.isReady(now)) { telemetryTimer.resetTimer(now); telemetry(robot, server, now); }
  if (pendingDump) {
    pendingDump = false;
    if (active() || pendingStart) emit(server, "ERR,DUMP_BUSY");
    else { dumping = true; dumpIndex = 0; metadata(server, false); emit(server, "SCAN,DUMP_BEGIN"); }
  }
  if (dumping && now - dumpSince >= 10) {
    dumpSince = now;
    if (dumpIndex < LANES * ROWS) {
      publishCell(server, dumpIndex / ROWS, dumpIndex % ROWS); dumpIndex++;
    } else { dumping = false; reportStatus(server); emit(server, "SCAN,DUMP_END"); }
  }
}
void Controller_c::telemetry(Robot_c &robot, RobotWifiAP_c &server, uint32_t now) {
  char line[230];
  snprintf(line, sizeof(line), "%lu,%.2f,%.2f,%.5f,%d,%d,%ld,%ld,%u,%u,%u,%u,%u,%u",
    (unsigned long)now, odometry.pose.x, odometry.pose.y, odometry.pose.theta,
    robot.getLeftMotorPWM(), robot.getRightMotorPWM(),
    (long)robot.getLeftEncoderCount(), (long)robot.getRightEncoderCount(),
    robot.surface.reading[0], robot.surface.reading[1], robot.surface.reading[2],
    robot.surface.reading[3], robot.surface.reading[4], signal);
  emit(server, line);
}
