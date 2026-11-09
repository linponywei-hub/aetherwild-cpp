import * as T from '/vendor/three.module.js';
// Rendering adapters; authoritative simulation remains in C++.
export const monsterSceneMethods = {
 makeGoblinPreview(){
   // Shared mother mesh with lightweight variants; these are visual-only test mobs.
   const variants=[
     {name:'哥布林斥候',scale:.85,tint:0x9cc976,weapon:'dagger',offset:[-3,0]},
     {name:'哥布林投石兵',scale:1,tint:0xc0d99a,weapon:'stone',offset:[0,0]},
     {name:'哥布林戰士',scale:1.08,tint:0x89b77b,weapon:'sword',offset:[3,0]},
     {name:'哥布林弓手',scale:.94,tint:0xb3bc7a,weapon:'bow',offset:[-2,3]},
     {name:'哥布林薩滿',scale:1.04,tint:0xa4b5c6,weapon:'staff',offset:[2,3]},
     {name:'哥布林隊長',scale:1.3,tint:0xb9a071,weapon:'axe',offset:[0,6]}
   ];
   this.goblinPack=[];
   const base=new T.Group();base.position.set(0,this.landHeight(0,48)+.04,48);
   this.root.add(base);this.goblinPackRoot=base;
   for(let i=0;i<variants.length;i++){
     const v=variants[i],g=new T.Group();g.position.set(v.offset[0],0,v.offset[1]);base.add(g);
     g.userData={kind:'goblin-preview',nonCombat:true,aiVisualOnly:true,variant:v,packIndex:i};
     const label=this.label(v.name,'#c9e4a2',.85);label.position.y=.95;g.add(label);
     const placeholder=this.sphere(g,.24,'#73994a',0,.38,0,[.8,1.4,.8]);
     g.userData.placeholder=placeholder;
     this.goblinPack.push(g);
   }
   this.goblinPreview=this.goblinPack[1];
   import('/vendor/GLTFLoader.js').then(({GLTFLoader})=>new GLTFLoader().loadAsync('/assets/models/characters/monsters/goblin-test.glb')).then(({scene})=>{
     for(const g of this.goblinPack){
       const v=g.userData.variant,copy=scene.clone(true);
       // Clone the skeleton per goblin: default clone(true) shares the source
       // skeleton and otherwise every variant drives the same armature.
       const originals=[],clones=[];
       scene.traverse(n=>originals.push(n));copy.traverse(n=>clones.push(n));
       const mapping=new Map(originals.map((n,i)=>[n,clones[i]]));
       for(const original of originals)if(original.isSkinnedMesh){
         const target=mapping.get(original);
         const bones=original.skeleton.bones.map(b=>mapping.get(b));
         if(bones.every(Boolean))target.bind(new T.Skeleton(bones,original.skeleton.boneInverses.map(m=>m.clone())),original.bindMatrix.clone());
       }
       const joints={};copy.traverse(n=>{if(n.isBone)joints[n.name.replace(/^mixamorig:/,'')]=n;});
       g.userData.joints=joints;
       g.userData.restPose=Object.fromEntries(Object.entries(joints).map(([k,b])=>[k,b.quaternion.clone()]));
       copy.traverse(node=>{
         if(!node.isMesh)return;
         node.material=Array.isArray(node.material)?node.material.map(m=>m.clone()):node.material?.clone();
         const mats=Array.isArray(node.material)?node.material:[node.material];
         for(const mat of mats)if(mat?.color)mat.color.multiply(new T.Color(v.tint));
         node.castShadow=false;
       });
       const bounds=new T.Box3().setFromObject(copy),h=bounds.max.y-bounds.min.y;
       if(!Number.isFinite(h)||h<=0)continue;
       copy.scale.setScalar(.5*v.scale/h);copy.updateMatrixWorld(true);
       const ground=new T.Box3().setFromObject(copy);
       // The GLB pivot is not at the feet. Keep the lowest skinned mesh above terrain.
       copy.position.y-=ground.min.y;copy.position.y+=.22;
       g.add(copy);g.userData.placeholder.visible=false;
       g.userData.animatedModel=copy;g.userData.baseModelY=copy.position.y;g.userData.baseModelYaw=copy.rotation.y;
       g.userData.groundClearance=.22;
       // Store the foot-bone baseline in local model coordinates for gait grounding.
       const feet=[joints.LeftFoot,joints.RightFoot].filter(Boolean);
       if(feet.length){copy.updateMatrixWorld(true);g.userData.footBaseline=Math.min(...feet.map(b=>{const w=new T.Vector3();b.getWorldPosition(w);return w.y;}));}
       // Role equipment is attached to the variant, not baked into the shared GLB.
       const gear=new T.Group();g.add(gear);
       const armor=this.mat(v.weapon==='axe'?'#8a542d':v.weapon==='staff'?'#594a80':v.weapon==='bow'?'#5e7040':'#71533c');
       if(v.weapon==='axe'){this.box(gear,.55,.18,.33,'#a68132',0,.62,0);this.sphere(gear,.25,'#a57c2f',0,.82,0,[1.2,.55,1]);}
       if(v.weapon==='staff'){this.mesh(new T.ConeGeometry(.22,.45,6),armor,[0,.82,0],gear);this.sphere(gear,.1,'#4edcce',.3,.96,0);}
       if(v.weapon==='bow'){this.box(gear,.48,.22,.28,armor,0,.48,0);this.box(gear,.12,.44,.16,'#765739',-.3,.52,-.13);}
       if(v.weapon==='sword'){this.box(gear,.48,.26,.32,armor,0,.47,0);this.mesh(new T.CylinderGeometry(.22,.22,.055,12),this.mat('#555b63'),[-.35,.45,.1],gear).rotation.z=Math.PI/2;}
       if(v.weapon==='dagger')this.box(gear,.35,.14,.25,armor,0,.42,0);
       // Distinct silhouette kits, built once per variant from low-poly meshes.
       const leather=this.mat('#49372b'),bronze=this.mat('#b08b43'),bone=this.mat('#e1d5ae');
       if(v.weapon==='dagger'){
         this.mesh(new T.ConeGeometry(.12,.3,5),bone,[-.14,.8,0],gear).rotation.z=.4;
         this.mesh(new T.ConeGeometry(.12,.3,5),bone,[.14,.8,0],gear).rotation.z=-.4;
         this.box(gear,.43,.12,.3,leather,0,.34,0);
       }
       if(v.weapon==='stone'){
         this.box(gear,.42,.25,.34,leather,0,.43,-.12);
         for(let n=0;n<3;n++)this.sphere(gear,.08,'#78756d',-.12+n*.12,.48,-.34);
       }
       if(v.weapon==='sword'){
         this.box(gear,.62,.17,.34,bronze,0,.7,0);
         this.box(gear,.15,.28,.15,bronze,0,.86,0);
       }
       if(v.weapon==='bow'){
         const quiver=this.cylinder(gear,.11,.09,.4,leather,-.24,.59,-.26,7);
         for(let n=0;n<4;n++)this.box(gear,.012,.34,.012,bone,-.32+n*.055,.86,-.26);
         this.box(gear,.5,.1,.3,leather,0,.34,0);
       }
       if(v.weapon==='staff'){
         this.box(gear,.5,.32,.4,armor,0,.42,0);
         for(const side of [-1,1])this.mesh(new T.ConeGeometry(.1,.25,6),bone,[side*.2,.9,.05],gear);
       }
       if(v.weapon==='axe'){
         this.box(gear,.72,.3,.4,bronze,0,.48,0);
         this.mesh(new T.ConeGeometry(.19,.34,6),bronze,[0,.95,0],gear);
         for(const side of [-1,1])this.mesh(new T.ConeGeometry(.12,.28,6),bone,[side*.28,.85,0],gear).rotation.z=side*.5;
       }
       const steel=this.mat('#8b8d83'),wood=this.mat('#69533a');
       if(v.weapon==='stone')this.sphere(gear,.09,'#777870',.33,.5,.1);
       if(v.weapon==='dagger'||v.weapon==='sword')this.box(gear,.055,v.weapon==='dagger'?.28:.44,.06,steel,.32,.48,.05);
       if(v.weapon==='staff')this.cylinder(gear,.035,.035,.8,wood,.3,.55,0,6);
       if(v.weapon==='axe'){this.cylinder(gear,.035,.035,.7,wood,.32,.52,0,6);this.box(gear,.3,.2,.08,steel,.32,.83,0);}
       if(v.weapon==='bow'){const bow=this.mesh(new T.TorusGeometry(.29,.035,6,24,Math.PI),wood,[.38,.58,.2],gear);bow.rotation.z=Math.PI/2;this.cylinder(gear,.008,.008,.58,'#ded4ad',.38,.58,.2,5);this.box(gear,.025,.025,.48,'#c4c6b7',.4,.6,.28);}
       g.userData.gear=gear;
     }
   }).catch(error=>console.warn('[Aetherwild] Goblin pack GLB failed; placeholders retained',error));
 },
 goblinTest(e){
   const g=this.wolf(false,e.definition?.replace('boss_','')||'windchime');
   if(!this.renderer.software){
     const asset='/assets/models/characters/monsters/goblin-test.glb';
     import('/vendor/GLTFLoader.js').then(({GLTFLoader})=>new GLTFLoader().loadAsync(asset)).then(({scene})=>{
       if(!g.parent)return;
       const bounds=new T.Box3().setFromObject(scene),height=bounds.max.y-bounds.min.y;
       if(!Number.isFinite(height)||height<=0)throw Error('Invalid goblin model bounds');
       scene.scale.setScalar(0.5/height);
       scene.updateMatrixWorld(true);
       const grounded=new T.Box3().setFromObject(scene);
       scene.position.y-=grounded.min.y;
       for(const child of [...g.children])if(child!==g.userData.goblinModel)child.visible=false;
       g.add(scene);g.userData.goblinModel=scene;
       scene.traverse(node=>{if(node.isMesh){node.castShadow=true;node.receiveShadow=true;}});
     }).catch(error=>{if(!g.userData.goblinWarned){console.warn('[Aetherwild] Goblin test GLB unavailable; procedural monster retained:',error);g.userData.goblinWarned=true;}});
   }
   return g;
 },
 wolf(boss=false,region='windchime'){const g=new T.Group();if(this.renderer.software)this.contactShadow(g,boss?2.5:.65,boss?1.8:.9);if(boss){const stone=region==='cinder'?'#5d5143':region==='frosthold'?'#98b6ba':region==='astral'?'#92909d':'#777d5b',glow=region==='cinder'?'#ff9b45':region==='frosthold'?'#96efff':region==='astral'?'#c7b2ff':'#d2e18b';this.box(g,2.7,3,1.7,stone,0,3,0);this.box(g,1.4,1.4,1.3,stone,0,5.05,0);for(const x of [-1,1]){this.box(g,.85,2,.9,stone,x*.85,1,0);this.box(g,1.2,1.9,1.2,stone,x*1.9,3,0);this.sphere(g,.16,this.mat(glow,{emissive:glow,emissiveIntensity:2}),x*.35,5.1,.69);}this.mesh(new T.OctahedronGeometry(.7),this.mat(glow,{emissive:glow,emissiveIntensity:1}),[0,3.1,.9],g);g.userData.boss=true;return g;}this.sphere(g,.65,'#797f71',0,.9,0,[.65,.8,1.2]);this.sphere(g,.36,'#8d9380',0,1.25,.6,[.8,1,.9]);this.sphere(g,.25,'#b0b5a0',0,1.13,.92,[.8,.65,1]);for(let x of [-.22,.22]){this.mesh(new T.ConeGeometry(.15,.4,4),'#6d7569',[x,1.63,.58],g);this.sphere(g,.04,this.mat('#d9c17b',{emissive:'#c8a952',emissiveIntensity:.4}),x,1.33,.85);}g.userData.legs=[];for(const x of [-.28,.28])for(const z of [-.48,.48]){let l=this.box(g,.16,.64,.17,'#636e61',x,.4,z);g.userData.legs.push(l);}const tail=this.cylinder(g,.12,.2,.8,'#69765f',0,1,-.9,7);tail.rotation.x=-.8;return g;},
 updateGoblinAI(dt){
   const p=this.state?.player;
   if(!p||p.scene!=='world')return;
   for(const g of this.goblinPack||[]){
   if(Math.hypot(p.position.x-(g.position.x+this.goblinPackRoot.position.x),p.position.z-(g.position.z+this.goblinPackRoot.position.z))>30)continue;
   const ai=g.userData.ai ||= {mode:'patrol',timer:0,attackCooldown:1.5,stones:0,projectiles:[],home:g.position.clone(),seed:g.userData.packIndex};
   ai.timer+=dt;ai.attackCooldown-=dt;
   const worldPos=new T.Vector3();g.getWorldPosition(worldPos);
   const dx=p.position.x-worldPos.x,dz=p.position.z-worldPos.z,dist=Math.hypot(dx,dz);
   const leader=this.goblinPack[1];const leaderPos=new T.Vector3();leader.getWorldPosition(leaderPos);
   const alerted=Math.hypot(p.position.x-leaderPos.x,p.position.z-leaderPos.z)<15;
   if(alerted&&dist<17)ai.mode=dist<2.5?'retreat':dist<7?'strafe':'chase';
   else if(dist>19)ai.mode='patrol';
   const heading=dist>.01?Math.atan2(dx,dz):g.rotation.y;
   if(ai.mode!=='patrol')g.rotation.y+=Math.atan2(Math.sin(heading-g.rotation.y),Math.cos(heading-g.rotation.y))*Math.min(1,dt*4);
   let vx=0,vz=0;
   if(ai.mode==='chase'&&dist>5){vx=dx/dist*2;vz=dz/dist*2;}
   if(ai.mode==='strafe'&&dist>0.1){const dir=Math.sin(ai.timer*.8+ai.seed)>0?1:-1;vx=dz/dist*1.15*dir;vz=-dx/dist*1.15*dir;}
   if(ai.mode==='retreat'&&dist<2.8){vx=-dx/dist*1.1;vz=-dz/dist*1.1;}
   if(ai.mode==='patrol'){
     const a=ai.timer*.6;const tx=ai.home.x+Math.cos(a)*4,tz=ai.home.z+Math.sin(a)*4;
     const px=tx-g.position.x,pz=tz-g.position.z,d=Math.hypot(px,pz);
     if(d>.3){vx=px/d*.65;vz=pz/d*.65;g.rotation.y=Math.atan2(vx,vz);}
   }
   g.position.x+=vx*dt;g.position.z+=vz*dt;
   g.position.y=this.landHeight(g.position.x+this.goblinPackRoot.position.x,g.position.z+this.goblinPackRoot.position.z)-this.goblinPackRoot.position.y;
   const model=g.userData.animatedModel;
   ai.moving=Math.hypot(vx,vz)>.1;
   if(model){model.rotation.z=Math.sin(this.time*(ai.moving?11:1.35))*(ai.moving?.07:.02);}
   const weapon=g.userData.variant.weapon;
   const range=weapon==='bow'?12:weapon==='staff'?10:weapon==='stone'?10:2.2;
   if(dist<range&&ai.attackCooldown<=0){
     if(weapon==='stone'&&ai.stones<=0){ai.stones=3;ai.gatherUntil=this.time+.7;ai.attackCooldown=1.2;}
     else {
       if(weapon==='stone')ai.stones--;
       ai.throwUntil=this.time+.55;ai.attackCooldown=(weapon==='dagger'?1.1:weapon==='sword'?1.6:weapon==='axe'?2.7:weapon==='bow'?2:weapon==='staff'?3:2.2)+Math.random()*.4;
       if(['stone','bow','staff'].includes(weapon)){
       const projectile=weapon==='bow'?new T.ConeGeometry(.055,.35,6):weapon==='staff'?new T.SphereGeometry(.12,8,6):new T.DodecahedronGeometry(.085,0);
       const material=this.mat(weapon==='staff'?'#65d8d8':weapon==='bow'?'#b5a38a':'#88857c');
       const stone=this.mesh(projectile,material,[worldPos.x+Math.sin(g.rotation.y)*.28,worldPos.y+.48,worldPos.z+Math.cos(g.rotation.y)*.28],this.world);
       const target=new T.Vector3(p.position.x,p.position.y+1.1,p.position.z);
       const origin=stone.position.clone();const velocity=target.sub(origin).normalize().multiplyScalar(8);
       velocity.y+=weapon==='stone'?1.5:0;ai.projectiles.push({mesh:stone,velocity,life:2.4,gravity:weapon==='stone'?9.8:0});
       }
     }
   }
   for(const shot of [...ai.projectiles]){
     shot.life-=dt;shot.velocity.y-=shot.gravity*dt;shot.mesh.position.addScaledVector(shot.velocity,dt);
     const ground=this.landHeight(shot.mesh.position.x,shot.mesh.position.z);
     if(shot.life<=0||shot.mesh.position.y<ground){this.world.remove(shot.mesh);shot.mesh.geometry.dispose();shot.mesh.material.dispose();ai.projectiles.splice(ai.projectiles.indexOf(shot),1);}
   }
   }
 }
};
