'use strict';
const M=window.ScanModel,$=id=>document.getElementById(id);
let socket=null,scan=null,latest=null,remainder='',lastData=0,lastSave=0,logs=[],demoMode=false;
const phases={WAITING:'待机',SWEEP:'纵向采样',TURN_OUT:'转向横移',SHIFT:'横移 50 mm',TURN_IN:'转回纵向',SETTLING:'停车稳定',COMPLETED:'扫描完成',ABORTED:'用户停止',FAULT:'故障停车'};
function log(text){logs.unshift(new Date().toLocaleTimeString()+'  '+text);$('log').textContent=logs.slice(0,20).join('\n');}
function labelSource(){const e=scan&&M.extent(scan);$('datasetLabel').textContent=!e?'尚无实测数据':scan.source==='simulation'?'仿真示例 · 非实测':scan.source==='import'?'导入记录':'实测记录';$('datasetLabel').className=scan?.source==='simulation'?'sim':'';}
function persist(){if(!scan||!M.extent(scan))return;if(Date.now()-lastSave<1000)return;lastSave=Date.now();try{localStorage.setItem('kit32-raster-last',JSON.stringify(scan));}catch(_){}}
function buttons(){const open=socket?.readyState===WebSocket.OPEN;$('connect').disabled=open;$('disconnect').disabled=!open;$('robotHost').disabled=open;$('stop').disabled=!open;$('dump').disabled=!open;$('start').disabled=!open||Date.now()-lastData>1500;$('demo').disabled=open;const has=!!(scan&&M.extent(scan));['json','csv','raw','graycsv','png'].forEach(id=>$(id).disabled=!has);}
function onLine(line){
 const t=M.telemetry(line);if(t){latest=t;lastData=Date.now();$('dn3').textContent=t.dn3;$('pwm').textContent=t.left+' / '+t.right;$('status').textContent='小车在线 · Wi-Fi';if(scan&&scan.source==='real'&&t.signal===1){scan.trajectory.push({x:t.x,y:t.y});scan.rawReadings.push(t);}buttons();draw();return;}
 if(line.startsWith('SCAN,')){
  if(line.startsWith('SCAN,BEGIN,'))demoMode=false;
  scan=M.ingest(scan,line);
  if(scan){$('phase').textContent=phases[scan.state]||scan.state;labelSource();if(line.startsWith('SCAN,END,'))log('本次扫描结束：'+scan.state+' / '+scan.reason);}
  if(!line.startsWith('SCAN,CELL,'))log(line);
  persist();draw();buttons();return;
 }
 if(line.startsWith('BRIDGE,ROBOT_ERROR')||line.startsWith('BRIDGE,ROBOT_DISCONNECTED')){$('status').textContent='小车断线；检查电源和 IP';lastData=0;}
 if(line.trim())log(line);buttons();
}
function connect(){if(socket&&[0,1].includes(socket.readyState))return;demoMode=false;remainder='';lastData=0;socket=new WebSocket('ws://127.0.0.1:8766/ws?host='+encodeURIComponent($('robotHost').value.trim()));
 socket.onopen=()=>{$('status').textContent='正在等待小车数据…';log('无线转接服务已连接');buttons();};
 socket.onmessage=e=>{remainder+=String(e.data);const lines=remainder.split(/\r?\n/);remainder=lines.pop();lines.forEach(onLine);};
 socket.onerror=()=>log('连接失败：检查本机服务、小车电源、电脑热点和 IP。');
 socket.onclose=()=>{socket=null;lastData=0;$('status').textContent='未连接小车';buttons();persist();};
}
function command(cmd){if(socket?.readyState!==WebSocket.OPEN)return;socket.send(cmd+'\n');log('已发送 '+cmd);}
function disconnect(){socket?.close();socket=null;lastData=0;buttons();$('status').textContent='已断开网页';}
function calibration(){const white=$('white').value===''?null:Number($('white').value),black=$('black').value===''?null:Number($('black').value);return {white,black,invert:$('invert').checked};}
function gray(v){const c=calibration();return M.gray(v,scan,c.white,c.black,c.invert);}
function axes(ctx,w,h,title){ctx.clearRect(0,0,w,h);ctx.fillStyle='#0e1920';ctx.fillRect(0,0,w,h);ctx.font='15px system-ui';ctx.fillStyle='#a5b6c3';ctx.fillText(title,55,24);ctx.fillText('y (mm)',8,48);ctx.fillText('x (mm)',w-75,h-12);}
function drawRoute(){const canvas=$('route'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height,L=60,T=50,W=w-100,H=h-105;axes(ctx,w,h,'计划：7 条 × 200 mm，横移 6 次 × 50 mm');const xy=(x,y)=>[L+x/300*W,T+(200-y)/200*H];
 ctx.strokeStyle='#283e4d';ctx.lineWidth=1;for(let x=0;x<=300;x+=50){const a=xy(x,0),b=xy(x,200);ctx.beginPath();ctx.moveTo(...a);ctx.lineTo(...b);ctx.stroke();ctx.fillText(x,L+x/300*W-10,h-35);}for(let y=0;y<=200;y+=50){const a=xy(0,y),b=xy(300,y);ctx.beginPath();ctx.moveTo(...a);ctx.lineTo(...b);ctx.stroke();ctx.fillText(y,20,T+(200-y)/200*H+5);}
 ctx.strokeStyle='#8399a9';ctx.setLineDash([6,5]);ctx.lineWidth=2;ctx.beginPath();M.route().forEach((p,i)=>i?ctx.lineTo(...xy(...p)):ctx.moveTo(...xy(...p)));ctx.stroke();ctx.setLineDash([]);
 const track=scan?.trajectory||[];if(track.length){ctx.strokeStyle=scan.source==='simulation'?'#e9b95b':'#6ad4b7';ctx.lineWidth=2;ctx.beginPath();track.forEach((p,i)=>i?ctx.lineTo(...xy(p.x,p.y)):ctx.moveTo(...xy(p.x,p.y)));ctx.stroke();const p=track[track.length-1];ctx.fillStyle=ctx.strokeStyle;ctx.beginPath();ctx.arc(...xy(p.x,p.y),5,0,Math.PI*2);ctx.fill();}
 ctx.fillStyle='#6ad4b7';ctx.fillText('起点 (0, 0)，车头朝上',L,h-10);
}
function drawMatrix(){const canvas=$('matrix'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height,L=60,T=50,W=w-100,H=h-105,cw=W/7,ch=H/41;axes(ctx,w,h,'采样列 x = 0, 50, 100, …, 300 mm');
 for(let r=0;r<41;r++)for(let c=0;c<7;c++){const value=scan?scan.raw[r][c]:null,g=scan?gray(value):null;ctx.fillStyle=g===null?'#483641':'rgb('+g+','+g+','+g+')';ctx.fillRect(L+c*cw,T+(40-r)*ch,cw-1,ch-.3);}
 ctx.fillStyle='#a5b6c3';for(let c=0;c<7;c++)ctx.fillText(c*50,L+(c+.5)*cw-12,h-35);for(let r=0;r<=40;r+=10)ctx.fillText(r*5,20,T+(40-r+.5)*ch+5);
 const e=scan&&M.extent(scan);$('filled').textContent=e?e.filled:0;const cal=calibration();$('scaleInfo').textContent=Number.isFinite(cal.white)&&Number.isFinite(cal.black)&&cal.white!==cal.black?'按用户输入的黑/白样本映射到 0–255；0 为黑，255 为白。':'未校准：相对最小/最大值拉伸，高读数显示更暗。不是绝对光照强度。'+(e?' 本次原始范围 '+e.min.toFixed(1)+'–'+e.max.toFixed(1):'');
 $('matrixText').textContent=e?M.matrixCsv(scan):'尚无测量；不会显示伪造的零矩阵。';
}
function draw(){drawRoute();drawMatrix();labelSource();}
function download(name,content,type){const a=document.createElement('a');const url=URL.createObjectURL(new Blob([content],{type}));a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function filename(ext){return 'DN3-'+(scan?.source||'real')+'-run'+(scan?.runId||0)+'.'+ext;}
$('connect').onclick=connect;$('disconnect').onclick=disconnect;$('start').onclick=()=>command('START');$('stop').onclick=()=>command('STOP');$('dump').onclick=()=>command('DUMP');
['white','black','invert'].forEach(id=>$(id).addEventListener('input',draw));
$('json').onclick=()=>{scan.visualisation=calibration();download(filename('json'),JSON.stringify(scan,null,2),'application/json');};
$('csv').onclick=()=>download(filename('csv'),M.matrixCsv(scan),'text/csv;charset=utf-8');
$('raw').onclick=()=>download(filename('raw.csv'),M.rawCsv(scan),'text/csv;charset=utf-8');
$('graycsv').onclick=()=>download(filename('gray.csv'),['y_mm/x_mm,0,50,100,150,200,250,300',...scan.raw.map((row,r)=>[r*5,...row.map(v=>v===null?'':gray(v))].join(','))].join('\n'),'text/csv;charset=utf-8');
$('png').onclick=()=>{const canvas=document.createElement('canvas');canvas.width=720;canvas.height=610;const ctx=canvas.getContext('2d');ctx.fillStyle='#0e1920';ctx.fillRect(0,0,720,610);ctx.drawImage($('matrix'),0,0);ctx.fillStyle='#e7eef4';ctx.font='16px system-ui';ctx.fillText((scan.source==='simulation'?'仿真示例 / 非实测':'DN3 测量数据')+' · run '+scan.runId,50,548);ctx.font='13px system-ui';ctx.fillText('采样列间距 50 mm，纵向 5 mm；粉色 = 缺测；编码器坐标未校准',50,575);canvas.toBlob(blob=>{const a=document.createElement('a'),url=URL.createObjectURL(blob);a.href=url;a.download=filename('png');a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);});};
$('matrix').addEventListener('mousemove',e=>{const box=e.currentTarget.getBoundingClientRect(),x=(e.clientX-box.left)*720/box.width,y=(e.clientY-box.top)*520/box.height,c=Math.floor((x-60)/(620/7)),r=40-Math.floor((y-50)/(415/41));if(c<0||c>6||r<0||r>40||!scan)return;const v=scan.raw[r][c],p=scan.estimatedPositions[r][c];$('cellInfo').textContent='名义坐标 ('+(c*50)+', '+(r*5)+') mm · '+(v===null?'未采样':'原始均值 '+v.toFixed(2)+' · n='+scan.counts[r][c]+(p?' · 估计位置 ('+p.xMm.toFixed(1)+', '+p.yMm.toFixed(1)+') mm':''));});
$('demo').onclick=()=>{if(socket)return;scan=M.empty(9001,'simulation');for(let c=0;c<7;c++)for(let r=0;r<41;r++){const x=c*50,y=r*5,dark=(x>=50&&x<=150&&y>=40&&y<=110)||((x-240)**2+(y-145)**2<38**2);scan.raw[r][c]=dark?2300:600;scan.counts[r][c]=1;scan.estimatedPositions[r][c]={xMm:x,yMm:y};}scan.trajectory=M.route().map(([x,y])=>({x,y}));scan.state='COMPLETED';demoMode=true;$('phase').textContent='仿真完成';$('status').textContent='仿真示例，未控制小车';log('加载合成矩形和圆形，只用于验证显示与导出，不是实测。');draw();buttons();};
$('import').onchange=async e=>{if(socket){log('请先断开连接，再导入历史记录。');return;}try{const parsed=JSON.parse(await e.target.files[0].text());if(parsed.rows!==41||parsed.cols!==7||!Array.isArray(parsed.raw)||parsed.raw.length!==41||parsed.raw.some(row=>!Array.isArray(row)||row.length!==7||row.some(v=>v!==null&&(!Number.isFinite(v)||v<0||v>65535))))throw Error('矩阵尺寸或读数不合法');parsed.counts=parsed.counts||M.empty().counts;parsed.estimatedPositions=parsed.estimatedPositions||M.empty().estimatedPositions;parsed.trajectory=Array.isArray(parsed.trajectory)?parsed.trajectory.filter(p=>Number.isFinite(p.x)&&Number.isFinite(p.y)):[];parsed.rawReadings=parsed.rawReadings||[];parsed.source=parsed.source==='simulation'?'simulation':'import';scan=parsed;$('phase').textContent='导入记录';draw();buttons();}catch(error){log('导入失败：'+error.message);}};
setInterval(()=>{if(socket?.readyState===WebSocket.OPEN&&lastData&&Date.now()-lastData>1500){$('status').textContent='读数已过期，请检查小车连接';buttons();}persist();},1000);
fetch('/config').then(r=>r.json()).then(c=>{$('robotHost').value=c.host;}).catch(()=>{});
draw();buttons();
