import {resolveVillagePose,captureVillagePose} from './village-pose.js';
import * as T from '/vendor/three.module.js';
import {applyVillageTransform} from '/modules/modeling/village-model-editor.js';
// Rendering adapters; authoritative simulation remains in C++.
export const villageSceneMethods = {
 refreshVillageCollision(group){
  const model=group?.userData?.villageGLB,index=group?.userData?.villageIndex;
  if(!model||!index)return;
  group.updateMatrixWorld(true);
  const bounds=new T.Box3().setFromObject(model);
  if(bounds.isEmpty())return;
  this.villageCollisionBounds??=[];
  const proxy={index,min:bounds.min.clone(),max:bounds.max.clone()};
  const at=this.villageCollisionBounds.findIndex(item=>item.index===index);
  if(at>=0)this.villageCollisionBounds[at]=proxy;else this.villageCollisionBounds.push(proxy);
  this.preview.villageCollisionBounds=this.villageCollisionBounds;
 },
 loadVillageHouse(group,index,building){
  const filename='village_building_'+String(index).padStart(2,'0')+'_lod0.glb';
  import('/vendor/GLTFLoader.js').then(({GLTFLoader})=>new GLTFLoader().loadAsync('/assets/models/buildings/village/'+filename)).then(({scene})=>{
    let savedPose;try{savedPose=resolveVillagePose(index,this.state?.content?.villageLayout||this.catalog.villageLayout)?.pose;}catch(error){group.userData.poseError=error.message;}
    // House 01: infer the base from broad near-planar underside faces.
    // This fixes an incorrectly authored up axis without guessing a 90-degree yaw.
    if(index===1&&!savedPose?.modelPosition){
      scene.updateMatrixWorld(true);
      const faces=[];
      const a=new T.Vector3(),b=new T.Vector3(),c=new T.Vector3();
      const ab=new T.Vector3(),ac=new T.Vector3(),normal=new T.Vector3();
      scene.traverse(obj=>{
        if(!obj.isMesh||!obj.geometry?.attributes?.position)return;
        const geo=obj.geometry,pos=geo.attributes.position,idx=geo.index;
        const count=idx?idx.count:pos.count,step=Math.max(3,Math.floor(count/24000/3)*3);
        for(let j=0;j+2<count;j+=step){
          a.fromBufferAttribute(pos,idx?idx.getX(j):j).applyMatrix4(obj.matrixWorld);
          b.fromBufferAttribute(pos,idx?idx.getX(j+1):j+1).applyMatrix4(obj.matrixWorld);
          c.fromBufferAttribute(pos,idx?idx.getX(j+2):j+2).applyMatrix4(obj.matrixWorld);
          ab.subVectors(b,a);ac.subVectors(c,a);normal.crossVectors(ab,ac);
          const area=normal.length()*.5;
          if(area>1e-7){normal.normalize();faces.push({n:normal.clone(),center:a.clone().add(b).add(c).multiplyScalar(1/3),area});}
        }
      });
      // Discover tilted foundation normals, not just the six world axes.
      // Candidate normals come from the largest actual mesh triangles.
      const directions=[new T.Vector3(0,1,0)];
      for(const face of [...faces].sort((a,b)=>b.area-a.area).slice(0,500)){
        for(const sign of [1,-1]){
          const up=face.n.clone().multiplyScalar(sign);
          // A building foundation should broadly point upwards; reject walls.
          if(up.y<.25||directions.some(v=>v.dot(up)>.992))continue;
          directions.push(up);
          if(directions.length>=90)break;
        }
        if(directions.length>=90)break;
      }
      const candidates=directions.map(up=>{
        let min=Infinity,max=-Infinity;
        for(const face of faces){const h=face.center.dot(up);min=Math.min(min,h);max=Math.max(max,h);}
        const band=Math.max(.025,(max-min)*.035);
        let score=0;
        for(const face of faces){
          const h=face.center.dot(up);
          if(h<=min+band&&Math.abs(face.n.dot(up))>.94)
            score+=face.area*Math.abs(face.n.dot(up));
        }
        // Prefer plausible near-upright orientations when surface scores tie.
        return {up,score:score*(.6+.4*up.y)};
      }).sort((a,b)=>b.score-a.score);
      if(candidates[0]?.score>0){
        const up=candidates[0].up;
        scene.quaternion.setFromUnitVectors(up,new T.Vector3(0,1,0));
        console.info('[Aetherwild] House 01 detected foundation normal',up.toArray(),'score',candidates[0].score);
      }
    }
    const bounds=new T.Box3().setFromObject(scene);
    const size=bounds.getSize(new T.Vector3());
    if(!Number.isFinite(size.x)||Math.min(size.x,size.y,size.z)<=0)throw Error('Invalid village house bounds');
    const scale=Math.min(building.width/size.x,building.depth/size.z,building.height/size.y);
    scene.scale.setScalar(scale);
    const fitted=new T.Box3().setFromObject(scene);
    const center=fitted.getCenter(new T.Vector3());
    scene.position.set(-center.x,-fitted.min.y,-center.z);
    // Ground-contact correction: sample the terrain under the entire building footprint.
    // Using only the center height makes a house float above sloping terrain.
    const footprint=[[0,0],[-.4,-.4],[-.4,.4],[.4,-.4],[.4,.4]];
    const heights=footprint.map(([u,v])=>this.landHeight(group.position.x+u*building.width,group.position.z+v*building.depth)).filter(Number.isFinite);
    // Anchor to the terrain at the house centre, then embed the foundation slightly.
    // The old minimum-footprint anchor left the entire visible foundation hovering
    // where the terrain renderer and catalog elevation did not coincide.
    const centreHeight=this.landHeight(group.position.x,group.position.z);
    if(Number.isFinite(centreHeight))group.position.y=centreHeight;
    else if(heights.length)group.position.y=heights[Math.floor(heights.length/2)];
    // Sink the bottom trim into the terrain rather than allowing a visible air gap.
    // Clamp to avoid burying short houses or changing their overall proportions.
    scene.position.y-=Math.min(.45,Math.max(.16,building.height*.065));
    if(index===1&&!savedPose?.modelPosition){
      // Recompute the actual underside AFTER correcting the GLB orientation.
      // Use a low percentile to ignore isolated stray triangles and align
      // the foundation with the terrain sampled under its whole footprint.
      group.updateMatrixWorld(true);
      const levels=[],v=new T.Vector3();
      scene.traverse(obj=>{
        if(!obj.isMesh||!obj.geometry?.attributes?.position)return;
        const attr=obj.geometry.attributes.position;
        const step=Math.max(1,Math.floor(attr.count/15000));
        for(let j=0;j<attr.count;j+=step){
          v.fromBufferAttribute(attr,j).applyMatrix4(obj.matrixWorld);
          if(Number.isFinite(v.y))levels.push(v.y);
        }
      });
      if(levels.length>100){
        levels.sort((a,b)=>a-b);
        const foundationY=levels[Math.floor(levels.length*.02)];
        // Use the lowest terrain contact point to prevent any floating corner
        // while keeping the house upright. Do not change the detected rotation.
        const terrainY=heights.length?Math.min(...heights):group.position.y;
        // The GLB is not parented to the house group yet, so its sampled
        // vertex heights are in local space. Convert terrain world height to
        // the same local space before calculating the correction.
        const targetLocalY=terrainY-group.position.y-.12;
        const shift=targetLocalY-foundationY;
        if(Number.isFinite(shift))scene.position.y+=shift;
        // House 01 uses a visible foundation above stray underside geometry.
        // Seat the entire building slightly into the terrain to eliminate
        // the persistent visible air gap without changing its rotation.
        scene.position.y-=Math.min(1.25,Math.max(.35,building.height*.12));
        console.info('[Aetherwild] House 01 ground correction',{
          foundationLocalY:foundationY,targetLocalY,shift,terrainY,
          groupY:group.position.y,modelY:scene.position.y
        });
      }
    }
    // GLB rendering repair pass: inconsistent triangle winding and thin
    // overlapping surfaces are common in the imported village assets.
    // Keep mesh positions untouched so manually saved transforms survive.
    scene.traverse(obj=>{
      if(!obj.isMesh)return;
      const fixMaterial=original=>{
        if(!original)return original;
        const material=original.clone();
        material.side=T.DoubleSide;
        material.shadowSide=T.DoubleSide;
        // Avoid opaque cutout surfaces incorrectly depth-writing through
        // neighboring pieces of the same imported house.
        if(material.transparent)material.depthWrite=false;
        material.needsUpdate=true;
        return material;
      };
      obj.material=Array.isArray(obj.material)?obj.material.map(fixMaterial):fixMaterial(obj.material);
      // Thin overlapping roofs/walls produce false diagonal tears when
      // shadow-mapped. Disable their casting until the GLB is remeshed.
      obj.castShadow=false;
      obj.receiveShadow=false;
    });
    group.children.forEach(child=>child.visible=false);
    group.add(scene);
    group.userData.villageModel=filename;
    group.userData.villageIndex=index;
    group.userData.villageGLB=scene;
    group.userData.villageBaseline=captureVillagePose(group,scene);
    try{applyVillageTransform(index,group,scene,this.state?.content?.villageLayout||this.catalog.villageLayout);}catch(error){group.userData.poseError=error.message;}
    this.villageEditorRefresh?.();
    // Generate cheap wall proxies once, from the lower/middle portion of the GLB.
    // Full mesh bounds include roof overhangs and create invisible walls.
    this.villageCollisionBounds??=[];
    group.updateMatrixWorld(true);
    const collisionBounds=new T.Box3().setFromObject(scene);
    const bottom=collisionBounds.min.y, top=collisionBounds.max.y;
    const wallBounds=new T.Box3();
    const vertex=new T.Vector3();
    const lower=bottom+(top-bottom)*.12,upper=bottom+(top-bottom)*.58;
    scene.traverse(obj=>{
      if(!obj.isMesh||!obj.geometry?.attributes?.position)return;
      const positions=obj.geometry.attributes.position;
      for(let i=0;i<positions.count;i++){
        vertex.fromBufferAttribute(positions,i).applyMatrix4(obj.matrixWorld);
        if(vertex.y>=lower&&vertex.y<=upper)wallBounds.expandByPoint(vertex);
      }
    });
    // Fall back to the fitted model bounds if the mesh has no suitable wall vertices.
    if(wallBounds.isEmpty())wallBounds.copy(collisionBounds);
    const existing=this.villageCollisionBounds.findIndex(item=>item.index===index);
    const proxy={index,min:wallBounds.min.clone(),max:wallBounds.max.clone()};
    if(existing>=0)this.villageCollisionBounds[existing]=proxy;
    else this.villageCollisionBounds.push(proxy);
    this.preview.villageCollisionBounds=this.villageCollisionBounds;
    // Temporary model identification marker for isolating the remaining cracked house.
    const markerCanvas=document.createElement('canvas');
    markerCanvas.width=256;markerCanvas.height=96;
    const ctx=markerCanvas.getContext('2d');
    ctx.fillStyle='rgba(17,29,28,0.86)';ctx.fillRect(12,10,232,76);
    ctx.strokeStyle='#ffe8a6';ctx.lineWidth=4;ctx.strokeRect(12,10,232,76);
    ctx.fillStyle='#fff2c7';ctx.font='bold 43px sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';
    ctx.fillText('房屋 '+String(index).padStart(2,'0'),128,49);
    const markerTexture=new T.CanvasTexture(markerCanvas);
    const marker=new T.Sprite(new T.SpriteMaterial({map:markerTexture,depthTest:false,transparent:true}));
    marker.position.set(0,Math.max(building.height, fitted.max.y-fitted.min.y)+1.5,0);
    marker.scale.set(3.8,1.42,1);
    marker.renderOrder=999;
    group.add(marker);
  }).catch(error=>console.warn('[Aetherwild] Village GLB unavailable; keeping procedural house',filename,error));
 }
};
