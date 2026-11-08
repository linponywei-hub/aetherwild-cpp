// Authoring document only. Publishing gameplay collision requires C++ validation.
export const FORMAT='aetherwild-world-draft';
export const MAX_BYTES=150*1024*1024;
export const MAX_ASSET_BYTES=50*1024*1024;
export function emptyProject(){return {format:FORMAT,version:1,assets:[],objects:[]};}
const vector=(v,positive=false)=>Array.isArray(v)&&v.length===3&&v.every(n=>Number.isFinite(n)&&Math.abs(n)<=1000000&&(!positive||n>=0.001));
export function validateProject(p){
 if(!p||p.format!==FORMAT||p.version!==1||!Array.isArray(p.assets)||!Array.isArray(p.objects)||p.assets.length>200||p.objects.length>2000)throw Error('場景格式或版本不正確');
 const ids=new Set();let total=0;
 for(const a of p.assets){if(!a||typeof a.id!=='string'||ids.has(a.id)||typeof a.name!=='string'||!(a.bytes instanceof ArrayBuffer)||a.bytes.byteLength>MAX_ASSET_BYTES)throw Error('模型資產不正確或超過 50 MB');ids.add(a.id);total+=a.bytes.byteLength;}
 if(total>MAX_BYTES)throw Error('場景模型合計超過 150 MB');
 const objects=new Set();
 for(const o of p.objects){
  if(!o||typeof o.id!=='string'||objects.has(o.id)||!ids.has(o.assetId)||typeof o.name!=='string'||!vector(o.position)||!vector(o.rotation)||!vector(o.scale,true)||!Array.isArray(o.colliders)||o.colliders.length>100)throw Error('物件資料不正確');
  objects.add(o.id);const colliders=new Set();
  for(const c of o.colliders){if(!c||typeof c.id!=='string'||colliders.has(c.id)||!vector(c.position)||!vector(c.rotation)||!vector(c.size,true))throw Error('碰撞箱資料不正確');colliders.add(c.id);}
 }
 return p;
}
export function checkGLB(bytes){
 if(!(bytes instanceof ArrayBuffer)||bytes.byteLength<20||bytes.byteLength>MAX_ASSET_BYTES)throw Error('GLB 檔案大小須介於 20 bytes 與 50 MB');
 const v=new DataView(bytes);if(v.getUint32(0,true)!==0x46546c67||v.getUint32(4,true)!==2||v.getUint32(8,true)!==bytes.byteLength)throw Error('不是有效的 GLB 2.0 檔案');
}
export function newObject(assetId,name){return {id:crypto.randomUUID(),assetId,name,position:[0,0,0],rotation:[0,0,0],scale:[1,1,1],colliders:[]};}
export function newCollider(){return {id:crypto.randomUUID(),position:[0,1,0],rotation:[0,0,0],size:[2,2,2]};}
