#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "Robot.h"
#include "RobotWifiAP.h"
#include "TaskTimer.h"
#include "OdometryModel.h"

// Map coordinates refer to the DN3 start reference: x right, y up, millimetres.
// The axle-to-sensor forward offset MUST be measured for accurate physical maps.
class Controller_c {
public:
  static constexpr uint8_t LANES = 7;
  static constexpr uint8_t ROWS = 41;
  static constexpr float LENGTH_MM = 200.0f;
  static constexpr float LANE_STEP_MM = 50.0f;
  static constexpr float SAMPLE_STEP_MM = 5.0f;
  static constexpr float SENSOR_FORWARD_MM = 0.0f; // Unmeasured assumption!
  static constexpr float DRIVE_PWM = 45.0f;
  static constexpr float TURN_PWM = 35.0f;
  OdometryModel_c odometry;

  Controller_c();
  bool start(); // Queues a new scan; motion begins only in update().
  void stop(Robot_c &robot);
  void reset(); // State only, not a motor command.
  void setSignal(uint8_t value);
  uint8_t getSignal() const;
  bool isStartComplete() const;
  void requestDump();
  void reportStatus(RobotWifiAP_c &server);
  void update(Robot_c &robot, RobotWifiAP_c &server);

private:
  enum Mode { WAITING, SWEEP, TURN_OUT, SHIFT, TURN_IN, SETTLING,
              COMPLETED, ABORTED, FAULT };
  Mode mode = WAITING, nextMode = WAITING;
  bool pendingStart = false, pendingAbort = false, pendingDump = false;
  bool lastButton = false;
  uint8_t signal = 0, lane = 0, stableHits = 0;
  uint32_t runId = 0, stageSince = 0, lastMotionMs = 0, dumpSince = 0;
  uint32_t lastSurfaceMs = 0;
  uint16_t dumpIndex = 0;
  bool dumping = false, stopping = false;
  float segmentX = 0, segmentY = 0, headingTarget = 0, lastX = 0, lastY = 0, lastTheta = 0;
  int lastSampleRow = -1;
  uint32_t sums[LANES][ROWS];
  uint16_t counts[LANES][ROWS];
  float positionX[LANES][ROWS], positionY[LANES][ROWS];
  TaskTimer_c controlTimer, telemetryTimer;

  const char* modeName() const;
  bool active() const;
  static float angleError(float target, float measured);
  void emit(RobotWifiAP_c &server, const char* message);
  void metadata(RobotWifiAP_c &server, bool newRun);
  void publishCell(RobotWifiAP_c &server, uint8_t col, uint8_t row);
  void sample(Robot_c &robot, RobotWifiAP_c &server, float along);
  void enter(Mode target, uint32_t now, RobotWifiAP_c &server);
  bool motor(Robot_c &robot, RobotWifiAP_c &server, float left, float right);
  void fail(Robot_c &robot, RobotWifiAP_c &server, const char* reason);
  void settle(Robot_c &robot, RobotWifiAP_c &server, Mode target, uint32_t now);
  void control(Robot_c &robot, RobotWifiAP_c &server, uint32_t now, bool freshSurface);
  void telemetry(Robot_c &robot, RobotWifiAP_c &server, uint32_t now);
};
#endif
