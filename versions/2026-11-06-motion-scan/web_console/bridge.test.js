'use strict';
// Uses a synthetic local TCP peer, never the real robot. Generated files are
// captured in memory to check persistence without mixing test and real data.
const assert=require('node:assert/strict'),fs=require('fs'),net=require('net'),http=require('http'),vm=require('vm'),path=require('path'),crypto=require('crypto');
const {once}=require('events');
async function main(){
 const written=new Map(),commands=[];const peers=new Set();
 const fake=net.createServer(s=>{peers.add(s);s.on('close',()=>peers.delete(s));let pending='';s.on('data',d=>{pending+=d;let lines=pending.split('\n');pending=lines.pop();for(const line of lines){commands.push(line);if(line==='PING')s.write('ACK,PING\n');if(line==='STATUS')s.write('SCAN,META,12,300,200,50,5,7,41,0\nSCAN,STATE,12,WAITING,0\n');if(line==='START')s.write('ACK,START\nSCAN,BEGIN,13,300,200,50,5,7,41,0\n10,0,0,1.5708,45,45,1,1,500,500,700,500,500,1\nSCAN,CELL,13,0,0,700,2,0,0\nSCAN,CELL,13,1,40,2400,4,50,200\nSCAN,END,13,COMPLETE,OK\n');}});});
 fake.listen(0,'127.0.0.1');await once(fake,'listening');
 const fakePort=fake.address().port,sandbox={module:{exports:{}},__dirname,Buffer,URL,console:{log(){}},setInterval,process:{env:{KIT32_WEB_PORT:'0'}},require:name=>{if(name==='net')return {...net,createConnection:options=>{assert.equal(options.host,'192.168.4.1');assert.equal(options.port,80);return net.createConnection({host:'127.0.0.1',port:fakePort});}};if(name==='fs')return {...fs,mkdirSync(){},writeFileSync:(p,value)=>written.set(path.basename(p),value)};return require(name);}};
 vm.runInNewContext(fs.readFileSync(path.join(__dirname,'wifi_bridge.js'),'utf8'),sandbox);
 const {server,links}=sandbox.module.exports;await once(server,'listening');const port=server.address().port;
 const clients=[];try{
  const config=await new Promise((resolve,reject)=>http.get('http://127.0.0.1:'+port+'/config',res=>{let data='';res.on('data',d=>data+=d);res.on('end',()=>resolve(JSON.parse(data)));}).on('error',reject));assert.equal(config.host,'192.168.4.1');
  const client=net.createConnection({host:'127.0.0.1',port});clients.push(client);await once(client,'connect');let response='';client.on('data',d=>response+=d.toString());
  client.write('GET /ws?host=192.168.4.1 HTTP/1.1\r\nHost: 127.0.0.1\r\nOrigin: http://127.0.0.1:8766\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: '+crypto.randomBytes(16).toString('base64')+'\r\nSec-WebSocket-Version: 13\r\n\r\n');
  async function until(predicate){const deadline=Date.now()+2000;while(!predicate()){if(Date.now()>deadline)throw Error('Test timeout');await new Promise(r=>setTimeout(r,10));}}
  await until(()=>response.includes('ACK,PING'));assert(response.includes('101 Switching Protocols'));assert(commands.includes('STATUS'));assert(commands.includes('DUMP'));
  const payload=Buffer.from('START\n'),mask=Buffer.from([1,2,3,4]);const frame=Buffer.concat([Buffer.from([0x81,0x80|payload.length]),mask,Buffer.from(payload.map((v,i)=>v^mask[i%4]))]);
  client.write(frame.subarray(0,3));client.write(frame.subarray(3));await until(()=>response.includes('SCAN,END,13,COMPLETE,OK'));
  await until(()=>[...written.keys()].some(k=>k.endsWith('-matrix.csv')));
  const saved=[...written].find(([k])=>k.endsWith('.json')&&k.startsWith('scan-'));assert(saved);const record=JSON.parse(saved[1]);assert.equal(record.source,'real');assert.equal(record.raw[0][0],700);assert.equal(record.raw[40][1],2400);assert.equal(record.raw[1][0],null);assert.equal(record.rawReadings[0].dn3,700);assert.equal(record.state,'COMPLETED');
  assert([...written.keys()].some(k=>k.endsWith('-relative.svg')));assert([...written.keys()].some(k=>k.endsWith('-raw.csv')));
  console.log('PASS: local fake peer -> fragmented WebSocket command -> DN3 matrix -> JSON/CSV/SVG persistence; no real-robot access.');
 }finally{for(const c of clients)c.destroy();for(const link of links.values())link.robot.destroy();for(const p of peers)p.destroy();await new Promise(r=>server.close(r));await new Promise(r=>fake.close(r));}
}
main().catch(e=>{console.error(e);process.exitCode=1;});
