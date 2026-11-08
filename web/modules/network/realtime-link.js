// Persistent transport. Credentials travel only in the authenticated message body.
export class RealtimeLink {
 constructor({url,token,onState,onStatus,WebSocketImpl=globalThis.WebSocket,clock=()=>performance.now()}){
  Object.assign(this,{url,token,onState,onStatus,WebSocketImpl,clock});this.pending=new Map();this.inputs=new Map();this.serial=0;this.ready=false;
 }
 connect(){
  this.close();this.connecting=true;this.onStatus?.('connecting');
  const ws=this.socket=new this.WebSocketImpl(this.url);this.connectTimer=setTimeout(()=>{if(!this.ready)ws.close();},12000);
  ws.onopen=()=>ws.send(JSON.stringify({channel:'auth',id:'auth',token:this.token}));
  ws.onmessage=event=>{
   let message;try{message=JSON.parse(event.data);this.lastMessageAt=this.clock();}catch{return;}
   if(message._reply==='auth'){
    if(message.ok===false){ws.close();return;}
    clearTimeout(this.connectTimer);this.connecting=false;this.ready=true;this.lastMessageAt=this.clock();this.watchdog=setInterval(()=>{if(this.clock()-this.lastMessageAt>5000)ws.close();},1000);this.onStatus?.('ready');
   }
   const input=this.inputs.get(message._reply);let rtt;
   if(input!==undefined){rtt=(this.clock()-input)/1000;this.inputs.delete(message._reply);}
   const pending=this.pending.get(message._reply);
   if(pending){clearTimeout(pending.timer);this.pending.delete(message._reply);rtt=(this.clock()-pending.at)/1000;message.ok===false?pending.reject(Error(message.error)):pending.resolve(message);}
   if(message.player)this.onState?.(message,rtt);
  };
  ws.onerror=()=>{};
  ws.onclose=()=>{if(this.socket===ws){clearTimeout(this.connectTimer);clearInterval(this.watchdog);this.connecting=false;this.ready=false;this.rejectPending();this.onStatus?.('closed');}};
 }
 sendInput(body){
  if(!this.ready||this.socket.bufferedAmount>65536)return false;
  const id='i'+(++this.serial);this.inputs.set(id,this.clock());
  while(this.inputs.size>128)this.inputs.delete(this.inputs.keys().next().value);
  this.socket.send(JSON.stringify({channel:'input',id,body}));return true;
 }
 call(channel,body){
  if(!this.ready)return Promise.reject(Error('持續連線尚未就緒'));
  if(this.pending.size>=16||this.socket.bufferedAmount>65536)return Promise.reject(Error('連線忙碌，請稍後再試'));
  const id='r'+(++this.serial);
  return new Promise((resolve,reject)=>{
   const timer=setTimeout(()=>{this.pending.delete(id);reject(Error('連線回應逾時'));},8000);
   this.pending.set(id,{resolve,reject,timer,at:this.clock()});
   this.socket.send(JSON.stringify({channel,id,body}));
  });
 }
 rejectPending(){for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(Error('連線中斷'));}this.pending.clear();this.inputs.clear();}
 close(){clearInterval(this.watchdog);clearTimeout(this.connectTimer);clearInterval(this.watchdog);this.connecting=false;this.ready=false;this.rejectPending();if(this.socket){const old=this.socket;this.socket=null;old.onclose=null;old.onmessage=null;old.onopen=null;old.close();}}
}
