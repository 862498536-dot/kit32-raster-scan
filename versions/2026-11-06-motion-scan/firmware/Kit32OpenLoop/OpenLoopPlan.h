#ifndef KIT32_OPEN_LOOP_PLAN_H
#define KIT32_OPEN_LOOP_PLAN_H
#include <stdint.h>

// EDIT THESE TIMINGS AFTER THE FIRST GROUND TEST.
// Estimates from Oct 18 UNLOADED motor rates and nominal wheel geometry.
// There is NO encoder/heading/path feedback in this schedule.
namespace OpenLoopPlan {
constexpr int DRIVE_PWM = 50;
constexpr int TURN_PWM = 40;
constexpr uint32_t SWEEP_MS = 1587; // nominal 200 mm
constexpr uint32_t SHIFT_MS = 397;  // nominal 50 mm
constexpr uint32_t TURN_CW_MS = 750;
constexpr uint32_t TURN_CCW_MS = 761;
constexpr uint32_t SETTLE_MS = 200;
constexpr unsigned MOVES = 25;
struct Move { int left, right; uint32_t duration; unsigned lane; const char* name; };
inline Move get(unsigned index) {
  unsigned lane=index/4, part=index%4;
  if(index>=MOVES)return {0,0,0,6,"COMPLETED"};
  if(part==0)return {DRIVE_PWM,DRIVE_PWM,SWEEP_MS,lane,"SWEEP"};
  if(part==2)return {DRIVE_PWM,DRIVE_PWM,SHIFT_MS,lane,"SHIFT"};
  bool clockwise=lane%2==0;
  return {clockwise?TURN_PWM:-TURN_PWM,clockwise?-TURN_PWM:TURN_PWM,
          clockwise?TURN_CW_MS:TURN_CCW_MS,lane,part==1?"TURN_OUT":"TURN_IN"};
}
}
#endif
