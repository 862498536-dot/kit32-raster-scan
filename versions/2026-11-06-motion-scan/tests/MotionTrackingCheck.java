public class MotionTrackingCheck {
  static void require(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
  static void run(float disturbance){
    Kit32MotionTwin s=new Kit32MotionTwin();s.resetPose();s.simRunning=true;s.enterPhase("SWEEP");
    boolean injected=false;int ticks=0;float maxEndpoint=0;
    while(s.simRunning&&ticks++<30000){
      if(!injected&&disturbance!=0&&s.simLane==1&&s.scanState.equals("SWEEP")&&s.simStageTime>.25f){s.robotX+=disturbance;injected=true;}
      String before=s.scanState;s.simulateStep(.01f);
      require(s.leftPWM==0||Math.abs(s.leftPWM)>=40,"left dead zone");
      require(s.rightPWM==0||Math.abs(s.rightPWM)>=40,"right dead zone");
      if(before.equals("SWEEP")&&(s.scanState.equals("SETTLING")||s.scanState.equals("COMPLETED")))maxEndpoint=Math.max(maxEndpoint,Math.abs(s.robotX-s.simLane*50));
    }
    require(s.scanState.equals("COMPLETED"),"run "+disturbance+" ended "+s.scanState+" at "+s.robotX+","+s.robotY);
    require(s.simLane==6,"seven lanes");require(maxEndpoint<=3.01f,"end cross-track tolerance");
    require(Math.abs(s.robotX-300)<=3&&Math.abs(s.robotY-200)<=2,"final endpoint");
    System.out.println("PASS disturbance="+disturbance+" mm; final="+s.robotX+","+s.robotY+"; max lane-end cross="+maxEndpoint+" mm; all nonzero PWM >=40");
  }
  public static void main(String[] args){
    run(0);run(8);run(-8);
    Kit32MotionTwin s=new Kit32MotionTwin();
    float[] a=s.lineReference(true,0,8,40);require(a[2]>Math.PI/2,"upward lane: steer left from right of line");
    float[] b=s.lineReference(true,1,58,160);require(b[2]<-Math.PI/2,"downward lane: steer right in heading coordinates");
    s.resetPose();s.simRunning=true;s.enterPhase("SWEEP");s.robotX=40;s.simulateStep(.01f);
    require(s.scanState.equals("FAULT: PATH_ERROR")&&s.leftPWM==0&&s.rightPWM==0,"path fault stops both motors");
    s.acceptLine("SCAN,BEGIN,42,300,200,50,5,7,41,0");s.acceptLine("10,2,3,1.57,45,44,1,1,500,500,700,500,500,1");
    require(s.robotX==2&&s.robotY==3&&s.havePose,"live telemetry retained");
    System.out.println("PASS steering directions, excess path error stop, live telemetry.");
  }
}
