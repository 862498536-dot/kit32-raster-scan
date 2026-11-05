#ifndef KIT32_PATH_FOLLOWER_H
#define KIT32_PATH_FOLLOWER_H
#include <math.h>

// Mirrored in Kit32MotionTwin.pde. Nominal geometry remains uncalibrated.
namespace PathFollower {
constexpr float MIN_PWM = 40.0f;
constexpr float START_PWM = 50.0f;
constexpr unsigned long START_MS = 120;
constexpr float TURN_PWM = 40.0f;
constexpr float LOOKAHEAD_MM = 25.0f;
constexpr float END_TOL_MM = 1.0f;
constexpr float CROSS_TOL_MM = 3.0f;
constexpr float TURN_TOL_RAD = 0.026179939f; // 1.5 degrees
constexpr float MAX_CROSS_MM = 30.0f;
constexpr float MAX_OVERSHOOT_MM = 8.0f;
inline float limit(float v, float lo, float hi) { return fmaxf(lo, fminf(hi, v)); }
inline float wrap(float v) { return atan2f(sinf(v), cosf(v)); }
struct Line {
  float cross, remaining, heading;
};
inline Line reference(bool sweep, unsigned lane, float x, float y) {
  const float direction = lane % 2 == 0 ? 1.0f : -1.0f;
  Line line;
  line.cross = sweep ? -direction * (x - lane * 50.0f)
                     : y - (lane % 2 == 0 ? 200.0f : 0.0f);
  line.remaining = sweep ? (direction > 0 ? 200.0f-y : y)
                         : (lane+1)*50.0f-x;
  const float nominal = sweep ? direction*1.570796327f : 0.0f;
  const float lookahead = limit(line.remaining, 8.0f, LOOKAHEAD_MM);
  line.heading = nominal - limit(atan2f(line.cross, lookahead), -0.523598776f, 0.523598776f);
  return line;
}
inline void drive(float error, float remaining, float &left, float &right) {
  const float correction = limit(error*25.0f, -12.0f, 12.0f);
  // Raise common-mode power instead of letting either wheel enter its dead zone.
  const float base = fmaxf(remaining < 15.0f ? MIN_PWM : 45.0f,
                           MIN_PWM + fabsf(correction));
  left=base-correction; right=base+correction;
}
inline float powered(float demand, int &lastSign, unsigned long &since, unsigned long now) {
  if (fabsf(demand)<0.01f) { lastSign=0; return 0; }
  const int sign=demand>0?1:-1;
  if (sign!=lastSign) { lastSign=sign; since=now; }
  const float floor = now-since < START_MS ? START_PWM : MIN_PWM;
  return sign*fmaxf(floor, fabsf(demand));
}
}
#endif
