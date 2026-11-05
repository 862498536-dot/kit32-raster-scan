// Processing 4: fixed-world-line tracking, matched to PathFollower.h.
import java.net.Socket;
import java.net.InetSocketAddress;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.concurrent.ConcurrentLinkedQueue;

final String ROBOT_IP = "192.168.4.1";
final int ROBOT_PORT = 80;
ArrayList<PVector> points = new ArrayList<PVector>();
ConcurrentLinkedQueue<String> incoming = new ConcurrentLinkedQueue<String>();
volatile Socket robotSocket;
volatile int connectionId = 0;
volatile boolean connecting = false;
volatile boolean connected = false;
volatile PrintWriter robotCommand;
volatile String connectionMessage = "Simulation ready. Press Start to run the robot model.";
String scanState = "IDLE";
boolean simulation = true, simRunning = false;
float simSpeed = 1, segmentX = 0, segmentY = 0, targetHeading = HALF_PI;
float simStageTime = 0, settleTime = 0;
String nextPhase = "";
int simLane = 0;
final float MIN_PWM=40, START_PWM=50, START_SECONDS=0.120;
final float LOOKAHEAD_MM=25, END_TOL_MM=1, CROSS_TOL_MM=3;
final float TURN_TOL_RAD=0.026179939, MAX_CROSS_MM=30, MAX_OVERSHOOT_MM=8;
float simulationTime=0, leftPowerSince=0, rightPowerSince=0, accumulator=0;
int leftPowerSign=0, rightPowerSign=0, stableHits=0;
long previousFrame = 0;
float robotX = 0, robotY = 0, robotTheta = HALF_PI;
float leftPWM = 0, rightPWM = 0;
boolean havePose = false;
long lastTelemetry = 0;

void settings() { size(1050, 760); }

void setup() {
  surface.setTitle("Kit32 Motion Twin - Simulation and Live");
  textFont(createFont("SansSerif", 16));
  frameRate(30);
}

void draw() {
  long now = System.currentTimeMillis();
  float elapsed = previousFrame == 0 ? 0 : min(0.1, (now-previousFrame)/1000.0);
  previousFrame = now;
  if(simulation && simRunning) {
    accumulator+=elapsed*simSpeed;
    while(accumulator>=0.01){simulateStep(0.01);accumulator-=0.01;}
  }
  for (int i=0; i<2000; i++) {
    String line = incoming.poll();
    if (line == null) break;
    acceptLine(line);
  }
  background(245, 247, 249);
  fill(28, 43, 54); textSize(25); text("Kit32 Motion Twin", 28, 40);
  drawButton(28,58,135,simulation ? "Mode: Simulation" : "Mode: Live");
  drawButton(173,58,135,connecting ? "Connecting..." : connected ? "Disconnect" : "Connect Live");
  drawButton(318,58,95,"Start");drawButton(423,58,95,"Stop");
  drawButton(528,58,125,"Clear Trail");drawButton(663,58,125,"Speed x"+nf(simSpeed,0,0));
  fill(65, 80, 90); textSize(13); text(connectionMessage, 28, 121);
  text(simulation ? "FIXED-LINE TRACKING | 7 lanes x 200 mm | 6 shifts x 50 mm | Position + heading correction" : "LIVE TELEMETRY | "+ROBOT_IP+":80 | Connect does not start the robot", 28, 145);
  drawTrajectory();
  fill(35, 55, 68); textSize(15);
  String fresh = simulation ? "SIMULATED" : lastTelemetry == 0 ? "Waiting for position" : System.currentTimeMillis()-lastTelemetry > 1500 ? "Telemetry stale" : "Receiving telemetry";
  text(scanState+"  |  "+fresh+"  |  "+points.size()+" trail points"+(simulation?"  |  Lane "+(simLane+1)+" / 7":""), 28, 685);
  text(String.format(java.util.Locale.ROOT, "x %.1f mm    y %.1f mm    Heading %.1f deg    Left / Right PWM %.0f / %.0f", robotX, robotY, degrees(robotTheta), leftPWM, rightPWM), 28, 714);
  fill(100, 112, 120); textSize(12);
  text(simulation ? "Unloaded motor measurements at PWM40/50; local linear estimate. Wheelbase 90 mm; ground behavior not calibrated." : "Encoder-estimated pose, not ground truth. Close other robot connections. Disconnect does not stop the robot.", 28, 741);
}

