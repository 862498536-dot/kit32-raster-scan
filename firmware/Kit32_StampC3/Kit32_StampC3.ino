#include <Wire.h>
#include <string.h>

#include "RobotWifiAP.h"

// WifiCredentials.h contains the current hotspot configuration.
// The owner requested that it be tracked in this private repository.
#include "WifiCredentials.h"
RobotWifiAP_c server(HOTSPOT_SSID, HOTSPOT_PASSWORD, 80);

#include "Robot.h"
#include "RobotIMU.h"
#include "Controller.h"


/** @brief Hardware-facing interface to the Pololu robot. */
Robot_c robot;
/** @brief Direct interface to the 3Pi+ accelerometer, gyro, and magnetometer. */
RobotIMU_c imu;
/** @brief Supplied nominal line-following controller. */
Controller_c controller;

/**
 * @brief Receives deliberately small, newline-terminated commands from the
 * local web control page. The commands are START, STOP, and PING.
 */
constexpr size_t SERIAL_COMMAND_CAPACITY = 16;
char serial_command[SERIAL_COMMAND_CAPACITY];
size_t serial_command_length = 0;

void sendCommandReply(const char* reply) {
  Serial.println(reply);
  server.sendLine(reply);
}

void processSerialCommand() {
  serial_command[serial_command_length] = '\0';

  if (strcmp(serial_command, "START") == 0) {
    sendCommandReply(controller.start() ? "ACK,START" : "ERR,START_BUSY");
  } else if (strcmp(serial_command, "STOP") == 0) {
    controller.stop(robot);
    sendCommandReply("ACK,STOP");
  } else if (strcmp(serial_command, "DUMP") == 0) {
    controller.requestDump();
    sendCommandReply("ACK,DUMP");
  } else if (strcmp(serial_command, "STATUS") == 0) {
    controller.reportStatus(server);
  } else if (strcmp(serial_command, "PING") == 0) {
    sendCommandReply("ACK,PING");
  } else if (serial_command_length > 0) {
    sendCommandReply("ACK,UNKNOWN");
  }

  serial_command_length = 0;
}

void acceptCommandByte(char received) {
  if (received == '\r') {
    return;
  }
  if (received == '\n') {
    processSerialCommand();
    return;
  }

  if (serial_command_length < SERIAL_COMMAND_CAPACITY - 1) {
    serial_command[serial_command_length++] = received;
  } else {
    // Discard an overlong command rather than processing a truncated prefix.
    serial_command_length = 0;
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    acceptCommandByte(static_cast<char>(Serial.read()));
  }
}

void readWifiCommands() {
  while (server.available() > 0) {
    acceptCommandByte(static_cast<char>(server.read()));
  }
}


/** @brief Initialises serial, WiFi, IMU, and the supplied robot-controller baseline. */
void setup() {

  Serial.begin(115200);
  if (!server.begin()) {
    Serial.println("ERROR,WIFI_SETUP_FAILED");
  } else {
    Serial.println("INFO,WIFI_CONNECTING");
  }

  robot.initialise();
//  robot.initialise("team name");


  // Robot_c starts the shared I2C bus before RobotIMU_c uses it. IMU readings
  // come directly from the sensor ICs and do not pass through the middleware.
  if (!imu.initialise() ) {
    Serial.println("Warning: LSM6DS33 IMU was not detected.");
    robot.setLED( 90, 50, 0, 100 ); // orange warning
  }

}


/** @brief Maintains telemetry transport and runs the controller repeatedly. */
void loop() {
  readSerialCommands();

  // Accept or maintain the optional Processing telemetry connection. The
  // controller continues to run even when no client is connected.
  server.update();
  readWifiCommands();


  // Run an iteration of the robot controller code.
  // Develop controller routines in Controller.cpp; edit parameters in Controller.h.
  controller.update(robot, server);

}
