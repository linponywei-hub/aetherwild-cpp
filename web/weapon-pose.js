export function weaponPose(kind,aspect=16/9){
 const edge=Math.min(.65,.65*Math.max(.65,aspect/(16/9)));
 return kind==='daggers'?{left:[-edge,-.38,-.72],right:[edge,-.38,-.72]}:{right:[.38,-.48,-.65]};
}
// Preserve draw/release/cancel order even when an HTTP draw ACK arrives late.
export class BowInput {
 constructor(send){this.send=send;this.held=false;this.active=false;this.generation=0;this.tail=Promise.resolve();this.open=false;}
 begin(){if(this.held)return this.pending;this.held=true;this.open=true;const ticket=this.generation;
  this.pending=this.tail.then(()=>this.send('bow_draw')).then(ok=>{const valid=!!ok&&ticket===this.generation;this.active=valid&&this.held;return valid;});this.tail=this.pending.catch(()=>false);return this.pending;}
 release(extra={}){if(!this.held)return;this.held=false;const ticket=this.generation,draw=this.pending;
  const task=this.tail.then(async()=>{if(!await draw||ticket!==this.generation)return false;this.active=false;const ok=await this.send('bow_release',extra);if(!ok)await this.send('bow_cancel');if(ticket===this.generation)this.open=false;return ok;});this.tail=task.catch(()=>false);return task;}
 cancel(){const drawing=this.open;this.open=false;this.held=this.active=false;this.generation++;if(drawing){this.tail=this.tail.then(()=>this.send('bow_cancel'));return this.tail;}}
}
