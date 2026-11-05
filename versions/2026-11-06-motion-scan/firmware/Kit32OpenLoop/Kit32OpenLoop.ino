#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string.h>
#include "Robot.h"
#include "RobotNetwork.h"
#include "OdometryModel.h"
#include "OpenLoopPlan.h"

// INDEPENDENT OPEN-LOOP EXPERIMENT. Motors obey PWM + duration ONLY.
// Encoders are read for display/logging, never for changing PWM or stage timing.
Robot_c robot;
OdometryModel_c odometry;
bool running=false,settling=false,stopping=false,dumping=false,lastButton=false;
uint32_t runId=0,stageSince=0,lastControl=0,lastTelemetry=0,lastSurface=0,lastDump=0;
unsigned moveIndex=0,dumpIndex=0;
const char* state="WAITING";
uint32_t sums[7][41];
uint16_t counts[7][41];
float positionX[7][41],positionY[7][41];
int lastSampleRow=-1;
char command[24];unsigned commandLength=0;
bool discardCommand=false;

void emit(const char* line){Serial.println(line);server.sendLine(line);}
bool active(){return running;}

void metadata(bool begin){
  char line[100];snprintf(line,sizeof(line),"SCAN,%s,%lu,300,200,50,5,7,41,0.0",begin?"BEGIN":"META",(unsigned long)runId);emit(line);
  emit("INFO,OPEN_LOOP,TIME_INDEXED_MAP,ENCODERS_FOR_DISPLAY_ONLY");
}
void status(){
  metadata(false);char line[100];snprintf(line,sizeof(line),"SCAN,STATE,%lu,%s,%u",(unsigned long)runId,state,OpenLoopPlan::get(moveIndex).lane);emit(line);
}
void publishCell(unsigned lane,unsigned row){
  unsigned n=counts[lane][row];if(!n)return;
  char line[160];snprintf(line,sizeof(line),"SCAN,CELL,%lu,%u,%u,%.2f,%u,%.2f,%.2f",(unsigned long)runId,lane,row,float(sums[lane][row])/n,n,positionX[lane][row]/n,positionY[lane][row]/n);emit(line);
}
void flushCell(){
  if(lastSampleRow>=0){publishCell(OpenLoopPlan::get(moveIndex).lane,lastSampleRow);lastSampleRow=-1;}
}
void endRun(const char* endState,const char* reason){
  flushCell();running=false;settling=false;state=endState;
  stopping=!robot.setMotorPWM(0,0);
  char line[120];snprintf(line,sizeof(line),"SCAN,END,%lu,%s,%s",(unsigned long)runId,strcmp(endState,"COMPLETED")==0?"COMPLETE":endState,reason);emit(line);
}
bool motor(int left,int right){
  if(robot.setMotorPWM(left,right))return true;
  endRun("FAULT","MOTOR_I2C");return false;
}
void enterMove(uint32_t now){
  OpenLoopPlan::Move move=OpenLoopPlan::get(moveIndex);
  stageSince=now;settling=false;lastSampleRow=-1;state=move.name;
  if(!motor(move.left,move.right))return;
  char line[100];snprintf(line,sizeof(line),"SCAN,STATE,%lu,%s,%u",(unsigned long)runId,state,move.lane);emit(line);
}
void startRun(){
  if(running||stopping){emit("ERR,START_BUSY");return;}
  if(!motor(0,0))return;
  memset(sums,0,sizeof(sums));memset(counts,0,sizeof(counts));
  memset(positionX,0,sizeof(positionX));memset(positionY,0,sizeof(positionY));
  moveIndex=0;lastSampleRow=-1;dumping=false;runId++;
  odometry.reset(0,0,HALF_PI);
  if(robot.getEncoders())odometry.update(robot.getLeftEncoderCount(),robot.getRightEncoderCount());
  running=true;lastSurface=millis();metadata(true);emit("ACK,START");enterMove(millis());
}
void processCommand(){
  command[commandLength]=0;
  if(strcmp(command,"START")==0)startRun();
  else if(strcmp(command,"STOP")==0){endRun("ABORTED","USER_STOP");emit("ACK,STOP");}
  else if(strcmp(command,"PING")==0)emit("ACK,PING");
  else if(strcmp(command,"STATUS")==0)status();
  else if(strcmp(command,"DUMP")==0){
    if(running)emit("ERR,DUMP_BUSY");
    else{dumping=true;dumpIndex=0;metadata(false);emit("SCAN,DUMP_BEGIN");}
  }else if(commandLength)emit("ERR,UNKNOWN_COMMAND");
  commandLength=0;
}
void acceptByte(char c){
  if(c=='\r')return;
  if(c=='\n'){if(!discardCommand)processCommand();commandLength=0;discardCommand=false;return;}
  if(discardCommand)return;
  if(commandLength<sizeof(command)-1)command[commandLength++]=c;
  else{commandLength=0;discardCommand=true;}
}
void sampleSurface(uint32_t now){
  if(!running||settling||moveIndex%4!=0)return;
  OpenLoopPlan::Move move=OpenLoopPlan::get(moveIndex);
  // Nominal map row comes ONLY from elapsed time, NOT odometry feedback.
  // Reported per-cell positions remain encoder estimates for later comparison.
  float fraction=fminf(1.0f,float(uint32_t(now-stageSince))/move.duration);
  float nominalY=move.lane%2==0?200*fraction:200*(1-fraction);
  int row=int(lroundf(nominalY/5));if(row<0||row>40)return;
  if(lastSampleRow>=0&&row!=lastSampleRow)publishCell(move.lane,lastSampleRow);
  lastSampleRow=row;
  unsigned n=counts[move.lane][row];if(n>=60000)return;
  sums[move.lane][row]+=robot.surface.reading[2];counts[move.lane][row]++;
  positionX[move.lane][row]+=odometry.pose.x;positionY[move.lane][row]+=odometry.pose.y;
}
void telemetry(uint32_t now){
  char line[230];snprintf(line,sizeof(line),"%lu,%.2f,%.2f,%.5f,%d,%d,%ld,%ld,%u,%u,%u,%u,%u,%u",(unsigned long)now,odometry.pose.x,odometry.pose.y,odometry.pose.theta,robot.getLeftMotorPWM(),robot.getRightMotorPWM(),(long)robot.getLeftEncoderCount(),(long)robot.getRightEncoderCount(),robot.surface.reading[0],robot.surface.reading[1],robot.surface.reading[2],robot.surface.reading[3],robot.surface.reading[4],running?1:0);emit(line);
}
void setup(){
  Serial.begin(115200);robot.initialise();stopping=!robot.setMotorPWM(0,0);
  if(server.begin())Serial.printf("INFO,WIFI_AP,%s\n",server.ip().toString().c_str());
  else Serial.println("ERROR,WIFI_SETUP_FAILED");
  emit("INFO,OPEN_LOOP,PWM_AND_TIME_ONLY,NO_PATH_OR_HEADING_CORRECTION");
}
void loop(){
  server.update();
  while(Serial.available())acceptByte(char(Serial.read()));
  while(server.available())acceptByte(char(server.read()));
  uint32_t now=millis();if(uint32_t(now-lastControl)<10)return;lastControl=now;
  if(stopping)stopping=!robot.setMotorPWM(0,0);
  bool button=digitalRead(3)==LOW;
  if(button&&!lastButton){if(running)endRun("ABORTED","BUTTON_STOP");else startRun();}
  lastButton=button;
  // Pose estimates are OUTPUT ONLY. They cannot influence scheduled motion.
  bool encoderOk=robot.getEncoders();
  if(encoderOk)odometry.update(robot.getLeftEncoderCount(),robot.getRightEncoderCount());
  bool surfaceOk=robot.getSurfaceSensors();
  // startRun() may have run from the button after the previous now snapshot.
  // Refresh before unsigned elapsed comparisons to avoid an apparent wrap.
  now=millis();if(surfaceOk)lastSurface=now;
  if(surfaceOk)sampleSurface(now);
  if(running&&uint32_t(now-lastSurface)>500)endRun("FAULT","SURFACE_READ");
  if(running){
    if(settling){
      if(uint32_t(now-stageSince)>=OpenLoopPlan::SETTLE_MS){moveIndex++;enterMove(now);}
    }else if(uint32_t(now-stageSince)>=OpenLoopPlan::get(moveIndex).duration){
      flushCell();
      if(moveIndex+1==OpenLoopPlan::MOVES)endRun("COMPLETED","TIMED_PLAN_FINISHED");
      else if(motor(0,0)){settling=true;stageSince=now;state="SETTLING";}
    }
  }
  if(uint32_t(now-lastTelemetry)>=50){lastTelemetry=now;telemetry(now);}
  if(dumping&&uint32_t(now-lastDump)>=10){
    lastDump=now;
    if(dumpIndex<287){publishCell(dumpIndex/41,dumpIndex%41);dumpIndex++;}
    else{dumping=false;status();emit("SCAN,DUMP_END");}
  }
}
