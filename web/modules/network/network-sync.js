// Transport-only ordering: never change gameplay values or calculate positions here.
export class SnapshotOrder {
  constructor(){this.reset();}
  reset(){this.epoch=null;this.sequence=-1;this.time=-Infinity;this.player=null;this.retired=new Set();}
  accept(snapshot){
    const id=snapshot.player?.id;
    if(this.player!==id){this.reset();this.player=id;}
    const sync=snapshot._sync;
    if(sync){
      if(this.retired.has(sync.epoch))return false;
      if(this.epoch!==sync.epoch){
        if(this.epoch!==null)this.retired.add(this.epoch);
        this.epoch=sync.epoch;this.sequence=-1;this.time=-Infinity;
      }
      if(sync.sequence<=this.sequence)return false;
      this.sequence=sync.sequence;
    }else if(Number.isFinite(snapshot.now)&&snapshot.now<this.time)return false;
    if(Number.isFinite(snapshot.now))this.time=snapshot.now;
    return true;
  }
}
