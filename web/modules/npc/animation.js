// Rendering follows server positions; animations never translate the gameplay root.
export function npcClip(clips, mode){
 const patterns={walk:/^walk(ing)?([_ .]|$)/i,talk:/talk|speak|wave|hello|greet|interact|^Yes$/i,idle:/^idle([_ .]|$)|standing|unarmed_idle/i};
 return clips.find(c=>(patterns[mode]||patterns.idle).test(c.name))||clips.find(c=>patterns.idle.test(c.name));
}
export function updateNpcVisual(npc,time,dt,clock){
 const pose=npc.userData.track?.at(time),life=npc.userData.npcLife||{};
 let speed=0,desired=life.yaw;
 if(pose){const dx=pose.x-npc.position.x,dz=pose.z-npc.position.z;speed=Math.hypot(dx,dz)/Math.max(dt,.001);if(speed>.03)desired=Math.atan2(dx,dz);npc.position.set(pose.x,pose.y,pose.z);}
 if(life.mode==='talk')desired=life.yaw;
 if(Number.isFinite(desired)){const delta=Math.atan2(Math.sin(desired-npc.rotation.y),Math.cos(desired-npc.rotation.y));npc.rotation.y+=Math.max(-dt*3,Math.min(dt*3,delta));}
 const moving=speed>.04,mode=life.mode==='talk'?'talk':moving?'walk':'idle';
 const mixer=npc.userData.modelMixer,clips=npc.userData.modelClips||[],clip=npcClip(clips,mode);
 if(mixer&&clip){
  const next=mixer.clipAction(clip);if(npc.userData.activeAction!==next){npc.userData.activeAction?.fadeOut(.25);next.reset().setEffectiveWeight(1).fadeIn(.25).play();npc.userData.activeAction=next;}
  next.setEffectiveTimeScale(mode==='walk'?Math.max(.65,Math.min(1.4,speed/.9)):1);mixer.update(dt);
 }else for(const [i,leg]of (npc.userData.legs||[]).entries())leg.rotation.x=moving?Math.sin(clock*7+i*Math.PI)*.22:0;
}