void drawButton(float x, float y, float w, String label) {
  noStroke(); fill(40, 111, 143); rect(x, y, w, 38, 5);
  fill(255); textSize(15); text(label, x+15, y+25);
}

void drawTrajectory() {
  float minX=0, maxX=300, minY=0, maxY=200;
  for(PVector point: points) {
    minX=min(minX, point.x); maxX=max(maxX, point.x);
    minY=min(minY, point.y); maxY=max(maxY, point.y);
  }
  if(havePose) { minX=min(minX,robotX);maxX=max(maxX,robotX);minY=min(minY,robotY);maxY=max(maxY,robotY); }
  minX-=20;maxX+=20;minY-=20;maxY+=20;
  float scale=min(900/(maxX-minX),450/(maxY-minY));
  float x0=70+(900-(maxX-minX)*scale)/2;
  float bottom=635-(450-(maxY-minY)*scale)/2;
  noStroke();fill(255);rect(28,165,994,490,7);
  stroke(223,229,233);strokeWeight(1);
  float grid=50;
  if(maxX-minX>1000||maxY-minY>1000)grid=200;
  for(float x=ceil(minX/grid)*grid;x<=maxX;x+=grid){
    float sx=x0+(x-minX)*scale;
    line(sx,bottom,sx,bottom-(maxY-minY)*scale);
    fill(100,115,125);textSize(11);text(nf(x,0,0),sx-10,650);
  }
  for(float y=ceil(minY/grid)*grid;y<=maxY;y+=grid){
    float sy=bottom-(y-minY)*scale;
    line(x0,sy,x0+(maxX-minX)*scale,sy);
    fill(100,115,125);textSize(11);text(nf(y,0,0),33,sy+4);
  }
  noFill();stroke(29,143,117);strokeWeight(2.5);
  beginShape();for(PVector point:points)vertex(x0+(point.x-minX)*scale,bottom-(point.y-minY)*scale);endShape();
  if(havePose){
    float rx=x0+(robotX-minX)*scale,ry=bottom-(robotY-minY)*scale;
    noStroke();fill(25,108,143);ellipse(rx,ry,16,16);
    stroke(25,108,143);strokeWeight(3);line(rx,ry,rx+24*cos(robotTheta),ry-24*sin(robotTheta));
  } else { fill(130,145,155);textSize(18);text(simulation ? "Press Start to simulate" : "Waiting for robot position",370,405); }
  fill(95,110,122);textSize(12);text("y ↑ (mm)",40,185);text("x → (mm)",940,648);
}

void mousePressed() {
  if(mouseY<58||mouseY>96)return;
  if(mouseX>=28&&mouseX<=163) {
    if(connected||connecting){connectionMessage="Disconnect Live before changing mode. Stop the robot first.";return;}
    simulation=!simulation;resetPose();
    connectionMessage=simulation?"Simulation ready. Press Start to run the robot model.":"Live mode. Join the robot Wi-Fi and click Connect Live.";
  }
  if(mouseX>=173&&mouseX<=308) {
    if(connected||connecting)disconnectRobot();
    else{if(simulation){resetPose();simulation=false;}connectRobot();}
  }
  if(mouseX>=318&&mouseX<=413) {
    if(simulation){resetPose();simRunning=true;havePose=true;enterPhase("SWEEP");points.add(new PVector(robotX,robotY));}
    else sendRobot("START");
  }
  if(mouseX>=423&&mouseX<=518) {
    if(simulation){simRunning=false;leftPWM=rightPWM=0;scanState="STOPPED";}
    else sendRobot("STOP");
  }
  if(mouseX>=528&&mouseX<=653)points.clear();
  if(mouseX>=663&&mouseX<=788&&simulation)simSpeed=simSpeed==1?2:simSpeed==2?4:1;
}

void resetPose(){
  points.clear();incoming.clear();simRunning=false;simLane=0;robotX=robotY=0;
  robotTheta=HALF_PI;leftPWM=rightPWM=0;havePose=false;lastTelemetry=0;scanState="IDLE";
  simulationTime=accumulator=0;leftPowerSign=rightPowerSign=stableHits=0;
}

