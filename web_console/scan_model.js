(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.ScanModel=api;})(typeof globalThis!=='undefined'?globalThis:this,function(){
  const COLS=7,ROWS=41;
  function route(){const points=[];for(let c=0;c<COLS;c++){points.push([c*50,c%2?200:0],[c*50,c%2?0:200]);}return points;}
  function empty(id=0,source='real'){return {schema:1,source,runId:id,widthMm:300,heightMm:200,stepXmm:50,stepYmm:5,cols:COLS,rows:ROWS,origin:'DN3 start reference; x right, y up',sensorForwardOffsetMm:0,geometryCalibrated:false,state:'WAITING',reason:null,raw:Array.from({length:ROWS},()=>Array(COLS).fill(null)),counts:Array.from({length:ROWS},()=>Array(COLS).fill(0)),estimatedPositions:Array.from({length:ROWS},()=>Array(COLS).fill(null)),trajectory:[],rawReadings:[],createdAt:new Date().toISOString()};}
  function ingest(scan,line){const f=line.trim().split(',');if(f[0]!=='SCAN')return scan;
    if(['BEGIN','META'].includes(f[1])){const id=Number(f[2]);if(!Number.isInteger(id))return scan;if(f[1]==='BEGIN'||!scan||scan.runId!==id)scan=empty(id);scan.sensorForwardOffsetMm=Number(f[9]||0);return scan;}
    if(!scan)return scan;
    if(['CELL','STATE','END'].includes(f[1])&&Number(f[2])!==scan.runId)return scan;
    if(f[1]==='CELL'){const c=Number(f[3]),r=Number(f[4]),value=Number(f[5]),n=Number(f[6]),x=Number(f[7]),y=Number(f[8]);if(Number.isInteger(c)&&Number.isInteger(r)&&c>=0&&c<COLS&&r>=0&&r<ROWS&&[value,n,x,y].every(Number.isFinite)&&value>=0&&value<=65535&&n>0){scan.raw[r][c]=value;scan.counts[r][c]=n;scan.estimatedPositions[r][c]={xMm:x,yMm:y};}}
    if(f[1]==='STATE'){scan.state=f[3];scan.lane=Number(f[4]);}
    if(f[1]==='END'){scan.state=f[3]==='COMPLETE'?'COMPLETED':f[3];scan.reason=f[4];scan.finishedAt=new Date().toISOString();}
    return scan;
  }
  function telemetry(line){const f=line.trim().split(',');if(f.length!==14||!f.every(v=>v!==''&&Number.isFinite(Number(v))))return null;return {t:Number(f[0]),x:Number(f[1]),y:Number(f[2]),theta:Number(f[3]),left:Number(f[4]),right:Number(f[5]),dn3:Number(f[10]),signal:Number(f[13])};}
  function extent(scan){const vals=scan.raw.flat().filter(Number.isFinite);return vals.length?{min:Math.min(...vals),max:Math.max(...vals),filled:vals.length}:null;}
  function gray(value,scan,white=null,black=null,invert=false){if(value===null)return null;const e=extent(scan);if(!e)return null;let v;if(Number.isFinite(white)&&Number.isFinite(black)&&white!==black)v=255*(value-black)/(white-black);else v=e.max===e.min?128:255*(e.max-value)/(e.max-e.min);v=Math.round(Math.max(0,Math.min(255,v)));return invert?255-v:v;}
  function matrixCsv(scan){return ['y_mm/x_mm,'+Array.from({length:COLS},(_,c)=>c*50).join(','),...scan.raw.map((row,r)=>[r*5,...row.map(v=>v===null?'':v.toFixed(2))].join(','))].join('\n');}
  function rawCsv(scan){return ['timestamp_ms,x_est_mm,y_est_mm,theta_rad,DN3,left_pwm,right_pwm',...scan.rawReadings.map(t=>[t.t,t.x,t.y,t.theta,t.dn3,t.left,t.right].join(','))].join('\n');}
  function svg(scan){const cw=56,ch=8,w=500,h=410;let s='<svg xmlns="http://www.w3.org/2000/svg" width="'+w+'" height="'+h+'"><rect width="100%" height="100%" fill="#eee"/><g font-family="sans-serif" font-size="12" fill="#222"><text x="16" y="20">DN3 matrix: relative contrast, high raw = dark</text><text x="16" y="38">7 sample columns at 50 mm; y step 5 mm; pink = missing</text>';for(let r=0;r<ROWS;r++)for(let c=0;c<COLS;c++){let g=gray(scan.raw[r][c],scan);s+='<rect x="'+(65+c*cw)+'" y="'+(50+(ROWS-1-r)*ch)+'" width="'+(cw-1)+'" height="'+(ch-0.5)+'" fill="'+(g===null?'#ce6672':'rgb('+g+','+g+','+g+')')+'"/>';}for(let c=0;c<COLS;c++)s+='<text x="'+(65+c*cw)+'" y="397">'+(c*50)+'</text>';return s+'<text x="16" y="61">200</text><text x="30" y="375">0</text><text x="12" y="210">y mm</text><text x="435" y="397">x mm</text></g></svg>';}
  return {COLS,ROWS,route,empty,ingest,telemetry,extent,gray,matrixCsv,rawCsv,svg};
});
