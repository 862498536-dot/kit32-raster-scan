#pragma once
#include "RobotWifiAP.h"
// Join the computer's 2.4 GHz hotspot; keep actual settings in the ignored file.
#if __has_include("RobotNetwork.local.h")
#include "RobotNetwork.local.h"
#else
#define ROBOT_WIFI_SSID "YOUR_PC_HOTSPOT"
#define ROBOT_WIFI_PASSWORD "CHANGE_ME_123"
#endif
RobotWifiAP_c server(ROBOT_WIFI_SSID, ROBOT_WIFI_PASSWORD, 80);