void enterPhase(String phase){
  scanState=phase;simStageTime=0;stableHits=0;segmentX=robotX;segmentY=robotY;
  if(phase.equals("SWEEP"))targetHeading=simLane%2==0?HALF_PI:-HALF_PI;
  if(phase.equals("TURN_OUT")||phase.equals("SHIFT"))targetHeading=0;
  if(phase.equals("TURN_IN"))targetHeading=(simLane+1)%2==0?HALF_PI:-HALF_PI;
}

void settleBefore(String phase){leftPWM=rightPWM=0;leftPowerSign=rightPowerSign=0;scanState="SETTLING";nextPhase=phase;settleTime=0;}

// Same line reference, steering limits and power-floor rules as PathFollower.h.
float[] lineReference(boolean sweep, int lane, float x, float y){
  float direction=lane%2==0?1:-1;
  float cross=sweep?-direction*(x-lane*50):y-(lane%2==0?200:0);
  float remaining=sweep?(direction>0?200-y:y):(lane+1)*50-x;
  float nominal=sweep?direction*HALF_PI:0;
  float heading=nominal-constrain(atan2(cross,constrain(remaining,8,LOOKAHEAD_MM)),-PI/6,PI/6);
  return new float[]{cross,remaining,heading};
}

void applyMotorPower(){
  int ls=abs(leftPWM)<0.01?0:leftPWM>0?1:-1;
  int rs=abs(rightPWM)<0.01?0:rightPWM>0?1:-1;
  if(ls!=leftPowerSign){leftPowerSince=simulationTime;leftPowerSign=ls;}
  if(rs!=rightPowerSign){rightPowerSince=simulationTime;rightPowerSign=rs;}
  leftPWM=ls==0?0:ls*max(abs(leftPWM),simulationTime-leftPowerSince<START_SECONDS?START_PWM:MIN_PWM);
  rightPWM=rs==0?0:rs*max(abs(rightPWM),simulationTime-rightPowerSince<START_SECONDS?START_PWM:MIN_PWM);
}

// Oct 18 unloaded encoder rates, averaged over three trials per direction.
// Linear interpolation/extrapolation near PWM40/50; not a loaded motor model.
float wheelSpeed(float pwm, boolean left){
  if(abs(pwm)<0.01)return 0;
  float at40=left?(pwm>0?314.95:318.01):(pwm>0?343.94:356.94);
  float at50=left?(pwm>0?433.36:442.61):(pwm>0?465.09:473.26);
  float cps=max(0,at40+(abs(pwm)-40)*(at50-at40)/10);
  return (pwm>0?1:-1)*cps*TWO_PI*16/358.3;
}

void simulateStep(float dt){
  if(!simRunning)return;
  simulationTime+=dt;
  if(scanState.equals("SETTLING")){
    settleTime+=dt;if(settleTime>=0.2){if(nextPhase.equals("SWEEP"))simLane++;enterPhase(nextPhase);}return;
  }
  simStageTime+=dt;
  boolean turning=scanState.equals("TURN_OUT")||scanState.equals("TURN_IN");
  if(simStageTime>(turning?10:20)){simRunning=false;leftPWM=rightPWM=0;scanState="FAULT: SEGMENT_TIMEOUT";return;}
  float error=atan2(sin(targetHeading-robotTheta),cos(targetHeading-robotTheta));
  if(turning){
    if(abs(error)<TURN_TOL_RAD){
      leftPWM=rightPWM=0;leftPowerSign=rightPowerSign=0;
      if(++stableHits>=3)settleBefore(scanState.equals("TURN_OUT")?"SHIFT":"SWEEP");return;
    }
    stableHits=0;float pwm=error>0?40:-40;leftPWM=-pwm;rightPWM=pwm;
  }else{
    float[] reference=lineReference(scanState.equals("SWEEP"),simLane,robotX,robotY);
    float cross=reference[0],remaining=reference[1];
    if(abs(cross)>MAX_CROSS_MM||remaining < -MAX_OVERSHOOT_MM){simRunning=false;leftPWM=rightPWM=0;scanState="FAULT: PATH_ERROR";return;}
    if(remaining<=END_TOL_MM&&abs(cross)<=CROSS_TOL_MM){
      if(scanState.equals("SWEEP")&&simLane==6){simRunning=false;leftPWM=rightPWM=0;scanState="COMPLETED";return;}
      settleBefore(scanState.equals("SWEEP")?"TURN_OUT":"TURN_IN");return;
    }
    error=atan2(sin(reference[2]-robotTheta),cos(reference[2]-robotTheta));
    float correction=constrain(error*25,-12,12);
    float base=max(remaining<15?MIN_PWM:45,MIN_PWM+abs(correction));
    leftPWM=base-correction;rightPWM=base+correction;
  }
  applyMotorPower();
  float vl=wheelSpeed(leftPWM,true),vr=wheelSpeed(rightPWM,false);
  float distance=(vl+vr)*0.5*dt;
  robotX+=distance*cos(robotTheta);robotY+=distance*sin(robotTheta);
  robotTheta+=(vr-vl)/90*dt;
  if(points.isEmpty()||PVector.dist(points.get(points.size()-1),new PVector(robotX,robotY))>=0.3)points.add(new PVector(robotX,robotY));
}

