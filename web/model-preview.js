import * as T from '/vendor/three.module.js';
import {ModelAssets} from '/model-assets.js';
import {MODEL_VARIANTS} from '/model-catalog.js';
const select=document.querySelector('#variant'),status=document.querySelector('#status'),viewport=document.querySelector('#viewport');
for(const [id,v] of Object.entries(MODEL_VARIANTS)){const option=document.createElement('option');option.value=id;option.textContent=id+'｜'+v.displayName;select.append(option);}
const scene=new T.Scene();scene.background=new T.Color('#253733');
const camera=new T.PerspectiveCamera(50,1,.05,300);camera.position.set(0,2,5);
scene.add(new T.HemisphereLight('#ffffff','#566455',2));
const light=new T.DirectionalLight('#fff1d6',3);light.position.set(4,9,7);scene.add(light);
const renderer=new T.WebGLRenderer({antialias:true});renderer.outputColorSpace=T.SRGBColorSpace;viewport.append(renderer.domElement);
const grid=new T.GridHelper(12,24,'#567e6b','#375147');scene.add(grid);
const root=new T.Group();scene.add(root);
const assets=new ModelAssets();let active=null;
async function load(){
  const id=select.value;status.textContent='載入 '+id+'…';
  if(active){root.remove(active);active=null;}
  try{const {instance}=await assets.instantiate(id);active=instance;root.add(instance);apply();status.textContent='載入成功：'+id+'\nMesh：'+countMeshes(instance)+'\n母模型：'+MODEL_VARIANTS[id].baseId;}
  catch(e){status.textContent='載入失敗：'+e.message+'\n請確認 GLB 已放入 web/assets/models/characters/';}
}
function countMeshes(o){let n=0;o.traverse(x=>{if(x.isMesh)n++;});return n;}
function apply(){if(!active)return;active.scale.setScalar(Number(document.querySelector('#scale').value));active.rotation.y=Number(document.querySelector('#yaw').value)*Math.PI/180;}
select.addEventListener('change',load);document.querySelector('#reload').addEventListener('click',()=>{assets.clearCache();load();});
document.querySelectorAll('input').forEach(el=>el.addEventListener('input',apply));
let dragging=false,lastX=0,angle=0;renderer.domElement.addEventListener('pointerdown',e=>{dragging=true;lastX=e.clientX;renderer.domElement.setPointerCapture(e.pointerId);});
renderer.domElement.addEventListener('pointerup',()=>dragging=false);
renderer.domElement.addEventListener('pointermove',e=>{if(dragging){angle+=(e.clientX-lastX)*.01;lastX=e.clientX;}});
function frame(){requestAnimationFrame(frame);const rect=viewport.getBoundingClientRect();if(rect.width&&rect.height){renderer.setSize(rect.width,rect.height,false);camera.aspect=rect.width/rect.height;camera.updateProjectionMatrix();}root.rotation.y=angle;renderer.render(scene,camera);}frame();load();
