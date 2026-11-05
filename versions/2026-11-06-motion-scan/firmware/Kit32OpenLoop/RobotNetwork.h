#pragma once
#include "RobotWifiAP.h"
// Put your actual configuration in ignored RobotNetwork.local.h.
#if __has_include("RobotNetwork.local.h")
#include "RobotNetwork.local.h"
#else
#define ROBOT_WIFI_SSID "Kit32-Scan"
#define ROBOT_WIFI_PASSWORD "CHANGE_ME_123"
#endif
RobotWifiAP_c server(ROBOT_WIFI_SSID, ROBOT_WIFI_PASSWORD, 80);
