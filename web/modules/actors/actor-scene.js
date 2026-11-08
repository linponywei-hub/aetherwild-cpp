import * as T from '../../vendor/three.module.js';
import {batchParts} from '../rendering/mesh-batch.js';
import {weaponParts,humanoidParts,nameplateVisible} from './actor-model.js';
export function humanoid(role,id){const root=new T.Group(),p=humanoidParts(role,id);root.add(batchParts(p.body));root.userData.legs=[];for(const leg of p.legs){const mesh=batchParts(leg);root.add(mesh);root.userData.legs.push(mesh);}return root;}
export function dressWeapon(root,descriptor){const signature=JSON.stringify(descriptor||null);if(root.userData.weaponSignature===signature)return;root.userData.weaponSignature=signature;
 const old=root.userData.weaponObject;if(old){root.remove(old);old.traverse(o=>{o.geometry?.dispose();o.material?.dispose();});}
 const parts=weaponParts(descriptor?.type||'unarmed');if(!parts.length){root.userData.weaponObject=null;return;}
 const weapon=batchParts(parts);weapon.position.set(descriptor.type==='daggers'?0:.36,1,.15);weapon.scale.setScalar(.8);root.add(weapon);root.userData.weaponObject=weapon;
}
export class Nameplates {
 constructor(){this.entries=new Map();this.root=document.createElement('div');this.root.id='nameplate-layer';document.body.append(this.root);this.point=new T.Vector3();}
 remove(key){const entry=this.entries.get(key);if(entry){entry.node.remove();this.entries.delete(key);}}
 register(key,object,text,npc=false){let entry=this.entries.get(key);if(!entry){const node=document.createElement('div');node.className='actor-nameplate'+(npc?' npc-nameplate':'');this.root.append(node);entry={node,object,npc};this.entries.set(key,entry);}entry.node.textContent=text;entry.object=object;}
 update(camera,world=true){const w=innerWidth,h=innerHeight;for(const e of this.entries.values()){const o=e.object;if(!o.visible||!o.parent||(e.npc&&!world)||!nameplateVisible(o.position,camera.position,e.npc?45:90)){e.node.hidden=true;continue;}this.point.copy(o.position);this.point.y+=e.npc?2.5:2.45;this.point.project(camera);const visible=this.point.z>-1&&this.point.z<1&&Math.abs(this.point.x)<1.05&&Math.abs(this.point.y)<1.05;e.node.hidden=!visible;if(visible)e.node.style.transform=`translate(${(this.point.x*.5+.5)*w}px,${(-this.point.y*.5+.5)*h}px) translate(-50%,-100%)`;}}
}