void sendRobot(String command){
  PrintWriter writer=robotCommand;
  if(writer==null){connectionMessage="Not connected to the robot.";return;}
  if(command.equals("START")&&System.currentTimeMillis()-lastTelemetry>1500){connectionMessage="Wait for fresh telemetry before Start.";return;}
  writer.println(command);connectionMessage="Sent "+command;
}

void connectRobot() {
  if(connecting||connected)return;
  connecting=true; connectionMessage="Connecting to "+ROBOT_IP+"...";
  final int request=++connectionId;
  Thread worker=new Thread(new Runnable(){
    public void run(){
      Socket local=new Socket();
      try {
        local.connect(new InetSocketAddress(ROBOT_IP,ROBOT_PORT),3000);
        local.setTcpNoDelay(true);local.setSoTimeout(4000);
        if(request!=connectionId)return;
        robotSocket=local;connected=true;connecting=false;
        connectionMessage="Connected. Waiting for telemetry; robot not started.";
        PrintWriter command=new PrintWriter(local.getOutputStream(),true);
        robotCommand=command;
        command.println("PING");command.println("STATUS");
        BufferedReader reader=new BufferedReader(new InputStreamReader(local.getInputStream(),"UTF-8"));
        String line;
        while(request==connectionId&&(line=reader.readLine())!=null){
          if(incoming.size()<10000)incoming.offer(line);
        }
        if(request==connectionId)connectionMessage="Robot connection closed.";
      } catch(Exception ex) {
        if(request==connectionId)connectionMessage="Connection failed or timed out. Check robot Wi-Fi and power.";
      } finally {
        try{local.close();}catch(Exception ignored){}
        if(request==connectionId){connected=false;connecting=false;robotSocket=null;robotCommand=null;}
      }
    }
  });
  worker.setDaemon(true);worker.start();
}

void disconnectRobot() {
  connectionId++;connecting=false;connected=false;
  robotCommand=null;
  Socket socket=robotSocket;robotSocket=null;
  if(socket!=null)try{socket.close();}catch(Exception ignored){}
  incoming.clear();connectionMessage="Disconnected. Trail retained. Disconnect does not stop the robot.";
}

void acceptLine(String line) {
  String[] fields=split(trim(line), ',');
  if(fields.length>=2&&fields[0].equals("SCAN")){
    if(fields[1].equals("BEGIN")){points.clear();havePose=false;scanState="SCAN STARTED";}
    if(fields[1].equals("STATE")&&fields.length>=4)scanState=fields[3];
    if(fields[1].equals("END")&&fields.length>=5)scanState=fields[3]+" / "+fields[4];
    return;
  }
  if(fields.length!=14)return;
  float[] v=new float[14];
  try {
    for(int i=0;i<14;i++){v[i]=Float.parseFloat(trim(fields[i]));if(Float.isNaN(v[i])||Float.isInfinite(v[i]))return;}
  }catch(Exception ex){return;}
  robotX=v[1];robotY=v[2];robotTheta=v[3];leftPWM=v[4];rightPWM=v[5];
  havePose=true;lastTelemetry=System.currentTimeMillis();
  if(points.isEmpty()||PVector.dist(points.get(points.size()-1),new PVector(robotX,robotY))>=0.3){
    points.add(new PVector(robotX,robotY));if(points.size()>30000)points.remove(0);
  }
}

public void dispose(){disconnectRobot();super.dispose();}
