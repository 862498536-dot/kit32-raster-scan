// Local loopback-only bridge. One robot TCP connection is shared by browser tabs.
// Real measurements are saved under results/; no simulated readings enter this path.
const http=require('http'),net=require('net'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const model=require('./scan_model');
const PORT=Number(process.env.KIT32_WEB_PORT || 8766), DEFAULT_HOST=process.env.KIT32_HOST||'192.168.137.5';
const links=new Map(), outputs=path.join(__dirname,'results');
const origins=new Set(['http://127.0.0.1:8766','http://localhost:8766','http://127.0.0.1:8765']);
function frame(socket,text,opcode=1){if(socket.destroyed)return;const data=Buffer.isBuffer(text)?text:Buffer.from(text);let head;if(data.length<126)head=Buffer.from([0x80|opcode,data.length]);else{head=Buffer.alloc(4);head[0]=0x80|opcode;head[1]=126;head.writeUInt16BE(data.length,2);}socket.write(Buffer.concat([head,data]));}
function broadcast(link,text){for(const c of link.clients)frame(c,text);}
function persist(link){if(!link.scan||!model.extent(link.scan))return;fs.mkdirSync(outputs,{recursive:true});const stem=link.fileStem||('scan-'+Date.now());const s=link.scan;
 fs.writeFileSync(path.join(outputs,stem+'.json'),JSON.stringify(s,null,2));
 fs.writeFileSync(path.join(outputs,stem+'-matrix.csv'),model.matrixCsv(s));
 fs.writeFileSync(path.join(outputs,stem+'-raw.csv'),model.rawCsv(s));
 fs.writeFileSync(path.join(outputs,stem+'-relative.svg'),model.svg(s));
}
function ingest(link,chunk){link.pending+=chunk.toString('utf8');if(link.pending.length>16384)link.pending='';const lines=link.pending.split(/\r?\n/);link.pending=lines.pop();
 for(const line of lines){
  if(line.startsWith('SCAN,BEGIN,')){persist(link);link.fileStem='scan-'+new Date().toISOString().replace(/[:.]/g,'-')+'-'+crypto.randomBytes(3).toString('hex');}
  const prior=link.scan;link.scan=model.ingest(link.scan,line);
  if(!prior&&link.scan)link.fileStem='recovered-'+Date.now()+'-'+crypto.randomBytes(3).toString('hex');
  const t=model.telemetry(line);
  if(t&&link.scan&&t.signal===1){link.scan.rawReadings.push(t);link.scan.trajectory.push({x:t.x,y:t.y});}
  if(line.startsWith('SCAN,CELL,')||line.startsWith('SCAN,END,'))link.dirty=true;
  if(line.startsWith('SCAN,END,')||line==='SCAN,DUMP_END')persist(link);
 }
}
function connectRobot(host){let link=links.get(host);if(link&&!link.robot.destroyed)return link;
 const robot=net.createConnection({host,port:80});
 link={host,robot,clients:new Set(),pending:'',scan:null,ready:false,dirty:false};links.set(host,link);
 robot.setNoDelay(true);robot.setTimeout(6000);
 robot.on('connect',()=>{link.ready=true;broadcast(link,'BRIDGE,ROBOT_CONNECTED\n');robot.write('PING\nSTATUS\nDUMP\n');});
 robot.on('data',data=>{broadcast(link,data);ingest(link,data);});
 robot.on('timeout',()=>{broadcast(link,'BRIDGE,ROBOT_ERROR,TIMEOUT\n');robot.destroy();});
 robot.on('error',error=>broadcast(link,'BRIDGE,ROBOT_ERROR,'+error.code+'\n'));
 robot.on('close',()=>{link.ready=false;persist(link);for(const c of link.clients){frame(c,'BRIDGE,ROBOT_DISCONNECTED\n');frame(c,Buffer.alloc(0),8);c.end();}links.delete(host);});
 return link;
}
function parse(socket,link,state){while(state.bytes.length>=2){const b=state.bytes,opcode=b[0]&15;if(!(b[0]&128)||!(b[1]&128)){socket.destroy();return;}
 let len=b[1]&127,offset=2;if(len===127){socket.destroy();return;}if(len===126){if(b.length<4)return;len=b.readUInt16BE(2);offset=4;}if(len>512){socket.destroy();return;}if(b.length<offset+4+len)return;
 const mask=b.subarray(offset,offset+4);offset+=4;const body=Buffer.alloc(len);for(let i=0;i<len;i++)body[i]=b[offset+i]^mask[i%4];state.bytes=b.subarray(offset+len);
 if(opcode===8){frame(socket,Buffer.alloc(0),8);socket.end();return;}if(opcode===9){frame(socket,body,10);continue;}
 if(opcode!==1)continue;const cmd=body.toString();if(!/^(START|STOP|PING|DUMP|STATUS)\n$/.test(cmd)){frame(socket,'ERR,COMMAND\n');continue;}
 if(!link.ready){frame(socket,'ERR,ROBOT_NOT_READY\n');continue;}link.robot.write(cmd);
}}
const assets=new Map([['/','index.html'],['/index.html','index.html'],['/scan_model.js','scan_model.js'],['/scan_app.js','scan_app.js']]);
const server=http.createServer((req,res)=>{
 const uri=new URL(req.url,'http://127.0.0.1');
 if(uri.pathname==='/config'){if(origins.has(req.headers.origin))res.setHeader('Access-Control-Allow-Origin',req.headers.origin);res.writeHead(200,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify({host:DEFAULT_HOST}));return;}
 const file=assets.get(uri.pathname);if(!file){res.writeHead(404);res.end('Not found');return;}
 res.writeHead(200,{'Content-Type':file.endsWith('.js')?'text/javascript; charset=utf-8':'text/html; charset=utf-8','Cache-Control':'no-store'});fs.createReadStream(path.join(__dirname,file)).pipe(res);
});
server.on('upgrade',(req,socket,head)=>{
 const uri=new URL(req.url,'http://127.0.0.1'),host=uri.searchParams.get('host')||DEFAULT_HOST;
 const last=Number(host.split('.').pop());
 if(uri.pathname!=='/ws'||!req.headers['sec-websocket-key']||!origins.has(req.headers.origin)||!/^192\.168\.137\.\d{1,3}$/.test(host)||last<2||last>254){socket.destroy();return;}
 const key=crypto.createHash('sha1').update(req.headers['sec-websocket-key']+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
 socket.write('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+key+'\r\n\r\n');
 const link=connectRobot(host);link.clients.add(socket);const state={bytes:head||Buffer.alloc(0)};
 if(link.ready){frame(socket,'BRIDGE,ROBOT_CONNECTED\n');link.robot.write('STATUS\nDUMP\n');}
 socket.on('data',d=>{state.bytes=Buffer.concat([state.bytes,d]);parse(socket,link,state);});
 const detach=()=>{link.clients.delete(socket);if(!link.clients.size)persist(link);};socket.on('close',detach);socket.on('error',detach);
 if(head.length)parse(socket,link,state);
});
setInterval(()=>{for(const link of links.values())if(link.dirty){link.dirty=false;persist(link);}},2000).unref();
server.listen(PORT,'127.0.0.1',()=>console.log('Raster scan console: http://127.0.0.1:'+PORT+'/'));
module.exports={server,links};
