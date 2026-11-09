import * as T from '../../vendor/three.module.js';
import {GLTFLoader} from '../../vendor/GLTFLoader.js';
let shared;
const load=()=>shared??=Promise.all(['grass_medium_01','shrub_01'].map(name=>new GLTFLoader().loadAsync('/assets/models/environment/'+name+'.glb')));
export function plantPlacements(points,kind){return points.filter(p=>kind==='grass'?p.variation<.15:p.variation>.975).slice(0,kind==='grass'?180:14);}
// Reuse scanned geometry and materials; only instance matrices change with native height tiles.
export class ScannedVegetation {
 constructor(parent,points,center,height,{onReady=()=>{},loadModels=load}={}){this.parent=parent;this.points=points;this.center=center;this.height=height;this.groups=[];this.ready=false;this.promise=loadModels().then(models=>{for(let i=0;i<models.length;i++)this.install(models[i].scene,i?'shrub':'grass');this.ready=true;this.update();onReady();return true;}).catch(error=>{this.error=error.message;return false;});}
 install(source,kind){source.updateMatrixWorld(true);const bounds=new T.Box3().setFromObject(source),center=bounds.getCenter(new T.Vector3()),normalization=new T.Matrix4().makeTranslation(-center.x,-bounds.min.y,-center.z),scale=(kind==='grass'?.48:.8)/Math.max(.001,bounds.max.y-bounds.min.y),points=plantPlacements(this.points,kind);source.traverse(node=>{if(!node.isMesh)return;const mesh=new T.InstancedMesh(node.geometry,node.material,points.length);mesh.name='polyhaven-'+kind;mesh.castShadow=kind==='shrub';mesh.receiveShadow=true;mesh.userData={points,base:normalization.clone().multiply(node.matrixWorld),scale,kind};this.parent.add(mesh);this.groups.push(mesh);});}
 update(){const dummy=new T.Object3D();for(const mesh of this.groups){const {points,base,scale}=mesh.userData;for(let i=0;i<points.length;i++){const p=points[i],height=this.height(p.x,p.z);dummy.position.set(p.x-this.center.x,Number.isFinite(height)?height:0,p.z-this.center.z);dummy.rotation.set(0,p.rotation,0);dummy.scale.setScalar(scale*(.85+p.variation*.3));dummy.updateMatrix();mesh.setMatrixAt(i,dummy.matrix.clone().multiply(base));}mesh.instanceMatrix.needsUpdate=true;mesh.computeBoundingSphere();}}
}
