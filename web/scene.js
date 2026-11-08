import {ModelAssets} from '/model-assets.js';
import {CityScene} from '/city-scene.js';
import {humanoid,dressWeapon,Nameplates} from '/actor-scene.js';
import {RemoteTrack,advanceRemoteClock} from '/remote-motion.js';
import {CombatFeedback} from '/combat-feedback.js';
import {renderFrame,attachViewmodel} from '/render-passes.js';
import {EnvironmentVisual} from '/environment-scene.js';
import {weaponPose} from '/weapon-pose.js';
import {makeLandscape} from '/landscape-scene.js';
import {gridHeight,surfaceAt} from '/landscape-model.js';
import {terrainMesh,updateContentScene,updateCastTelegraphs} from '/content-scene.js';
import {MovementPreview} from '/movement-preview.js';
import * as T from '/vendor/three.module.js';
import {SoftwareRenderer} from '/software-renderer.js';
export class RealmScene {
 constructor(canvas,catalog,callbacks){this.canvas=canvas;this.catalog=catalog;this.cb=callbacks;this.world=new T.Scene();this.world.background=new T.Color('#b8cbd1');this.world.fog=new T.FogExp2('#b8c9c0',.0037);this.camera=new T.PerspectiveCamera(68,innerWidth/innerHeight,.08,1500);this.applySettings();this.camera.position.set(0,2,34);this.camera.rotation.order='YXZ';this.yaw=0;this.pitch=-.08;this.keys=new Set();this.jump=false;this.state=null;this.preview=new MovementPreview(catalog);this.networkRTT=0;this.target=null;this.entities=new Map();this.npcObjects=new Map();this.nameplates=new Nameplates();this.modelAssets=new ModelAssets();this.sceneryCell='';this.swing=0;this.feedback=new CombatFeedback();this.time=0;this.arrowTrails=[];this.clock=new T.Clock();try{this.renderer=new T.WebGLRenderer({canvas,antialias:true,alpha:false,powerPreference:'high-performance'});}catch{this.renderer=new SoftwareRenderer(canvas);}this.renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));this.renderer.setSize(innerWidth,innerHeight);this.renderer.shadowMap.enabled=localStorage.getItem('aw-shadows')!=='0';this.renderer.shadowMap.type=T.PCFSoftShadowMap;this.renderer.outputColorSpace=T.SRGBColorSpace;this.renderer.toneMapping=T.ACESFilmicToneMapping;this.renderer.toneMappingExposure=1.05;this.hemisphere=new T.HemisphereLight('#d1e5ed','#586d38',2.05);this.world.add(this.hemisphere);this.sun=new T.DirectionalLight('#ffdfaa',3.4);this.sun.position.set(-35,180,-25);this.sun.castShadow=true;this.sun.shadow.mapSize.set(1024,1024);this.sun.shadow.camera.left=-95;this.sun.shadow.camera.right=95;this.sun.shadow.camera.top=95;this.sun.shadow.camera.bottom=-95;this.sun.shadow.camera.near=.5;this.sun.shadow.camera.far=700;this.sun.shadow.normalBias=.04;this.sun.shadow.bias=-.00015;this.world.add(this.sun,this.sun.target);this.root=new T.Group();this.dungeon=new T.Group();this.ambient=new T.Group();this.entityRoot=new T.Group();this.world.add(this.root,this.dungeon,this.ambient,this.entityRoot);this.makeWorld();this.makeDungeon();this.dungeon.visible=false;this.ray=new T.Raycaster();this.mouse=new T.Vector2(0,0);this.weaponScene=new T.Scene();this.weaponCamera=new T.PerspectiveCamera(68,innerWidth/innerHeight,.01,5);this.weaponScene.add(this.weaponCamera,new T.HemisphereLight('#ffffff','#6b705a',2.5));this.makeWeapon('warrior');this.world.add(this.camera);this.start=performance.now();this.active=false;window.addEventListener('resize',()=>this.resize());document.addEventListener('pointerlockchange',()=>callbacks.lock?.(document.pointerLockElement===canvas));document.addEventListener('mousemove',e=>{if(document.pointerLockElement===canvas){const sens=Number(localStorage.getItem('aw-sensitivity')||1);this.yaw-=e.movementX*.0022*sens;this.pitch=Math.max(-1.3,Math.min(1.3,this.pitch-e.movementY*.0022*sens));}});this.environment=new EnvironmentVisual(this);this.animate();}
 shootArrow(){const direction=new T.Vector3();this.camera.getWorldDirection(direction);const g=new T.Group();const material=new T.MeshBasicMaterial({color:'#e5d5a5'}),shaft=new T.Mesh(new T.CylinderGeometry(.012,.012,.7,4),material);g.add(shaft);const tip=new T.Mesh(new T.ConeGeometry(.035,.11,4),material);tip.position.y=.4;g.add(tip);g.position.copy(this.camera.position).addScaledVector(direction,.8);g.position.y-=.13;g.quaternion.setFromUnitVectors(new T.Vector3(0,1,0),direction);this.world.add(g);this.arrowTrails.push({g,direction,life:.32});}
 contactShadow(parent,x,z){const m=new T.Mesh(new T.CircleGeometry(1,12),new T.MeshBasicMaterial({color:'#102018',transparent:true,opacity:.25,depthWrite:false,side:T.DoubleSide}));m.rotation.x=-Math.PI/2;m.scale.set(x,z,1);m.position.y=.035;m.castShadow=m.receiveShadow=false;parent.add(m);return m;}
 mat(color,extra={}){return new T.MeshStandardMaterial({color,roughness:.88,...extra});}
 mesh(geo,mat,pos,parent=this.root){let m=new T.Mesh(geo,typeof mat==='string'||typeof mat==='number'?this.mat(mat):mat);if(pos)m.position.set(...pos);m.castShadow=true;m.receiveShadow=true;parent.add(m);return m;}
 box(parent,w,h,d,color,x=0,y=0,z=0){return this.mesh(new T.BoxGeometry(w,h,d),color,[x,y,z],parent);}
 cylinder(parent,r1,r2,h,color,x=0,y=0,z=0,s=12){return this.mesh(new T.CylinderGeometry(r1,r2,h,s),color,[x,y,z],parent);}
 sphere(parent,r,color,x=0,y=0,z=0,scale){let m=this.mesh(new T.SphereGeometry(r,12,9),color,[x,y,z],parent);if(scale)m.scale.set(...scale);return m;}
 label(text,color='#f4e6bb',size=1.6){const c=document.createElement('canvas');c.width=512;c.height=84;let ctx=c.getContext('2d');ctx.font='30px system-ui';ctx.textAlign='center';ctx.fillStyle='rgba(12,28,22,.75)';ctx.beginPath();ctx.roundRect(30,3,452,72,12);ctx.fill();ctx.fillStyle=color;ctx.fillText(text,256,49);let tex=new T.CanvasTexture(c),m=new T.SpriteMaterial({map:tex,transparent:true,depthWrite:false});let s=new T.Sprite(m);s.scale.set(size*3,size*.5,1);return s;}
 tree(x,z,h=8,parent=this.ambient,seed=0){let g=new T.Group();g.position.set(x,this.landHeight(x,z),z);parent.add(g);this.cylinder(g,.18,.4,h*.63,'#65503a',0,h*.31,0,7);const leaf=['#526e47','#708447','#627c42','#8b9651'][Math.abs(seed)%4];if(seed%3===0){for(let i=0;i<3;i++)this.mesh(new T.ConeGeometry(h*(.25-i*.04),h*.49,7),leaf,[0,h*(.55+i*.18),0],g);}else{this.sphere(g,h*.31,leaf,0,h*.73,0,[1,1.1,1]);this.sphere(g,h*.23,leaf,h*.2,h*.65,.1);this.sphere(g,h*.22,leaf,-h*.22,h*.62,h*.08);}return g;}
 arch(parent,x,z,color='#b3bca5',height=6){this.box(parent,1,height,1,color,x-3,height/2,z);this.box(parent,1,height,1,color,x+3,height/2,z);let a=this.mesh(new T.TorusGeometry(3,.55,7,16,Math.PI),color,[x,height,z],parent);a.rotation.z=0;return a;}
 makeWorld(){let soil=this.mat('#718866');this.mesh(new T.PlaneGeometry(this.catalog.terrain?this.catalog.worldLimit*2:8000,this.catalog.terrain?this.catalog.worldLimit*2:8000),soil,[0,this.catalog.terrain?-500:-.06,0]).rotation.x=-Math.PI/2;if(this.catalog.terrain){this.globalTerrain=terrainMesh(this.catalog.terrain,'#718866',this.catalog.landscape);this.root.add(this.globalTerrain);}this.sky=this.mesh(new T.SphereGeometry(1000,24,16),new T.ShaderMaterial({side:T.BackSide,depthWrite:false,uniforms:{top:{value:new T.Color('#7aaac5')},bottom:{value:new T.Color('#efe5bf')}},vertexShader:'varying vec3 v;void main(){v=position;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.0);}',fragmentShader:'varying vec3 v;uniform vec3 top;uniform vec3 bottom;void main(){float h=clamp(normalize(v).y,0.0,1.0);gl_FragColor=vec4(mix(bottom,top,pow(h,.65)),1.0);}'}),[0,0,0]);this.sky.castShadow=false;this.sky.renderOrder=-1000;this.sky.material.depthTest=false;
 for(const r of this.catalog.regions){const g=new T.Group();g.position.set(r.position.x,r.elevation,r.position.z);this.root.add(g);const palettes={windchime:['#c7c7a1','#817152'],verdant:['#9ca47a','#567b50'],cinder:['#555b58','#9d6149'],astral:['#e1dccc','#8b8bb3'],frosthold:['#b2c8ca','#698d9e'],tidehaven:['#b5c4b1','#437b81'],sundune:['#d9c48e','#9b7547'],darkforest:['#3d5144','#293d31']},pal=palettes[r.id]||['#c4c4a0','#687d68'];if(r.wilderness)continue;
 if(['verdant','astral'].includes(r.id)){let shape=new T.Shape();shape.absarc(0,0,94,0,Math.PI*2,false);if(r.id==='verdant'){let hole=new T.Path();hole.moveTo(-8,-31);hole.lineTo(-8,-96);hole.lineTo(8,-96);hole.lineTo(8,-31);hole.closePath();shape.holes.push(hole);}let geo=new T.ExtrudeGeometry(shape,{depth:1.5,bevelEnabled:false,curveSegments:40});geo.rotateX(-Math.PI/2);this.mesh(geo,pal[0],[0,-1.5,0],g);for(let k=0;k<16;k++){let a=k*Math.PI/8;this.cylinder(g,.4,.6,4,pal[0],Math.sin(a)*91,1,Math.cos(a)*91);}
 if(r.id==='verdant'){this.cylinder(g,8,13,38,'#594e35',0,-20,0,14);for(let i=0;i<7;i++){let a=i*6.28/7;this.sphere(g,22,'#637d4b',Math.cos(a)*25,24+Math.sin(a)*2,Math.sin(a)*25,[1,.48,1]);}const ramp=this.box(this.root,14,1,Math.hypot(120,32),pal[0],r.position.x,16-.5,r.position.z+90);ramp.rotation.x=Math.atan2(32,120);}
 if(r.id==='astral'){this.box(g,13,1,30,'#dad9c1',0,-.5,101);this.cylinder(g,22,5,38,'#787f8c',0,-21,0);for(let i=0;i<8;i++){let a=i*Math.PI/4;this.cylinder(g,.7,.7,19,'#dad8be',Math.cos(a)*20,9,Math.sin(a)*20);}let ring=this.mesh(new T.TorusGeometry(22,.7,8,48),this.mat('#d5b66c',{metalness:.6}),[0,20,0],g);ring.rotation.x=Math.PI/2;this.sphere(g,4,this.mat('#b0b7ed',{emissive:'#7a76d4',emissiveIntensity:.45}),0,21,0);}
 }else if(!['tidehaven','aw_westport','aw_southmill'].includes(r.id)){this.cylinder(g,r.city?38:17,r.city?38:17,.16,pal[0],0,.02,0,48);for(let k=0;k<4;k++){let road=this.box(g,r.city?9:5,.06,r.city?170:120,'#9a9c7c',0,-.01,0);road.rotation.y=k*Math.PI/4;road.castShadow=false;}}
 if(r.id==='windchime'){this.cylinder(g,2.5,3,.8,'#8e947e',0,.4,-12);this.cylinder(g,1.9,1.9,.3,'#78968c',0,.82,-12);this.cylinder(g,.3,.5,2.8,'#b4b79a',0,1.6,-12);this.cylinder(g,.95,.3,.4,'#c7c4a3',0,3.2,-12);let flag=this.box(g,.05,2,1.2,'#c9c195',-10,3,-4);this.cylinder(g,.06,.06,5,'#77705b',-10,2.5,-4);this.arch(g,0,-60,pal[0],5);}




 }
 makeLandscape(this);this.cityScene=new CityScene(this);
 for(const b of this.catalog.buildings.filter(b=>!b.style.includes('|'))){if(b.region==='windchime')continue;const g=new T.Group();g.position.set(b.position.x,b.position.y,b.position.z);this.root.add(g);if(this.renderer.software)this.contactShadow(g,b.width*.64,b.depth*.64);const dark=b.region==='cinder',snow=b.region==='frosthold',astral=b.region==='astral',desert=b.region==='sundune';const stone=dark?'#5a5a50':desert?'#d4bf8b':snow?'#a9bdb5':astral?'#dedbc5':'#c4c4a0',roof=dark?'#5c4339':snow?'#6d8d95':astral?'#9292ad':b.region==='tidehaven'?'#4f7473':'#686f51';this.box(g,b.width,b.height*.72,b.depth,stone,0,b.height*.36,0);this.box(g,b.width+.2,.4,b.depth+.2,'#6a6a4f',0,.2,0);const roofMesh=this.mesh(new T.ConeGeometry(Math.hypot(b.width,b.depth)*.57,b.height*.48,4),roof,[0,b.height*.82,0],g);roofMesh.rotation.y=Math.PI/4;roofMesh.scale.z=b.depth/b.width;this.box(g,1.8,3,.14,'#504f39',0,1.5,b.depth/2+.08);for(const x of [-b.width*.28,b.width*.28]){this.box(g,1.6,1.8,.12,this.mat('#e5c780',{emissive:'#bb9858',emissiveIntensity:.22}),x,b.height*.43,b.depth/2+.09);this.box(g,.12,1.9,.18,'#65684b',x,b.height*.43,b.depth/2+.18);}if(b.region==='windchime'){for(const x of [-b.width/2+.2,b.width/2-.2])this.box(g,.22,b.height*.72,b.depth+.1,'#7b7354',x,b.height*.36,0);this.box(g,1,3,1,'#99977e',b.width*.26,b.height*.9,0);}}
 for(const n of this.catalog.npcs){const eileenTest=n.region==='windchime'&&n.name==='艾琳';const entity=this.human(n.role,n.id,eileenTest?'HUM_EILEEN_TEST_001':null);if(eileenTest)entity.userData.eileenTest=true;entity.position.set(n.position.x,n.position.y,n.position.z);entity.userData={...entity.userData,kind:'npc',data:n};this.nameplates.register('npc:'+n.id,entity,n.name+' · '+(({guild:'公會',merchant:'商人',smith:'鍛造',healer:'療癒',cartographer:'製圖'})[n.role]||'居民'),true);this.root.add(entity);this.npcObjects.set(n.id,entity);entity.userData.track=new RemoteTrack({changesOnly:true});}
 for(const d of this.catalog.dungeons){let g=new T.Group();g.position.set(d.position.x,d.position.y,d.position.z);g.userData={kind:'entrance',data:d};this.root.add(g);if(d.entrance==='hollow'){this.cylinder(g,2,2.7,8,'#5d523a',0,4,0,8);this.sphere(g,4,'#5b7046',0,8,0);}else if(d.entrance==='hut'){this.box(g,6,4.2,5,'#827957',0,2.1,0);let roof=this.mesh(new T.ConeGeometry(5,3,4),'#525f40',[0,5,0],g);roof.rotation.y=Math.PI/4;}else{this.sphere(g,3,'#868b7a',0,2,0,[1.5,1.2,.6]);}this.box(g,2,3.5,.15,this.mat('#193a36',{emissive:'#4b8874',emissiveIntensity:.25}),0,1.75,2.7);let p=this.mesh(new T.TorusGeometry(1.4,.07,5,20),this.mat('#b4d4ad',{emissive:'#a2d5a4',emissiveIntensity:.4}),[0,1.8,2.8],g);p.scale.y=1.25;}
 this.liftRoot=new T.Group();this.root.add(this.liftRoot);const astral=this.catalog.regions.find(r=>r.id==='astral');this.liftRoot.position.set(astral.position.x,0,astral.position.z+110);this.cylinder(this.liftRoot,6,6,.6,'#b5b5a3',0,-.3,0,24);this.cylinder(this.liftRoot,5.8,5.8,.04,this.mat('#8e9cb4',{emissive:'#7d83a5',emissiveIntensity:.2}),0,.02,0,24);this.liftRoot.userData={kind:'lift',data:{id:'astral',name:'星穹升降台'}};for(const h of [0,62]){const post=this.box(this.root,.5,1.5,.5,'#b9c0aa',astral.position.x+5,h+.75,astral.position.z+110);post.userData={kind:'lift',data:{id:'astral',name:'星穹升降台'}};}
 this.makeScenery(0,24);
 }
 loadVillageHouse(group,index,building){
  const filename='village_building_'+String(index).padStart(2,'0')+'_lod0.glb';
  import('/vendor/GLTFLoader.js').then(({GLTFLoader})=>new GLTFLoader().loadAsync('/assets/models/buildings/village/'+filename)).then(({scene})=>{
    // House 01: infer the base from broad near-planar underside faces.
    // This fixes an incorrectly authored up axis without guessing a 90-degree yaw.
    if(index===1){
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
    if(index===1){
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
    scene.traverse(obj=>{
      if(!obj.isMesh)return;
      obj.castShadow=true;obj.receiveShadow=true;
      if(index===1){
        // The 001 Tripo mesh has open seams and back-facing patches.
        // Render both sides of its original textured surfaces to prevent
        // back-face culling from revealing the sky along those joins.
        const materials=Array.isArray(obj.material)?obj.material:[obj.material];
        for(const material of materials){
          if(!material)continue;
          material.side=T.DoubleSide;
          material.needsUpdate=true;
        }
      }
    });
    group.children.forEach(child=>child.visible=false);
    group.add(scene);
    group.userData.villageModel=filename;
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
 makeScenery(){}
 human(role='warrior',id='',assetOverride=null){const g=humanoid(role,id||role);g.userData.proceduralParts=[...g.children];const assetId=({warrior:'HUM_WARRIOR_001',mage:'HUM_MAGE_001',ranger:'HUM_RANGER_001',merchant:'HUM_MERCHANT_001',guard:'HUM_GUARD_001'})[role]||'HUM_NPC_001';g.userData.modelAssetId=assetOverride||assetId;if(this.renderer.software)this.contactShadow(g,.45,.35);else if(this.modelAssets.registry[g.userData.modelAssetId])queueMicrotask(()=>this.modelAssets.attach(g,g.userData.modelAssetId));return g;}
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
 }
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
 }
 wolf(boss=false,region='windchime'){const g=new T.Group();if(this.renderer.software)this.contactShadow(g,boss?2.5:.65,boss?1.8:.9);if(boss){const stone=region==='cinder'?'#5d5143':region==='frosthold'?'#98b6ba':region==='astral'?'#92909d':'#777d5b',glow=region==='cinder'?'#ff9b45':region==='frosthold'?'#96efff':region==='astral'?'#c7b2ff':'#d2e18b';this.box(g,2.7,3,1.7,stone,0,3,0);this.box(g,1.4,1.4,1.3,stone,0,5.05,0);for(const x of [-1,1]){this.box(g,.85,2,.9,stone,x*.85,1,0);this.box(g,1.2,1.9,1.2,stone,x*1.9,3,0);this.sphere(g,.16,this.mat(glow,{emissive:glow,emissiveIntensity:2}),x*.35,5.1,.69);}this.mesh(new T.OctahedronGeometry(.7),this.mat(glow,{emissive:glow,emissiveIntensity:1}),[0,3.1,.9],g);g.userData.boss=true;return g;}this.sphere(g,.65,'#797f71',0,.9,0,[.65,.8,1.2]);this.sphere(g,.36,'#8d9380',0,1.25,.6,[.8,1,.9]);this.sphere(g,.25,'#b0b5a0',0,1.13,.92,[.8,.65,1]);for(let x of [-.22,.22]){this.mesh(new T.ConeGeometry(.15,.4,4),'#6d7569',[x,1.63,.58],g);this.sphere(g,.04,this.mat('#d9c17b',{emissive:'#c8a952',emissiveIntensity:.4}),x,1.33,.85);}g.userData.legs=[];for(const x of [-.28,.28])for(const z of [-.48,.48]){let l=this.box(g,.16,.64,.17,'#636e61',x,.4,z);g.userData.legs.push(l);}const tail=this.cylinder(g,.12,.2,.8,'#69765f',0,1,-.9,7);tail.rotation.x=-.8;return g;}
 resource(def){let g=new T.Group();const d=this.catalog.resources.find(r=>r.id===def);if(d.family==='wood'){this.cylinder(g,.15,.34,4,'#725a3e',0,2,0,7);this.sphere(g,1.7,'#829251',0,4.4,0,[1,1.15,1]);this.sphere(g,1.1,'#91a25d',1,3.9,0);}else if(d.family==='ore'||d.family==='crystal'){this.mesh(new T.DodecahedronGeometry(.75,0),'#798579',[0,.45,0],g).scale.set(1.4,.8,1);for(let i=0;i<3;i++)this.mesh(new T.OctahedronGeometry(.25),this.mat(d.tier>2?'#b4c5ce':'#a1b6b2',{metalness:.5,emissive:'#617b74',emissiveIntensity:.15}),[(i-1)*.38,.8,0],g);}else if(def==='stone')this.sphere(g,.28,'#a5ac96',0,.14,0,[1.5,.7,1]);else if(def==='twig'){let twig=this.cylinder(g,.035,.065,1.1,'#9d8963',0,.09,0,5);twig.rotation.z=1.4;let tw=this.cylinder(g,.02,.03,.45,'#8d7a59',.17,.1,.16,5);tw.rotation.x=1.5;tw.rotation.z=.8;}else{for(let i=0;i<5;i++){let leaf=this.sphere(g,.23,d.family==='herb'?'#96ac72':d.family==='cotton'?'#dce0c8':'#9f9271',Math.cos(i)*.27,.18+Math.sin(i)*.05,Math.sin(i)*.27,[.4,1.2,.7]);leaf.rotation.z=i*.6;}if(d.family==='herb')this.sphere(g,.12,'#d6ba91',0,.55,0);}return g;}
 makeDungeon(){this.dungeon.name='dungeon';this.box(this.dungeon,50,.5,60,'#495750',0,-.27,0);for(let x=-24;x<25;x+=4)for(let z=-28;z<30;z+=4){let m=this.box(this.dungeon,3.9,.08,3.9,(x+z)%3?'#535f55':'#596457',x,.02,z);m.castShadow=false;}this.box(this.dungeon,1,7,60,'#394b44',-25,3.5,0);this.box(this.dungeon,1,7,60,'#394b44',25,3.5,0);this.box(this.dungeon,50,7,1,'#394b44',0,3.5,-30);this.box(this.dungeon,50,7,1,'#394b44',0,3.5,30);for(let x of [-20,20])for(let z of [-24,-12,0,12,24]){this.cylinder(this.dungeon,.8,1,7,'#677063',x,3.5,z,8);this.box(this.dungeon,2.6,.5,2.6,'#7d8470',x,6.8,z);this.box(this.dungeon,.3,1,.3,'#9e8960',x,2,z);let l=new T.PointLight('#f7bc6f',12,15,1.7);l.position.set(x,3,z);this.dungeon.add(l);}let portal=this.mesh(new T.TorusGeometry(2,.2,8,30),this.mat('#9aceba',{emissive:'#70c7ad',emissiveIntensity:.6}),[0,2,22],this.dungeon);portal.userData={kind:'leave',data:{id:'leave',name:'返回世界'}};this.cylinder(this.dungeon,2.3,2.6,.2,'#718375',0,.1,22,24);this.chest=new T.Group();this.dungeon.add(this.chest);this.chest.position.set(0,0,-25);this.chest.userData={kind:'chest',data:{id:'chest',name:'通關寶箱'}};this.box(this.chest,1.6,.9,1,'#97794c',0,.55,0);this.box(this.chest,1.7,.25,1.1,'#b9a275',0,1.1,0);for(let x of [-.55,.55])this.box(this.chest,.1,1,1.03,'#c9b57e',x,.6,0);this.box(this.chest,.3,.3,.1,'#d7c082',0,.8,.56);}
 makeWeapon(profession){if(this.hand)this.hand.parent?.remove(this.hand);this.hand=new T.Group();this.hand.position.set(.38,-.48,-.65);this.daggers=[];this.bowString=null;this.bowArrow=null;const registered=this.state?.player.weaponType||this.catalog.classes.find(c=>c.id===profession)?.type;const raw=registered||(['mage','battlemage','archmage','sage','priest','witch','necromancer'].includes(profession)?'staff':['ranger','hawkeye','windlord'].includes(profession)?'bow':'sword');this.weaponKind=raw;const type=['instrument','totem','focus'].includes(raw)?'staff':['cannon','toolgun'].includes(raw)?'mechanical':['shieldblade','daggers','gauntlets','chakram','hammer','spear','greatsword'].includes(raw)?'melee':raw;if(type==='mechanical'){this.box(this.hand,.2,.25,.65,'#5f6864',0,.1,-.2);this.cylinder(this.hand,.1,.1,.25,'#bf9961',0,.25,-.2);}else if(type==='melee'){if(raw==='shieldblade'){this.cylinder(this.hand,.25,.25,.08,'#9ba59a',-.2,.2,0,8);this.box(this.hand,.07,.55,.03,'#cbd0c7',.12,.35,0);}else if(raw==='daggers'){this.hand.position.set(0,0,0);for(const side of [-1,1]){const g=new T.Group();g.userData.side=side;this.hand.add(g);this.box(g,.055,.32,.025,'#d5ddd4',0,.18,0);this.box(g,.13,.035,.07,'#b3a174',0,.015,0);this.box(g,.07,.14,.07,'#655b48',0,-.065,0);this.daggers.push(g);}}else if(raw==='gauntlets'){this.box(this.hand,.22,.25,.3,'#aaa995',0,.1,0);}else{this.box(this.hand,.08,raw==='spear'?1.2:.65,.04,'#c8cfbc',0,.4,0);this.box(this.hand,.3,.08,.1,'#998764',0,.05,0);}}else if(type==='sword'){this.box(this.hand,.075,.65,.025,this.mat('#d9ddd1',{metalness:.75,roughness:.25}),0,.4,0);this.box(this.hand,.28,.05,.06,'#b5a576',0,.08,0);this.box(this.hand,.055,.21,.055,'#625e42',0,-.05,0);this.hand.rotation.z=-.25;}else if(type==='staff'){this.cylinder(this.hand,.033,.05,1.7,'#7f6c48',0,.3,0);this.mesh(new T.TorusGeometry(.18,.035,6,24),'#c3b27b',[0,1.1,0],this.hand);this.sphere(this.hand,.095,this.mat('#b9d8df',{emissive:'#77bacd',emissiveIntensity:1.2}),0,1.1,0);}else{let b=this.mesh(new T.TorusGeometry(.48,.035,7,30,Math.PI),'#a79367',[0,.18,0],this.hand);b.rotation.z=-Math.PI/2;const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute([0,-.3,0,0,.18,0,0,.66,0],3));this.bowString=new T.Line(g,new T.LineBasicMaterial({color:'#eee1bd'}));this.hand.add(this.bowString);this.bowArrow=new T.Group();this.box(this.bowArrow,.018,.018,.65,'#d6c39a',0,.18,-.17);const tip=this.mesh(new T.ConeGeometry(.035,.11,4),'#d3d9d2',[0,.18,-.54],this.bowArrow);tip.rotation.x=-Math.PI/2;this.hand.add(this.bowArrow);}attachViewmodel(this.hand,this.renderer.software?this.camera:this.weaponCamera,this.renderer.software);this.weapon=profession;this.hand.traverse(o=>{if(o.isMesh){o.castShadow=false;o.receiveShadow=false;o.material.depthTest=!this.renderer.software;}});}
 update(state){const old=this.state;if(!old||old._sync?.epoch!==state._sync?.epoch){this.remoteClock=state.now;for(const g of this.entities.values())if(g.userData.track)g.userData.track.samples=[];for(const g of this.npcObjects.values())g.userData.track.samples=[];}this.state=state;this.environment.sync(state);this.remoteSnapshotAt=performance.now()/1000;this.preview.reconcile(state,performance.now()/1000,this.networkRTT);let p=state.player;
   // Pin the test goblin in front of the player on first world sync so it
   // cannot be lost outside the village or behind scenery.
   if(this.goblinPreview && p?.scene==='world' && !this.goblinPreview.userData.spawnPlaced){
     const forward=new T.Vector3(0,0,-1).applyAxisAngle(new T.Vector3(0,1,0),this.yaw);
     const x=p.position.x+forward.x*5,z=p.position.z+forward.z*5;
     this.goblinPackRoot.position.set(x,this.landHeight(x,z),z);
     this.goblinPreview.userData.spawnPlaced=true;
     console.info('[Aetherwild] Goblin test placed near player',x,z);
   }
   this.active=true;this.root.visible=p.scene==='world';this.ambient.visible=p.scene==='world';this.dungeon.visible=p.scene!=='world';this.sky.visible=p.scene==='world';if(this.weapon!==p.profession)this.makeWeapon(p.profession);if(!old||old.player.scene!==p.scene||Math.hypot(this.camera.position.x-p.position.x,this.camera.position.z-p.position.z)>22)this.camera.position.set(p.position.x,p.position.y+1.68,p.position.z);let needed=new Set();for(const type of ['monsters','resources','players','objectives'])for(const e of state[type]||[]){const key=type+':'+e.id;needed.add(key);let g=this.entities.get(key);if(!g){g=type==='monsters'?(e.boss?this.wolf(true,e.definition.replace('boss_','')):(e.definition==='goblin'?this.goblinTest(e):this.wolf(false,e.definition.replace('boss_','')))):type==='resources'?this.resource(e.definition):type==='players'?this.human(e.profession,e.id):new T.Group();if(type==='objectives'){this.mesh(new T.OctahedronGeometry(.65),this.mat('#a7d5c7',{emissive:'#8ec7b3',emissiveIntensity:.7}),[0,1.5,0],g);this.cylinder(g,.8,1,.5,'#7d8b77',0,.25,0,8);}if(type==='monsters'){let l=this.label(type==='players'?e.name:(e.boss?e.name:'Lv.'+e.level+' '+e.name),e.boss?'#e3c690':'#d9e3cf',e.boss?1.8:.85);l.position.y=e.boss?6.4:2.1;g.add(l);}this.entityRoot.add(g);this.entities.set(key,g);g.position.set(e.position.x,e.position.y,e.position.z);}g.userData={...g.userData,kind:type==='monsters'?'monster':type==='resources'?'resource':type==='players'?'player':'objective',data:e,desired:new T.Vector3(e.position.x,e.position.y,e.position.z)};if(type==='players'){dressWeapon(g,e.weapon);this.nameplates.register(key,g,e.name||'旅人');}g.userData.track ||= new RemoteTrack();g.userData.track.push(e.position,state.now);g.visible=type==='players'&&Math.hypot(e.position.x-p.position.x,e.position.z-p.position.z)<1.1?false:type==='monsters'?e.hp>0:type==='resources'?e.readyAt<=state.now:true;if(type==='monsters'||type==='players')g.userData.desiredYaw=(e.yaw||0)+(type==='players'?Math.PI:0);if(type==='objectives')g.children[0].material.emissiveIntensity=e.done?.04:.7;}
 for(const[key,g]of this.entities)if(!needed.has(key)){this.entityRoot.remove(g);g.traverse(o=>{o.geometry?.dispose();if(o.material&&!o.material.map)o.material.dispose();});this.entities.delete(key);this.nameplates.remove(key);}
 if(state.lifts?.[0])this.liftRoot.position.y=state.lifts[0].position.y;this.chest.visible=!!state.run&&!state.run.claimed;this.world.fog.density=p.scene==='world'?2.6/this.renderDistance:.013;this.renderer.toneMappingExposure=Number(localStorage.getItem('aw-brightness')||1.05)*(p.scene==='world'?1:1.4);this.makeScenery(p.position.x,p.position.z);updateContentScene(this,state);updateCastTelegraphs(this,state);}
 aim(){this.ray.setFromCamera(this.mouse,this.camera);this.ray.far=45;const roots=[...this.entityRoot.children.filter(o=>o.visible)];if(this.root.visible)roots.push(...this.root.children.filter(o=>o.visible&&o.userData.kind&&o.position.distanceTo(this.camera.position)<55));if(this.dungeon.visible)roots.push(...this.dungeon.children.filter(o=>o.userData.kind));const hits=this.ray.intersectObjects(roots,true);let found=null;for(const hit of hits){let o=hit.object;while(o&&!o.userData.kind)o=o.parent;if(o&&o.visible&&hit.distance<45){found={kind:o.userData.kind,data:o.userData.data,distance:hit.distance,object:o};break;}if(hit.distance<4)break;}this.target=found;this.cb.target?.(found);}

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
 animate(){requestAnimationFrame(()=>this.animate());if(this.renderer.software&&performance.now()-(this.lastRender||0)<33)return;this.lastRender=performance.now();let dt=Math.min(.1,this.clock.getDelta());this.time+=dt;
   // Fixed midday lighting for the preview build.
   this.sun.position.set(-35,180,-25);this.sun.intensity=3.4;
   this.hemisphere.intensity=2.05;
   this.goblinAiAccumulator=(this.goblinAiAccumulator||0)+dt;
   if(this.goblinAiAccumulator>=.05){this.updateGoblinAI(Math.min(.1,this.goblinAiAccumulator));this.goblinAiAccumulator=0;}
   // Goblin test has no embedded clips: animate its rigid model safely.
   for(const preview of this.goblinPack||[]){
     if(!preview.userData.animatedModel)continue;
     const player=this.state?.player;
     const wx=preview.position.x+(this.goblinPackRoot?.position.x||0),wz=preview.position.z+(this.goblinPackRoot?.position.z||0);
     const nearby=player?.scene==='world'&&Math.hypot(player.position.x-wx,player.position.z-wz)<28;
     preview.visible=nearby;
     if(!nearby)continue;
     const model=preview.userData.animatedModel;
     const ai=preview.userData.ai,walking=!!ai?.moving;
     const stride=Math.sin(this.time*11);
     const throwing=ai?.throwUntil>this.time,gathering=ai?.gatherUntil>this.time;
     model.position.y=preview.userData.baseModelY+(walking?Math.abs(stride)*.025:Math.sin(this.time*2.2)*.008)+(gathering?-.025:0);
     model.rotation.y=preview.userData.baseModelYaw+(walking?Math.sin(this.time*5.5)*.018:Math.sin(this.time*.8)*.018);
     model.rotation.x=throwing?-.1+release*.22:gathering?.2:walking?stride*.06:0;
     model.rotation.z=walking?Math.sin(this.time*5.5)*.015:Math.sin(this.time*1.35)*.008;
     // Real skeletal animation: drive both legs, knees, arms and elbows.
     const joints=preview.userData.joints||{},rest=preview.userData.restPose||{};
     const phase=this.time*10+preview.userData.packIndex*.7;
     const swing=walking?Math.sin(phase)*.24:0;
     const bend=walking?Math.max(0,Math.sin(phase))*.2:0;
     const throwProgress=throwing?Math.max(0,Math.min(1,1-(ai.throwUntil-this.time)/.55)):0;
     const windup=throwing?Math.sin(Math.min(1,throwProgress*2)*Math.PI/2):0;
     const release=throwing?Math.max(0,Math.sin((throwProgress-.35)*Math.PI/ .65)):0;
     const gatherPhase=gathering?Math.sin((ai.gatherUntil-this.time)/.7*Math.PI):0;
     const pose=(name,x=0,y=0,z=0)=>{const bone=joints[name],base=rest[name];if(!bone||!base)return;bone.quaternion.copy(base).multiply(new T.Quaternion().setFromEuler(new T.Euler(x,y,z)));};
     pose('LeftUpLeg',swing+gatherPhase*.45);pose('RightUpLeg',-swing+gatherPhase*.45);
     pose('LeftLeg',-bend);pose('RightLeg',walking?-Math.max(0,-Math.sin(phase))*.2:0);
     pose('LeftFoot',walking?Math.sin(phase)*.07:0);pose('RightFoot',walking?-Math.sin(phase)*.07:0);
     pose('LeftArm',-swing*.75+gatherPhase*.35+windup*.12);pose('RightArm',swing*.75+windup*.35-release*.9+gatherPhase*.4,0,-windup*.25);
     pose('LeftForeArm',-.12-gatherPhase*.5);pose('RightForeArm',-.15-windup*.9+release*.65-gatherPhase*.6);
     pose('Spine',gatherPhase*.15+windup*.12-release*.22,windup*-.22+release*.32);
     // Keep the lowest animated foot at or above its original grounded position.
     // Avoid full skinned-mesh bounding-box recalculation on every frame.
     // Foot grounding is in WORLD coordinates, not model-local coordinates.
     // Previous code mixed the two spaces, so it could not correct terrain penetration.
     // Ground against the DEFORMED skinned geometry, not ankle bone positions.
     // Reset to the original calibrated height each frame to avoid accumulated lift.
     model.position.y=preview.userData.baseModelY;
     model.updateWorldMatrix(true,true);
     const bounds=new T.Box3();
     let hasSkin=false;
     model.traverse(node=>{
       if(!node.isSkinnedMesh)return;
       node.computeBoundingBox();
       if(!node.boundingBox||node.boundingBox.isEmpty())return;
       const worldBox=node.boundingBox.clone().applyMatrix4(node.matrixWorld);
       bounds.union(worldBox);
       hasSkin=true;
     });
     if(hasSkin && Number.isFinite(bounds.min.y)){
       const center=new T.Vector3();
       preview.getWorldPosition(center);
       const floor=this.landHeight(center.x,center.z);
       // Ground clearance of 2cm; raise the entire skinned model, never the skeleton alone.
       const lift=floor+.02-bounds.min.y;
       if(Math.abs(lift)<3)model.position.y+=lift;
     }

   }
   for(const npc of this.npcObjects.values()){if(npc.userData.eileenTest){
        const player=this.state?.player;
        const near=player?.scene==='world'&&Math.hypot(player.position.x-npc.position.x,player.position.z-npc.position.z)<5;
         const model=npc.userData.modelInstance;
         const walking=!near&&!!model&&!!npc.userData.modelClips?.some(c=>/walk|run|行走/i.test(c.name));
         if(model){const phase=this.time*.48;const tx=walking?Math.sin(phase)*1.1:0;const tz=walking?Math.cos(phase)*.65-.65:0;const blend=Math.min(1,dt*2.5);model.position.x+=(tx-model.position.x)*blend;model.position.z+=(tz-model.position.z)*blend;}
        if(near){
          const desired=Math.atan2(player.position.x-npc.position.x,player.position.z-npc.position.z);
          let delta=Math.atan2(Math.sin(desired-npc.rotation.y),Math.cos(desired-npc.rotation.y));
          npc.rotation.y+=Math.max(-dt*2.4,Math.min(dt*2.4,delta));
        }else{
          const delta=Math.atan2(Math.sin(-npc.rotation.y),Math.cos(-npc.rotation.y));
          npc.rotation.y+=Math.max(-dt*.8,Math.min(dt*.8,delta));
        }
        const mixer=npc.userData.modelMixer,clips=npc.userData.modelClips||[];
        if(mixer){
          const selected=near?clips.find(c=>/talk|speak|greet|交談/i.test(c.name)):(walking?clips.find(c=>/walk|run|行走/i.test(c.name)):null);
          const next=selected||clips.find(c=>/idle|breathe|待機/i.test(c.name))||clips[0];
          if(next&&npc.userData.activeClip!==next){
            const previous=npc.userData.activeClip;
            if(previous)mixer.clipAction(previous).fadeOut(.2);
            mixer.clipAction(next).reset().fadeIn(.2).play();
            npc.userData.activeClip=next;
          }
          mixer.update(dt);
          // Idle/Walk/Talk clips omit upper-arm rotations: restore relaxed pose each frame.
          for (const shoulder of npc.userData.eileenShoulders || []) {
            shoulder.bone.rotation.z = shoulder.angle;
          }

        }
        if(model&&!mixer){model.position.y=Math.sin(this.time*1.65)*.025;model.rotation.y=Math.sin(this.time*.55)*.07;}
      }}for(const a of [...this.arrowTrails]){a.life-=dt;a.g.position.addScaledVector(a.direction,dt*80);if(a.life<=0){this.world.remove(a.g);a.g.traverse(o=>{o.geometry?.dispose();o.material?.dispose();});this.arrowTrails.splice(this.arrowTrails.indexOf(a),1);}}if(this.state){let p=this.state.player;this.remoteClock=advanceRemoteClock(this.remoteClock,this.state.now+Math.min(.25,performance.now()/1000-this.remoteSnapshotAt),dt);const rate=1-Math.exp(-dt*18);const k=this.keys,input={x:Number(k.has('KeyD')||k.has('ArrowRight'))-Number(k.has('KeyA')||k.has('ArrowLeft')),z:Number(k.has('KeyW')||k.has('ArrowUp'))-Number(k.has('KeyS')||k.has('ArrowDown')),yaw:this.yaw,sprint:k.has('ShiftLeft')};const predicted=this.preview.advance(dt,performance.now()/1000,input);const pos=new T.Vector3(predicted?.x??p.position.x,(predicted?.y??p.position.y)+1.68,predicted?.z??p.position.z);if(predicted)this.camera.position.copy(pos);else this.camera.position.lerp(pos,rate);this.camera.rotation.set(this.pitch,this.yaw,0);this.cb.heading?.(this.yaw);this.environment.update(p.position,p.scene==='world');this.sky.position.copy(this.camera.position);this.cityScene.update(p.position,1-(this.lighting?.daylight||0),p.scene==='world');for(const g of this.entities.values()){let desired=g.userData.desired;if(desired){let moving=g.position.distanceTo(desired)>.03;const pose=g.userData.track?.at(this.remoteClock-.15,.1);if(pose)g.position.set(pose.x,pose.y,pose.z);else g.position.lerp(desired,rate);if(g.userData.desiredYaw!==undefined){const delta=Math.atan2(Math.sin(g.userData.desiredYaw-g.rotation.y),Math.cos(g.userData.desiredYaw-g.rotation.y));g.rotation.y+=delta*rate;}if(g.userData.legs)g.userData.legs.forEach((l,i)=>l.rotation.x=moving?Math.sin(this.time*10+i*Math.PI)*.4:0);}if(g.userData.kind==='objective')g.children[0].rotation.y=this.time;}
 for(const npc of this.npcObjects.values()){const pose=npc.userData.track?.at(this.remoteClock-1.05);if(pose){const dx=pose.x-npc.position.x,dz=pose.z-npc.position.z;if(Math.hypot(dx,dz)>.001)npc.rotation.y=Math.atan2(dx,dz);npc.position.set(pose.x,pose.y,pose.z);for(const [i,leg]of (npc.userData.legs||[]).entries())leg.rotation.x=Math.hypot(dx,dz)>.001?Math.sin(this.time*7+i*Math.PI)*.12:0;}}
 const walking=this.keys.size>0&&document.pointerLockElement===this.canvas;this.hand.position.y=-.48+(walking?Math.sin(this.time*9)*.015:Math.sin(this.time*1.4)*.009);const feedback=this.feedback.sample(performance.now()/1000);this.swing=Math.max(0,this.swing-(feedback.hold?0:dt*5));this.hand.position.z=-.65+Math.sin(this.swing*Math.PI)*.12+feedback.recoil*.045;this.hand.rotation.x=-Math.sin(this.swing*Math.PI)*1.35-feedback.recoil*.12;this.hand.rotation.z=(this.weapon==='warrior'?-.25:0)-Math.sin(this.swing*Math.PI)*.6; if(this.weaponKind==='daggers'){this.hand.position.set(0,0,0);this.hand.rotation.set(0,0,0);const poses=weaponPose('daggers',this.camera.aspect);for(const g of this.daggers){const side=g.userData.side,pose=side<0?poses.left:poses.right;g.position.set(...pose);g.position.y+=(walking?Math.sin(this.time*9+side)*.009:0)+Math.sin(this.swing*Math.PI)*.07;g.rotation.set(-.3-Math.sin(this.swing*Math.PI)*.5,side*.12,side*-.2);}}
 if(this.bowString){const charge=this.bowDrawing?Math.min(1,(performance.now()/1000-this.bowStarted)/1.2):0;this.bowString.geometry.attributes.position.setXYZ(1,0,.18,.28*charge);this.bowString.geometry.attributes.position.needsUpdate=true;this.bowArrow.position.z=.28*charge;this.hand.position.x=.32-.14*charge;this.hand.position.y=-.38+.07*charge;this.hand.rotation.x=-.06*charge+.08*Math.sin(this.swing*Math.PI);this.hand.rotation.z=-.04*charge-.05*Math.sin(this.swing*Math.PI);this.bowArrow.visible=this.bowDrawing||this.swing===0;}
if(this.time-(this.lastCull||0)>.2){this.lastCull=this.time;for(const o of this.root.children){if(o===this.sky||o===this.globalTerrain||o===this.contentRoot||o===this.contentProps||o===this.capital)continue;const radius=o.userData.cullRadius||0;o.visible=Math.hypot(o.position.x-p.position.x,o.position.z-p.position.z)<this.renderDistance+radius;}}this.aim();}else{this.camera.position.set(7,3.4,41);this.camera.lookAt(0,2,13);this.sun.target.position.set(0,0,24);this.environment.update({x:0,y:0,z:24},true);}renderFrame(this.renderer,this.world,this.camera,this.weaponScene,this.weaponCamera);this.nameplates.update(this.camera,this.state?.player.scene==='world');this.renderedFrames=(this.renderedFrames||0)+1;const now=performance.now();this.fpsSince??=now;if(now-this.fpsSince>=1000){this.fps=this.renderedFrames*1000/(now-this.fpsSince);if(this.renderer.software)this.renderer.drawDistance=this.fps<20?140:this.fps<27?200:Math.min(280,this.renderDistance);this.renderedFrames=0;this.fpsSince=now;}}
 startSwing(){this.swing=1;this.feedback.swing(performance.now()/1000);}
 confirmHit(id,amount){if(!this.feedback.hit(performance.now()/1000,amount))return;const object=this.entities.get('monsters:'+id);if(object){if(!object.userData.flashBase){const saved=[];object.traverse(o=>{if(o.isMesh&&o.material?.emissive)saved.push([o.material,o.material.emissive.getHex(),o.material.emissiveIntensity]);});object.userData.flashBase=saved;}for(const [m]of object.userData.flashBase){m.emissive.set('#d68d51');m.emissiveIntensity=.65;}clearTimeout(object.userData.flashTimer);object.userData.flashTimer=setTimeout(()=>{for(const [m,color,intensity]of object.userData.flashBase||[]){m.emissive.setHex(color);m.emissiveIntensity=intensity;}delete object.userData.flashBase;},90);}}

 landHeight(x,z){return gridHeight(this.state?.content?.terrainPatch,x,z)??gridHeight(this.catalog.terrain,x,z)??0;}
 makeCapital(){const r=this.catalog.regions.find(r=>r.id==='aw_crownreach');if(!r)return;const g=this.capital=new T.Group();g.position.set(r.position.x,r.position.y,r.position.z);this.root.add(g);for(const f of [.38,.7,1]){const wall=this.mesh(new T.CylinderGeometry(r.radius*f,r.radius*f,14,96,1,true),this.mat('#aaa98f',{side:T.DoubleSide}),[0,7,0],g);wall.castShadow=false;}for(let i=0;i<8;i++){const road=this.box(g,18,.08,r.radius*2,'#c1b68d',0,.04,0);road.rotation.y=i*Math.PI/8;road.castShadow=false;}for(let ring=1;ring<=4;ring++)for(let i=0;i<24;i++){const a=(i+.5)*Math.PI/12,rad=r.radius*(ring/5),h=25+(i%4)*9;const house=this.box(g,65,h,85,'#b8b19a',Math.cos(a)*rad,h/2,Math.sin(a)*rad);house.castShadow=false;}this.cylinder(g,85,110,90,'#d4ccb0',0,45,-170,16);for(const x of [-110,110])this.cylinder(g,25,35,160,'#c8c2a9',x,80,-160,12);}
 applySettings(){this.renderDistance=Math.max(300,Math.min(8000,Number(localStorage.getItem('aw-render-distance'))||1800));this.camera.far=Math.max(1500,this.renderDistance+500);this.camera.updateProjectionMatrix();if(this.world?.fog)this.world.fog.density=2.6/this.renderDistance;if(this.renderer)this.renderer.shadowMap.enabled=localStorage.getItem('aw-shadows')!=='0';if(this.renderer?.software)this.renderer.drawDistance=Math.min(350,this.renderDistance);}
 resize(){this.weaponCamera.aspect=innerWidth/innerHeight;this.weaponCamera.updateProjectionMatrix();this.camera.aspect=innerWidth/innerHeight;this.camera.updateProjectionMatrix();this.renderer.setSize(innerWidth,innerHeight);}
 capture(){this.canvas.requestPointerLock?.()?.catch?.(()=>this.cb.toast?.('點擊畫面以啟用滑鼠環視。'));}
}
