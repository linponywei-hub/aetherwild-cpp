export const VILLAGE_KEY='aw-village-glb-transforms-v1';
const vector=(v,positive=false)=>Array.isArray(v)&&v.length===3&&v.every(n=>typeof n==='number'&&Number.isFinite(n)&&Math.abs(n)<=100000&&(!positive||n>=.0001&&n<=1000));
export function validateVillagePose(p,complete=false){
 if(!p||typeof p!=='object'||!vector(p.position)||!vector(p.rotation)||!vector(p.scale,true)||(p.modelPosition!==undefined&&!vector(p.modelPosition))||(complete&&!vector(p.modelPosition))||(p.rotationOrder!==undefined&&p.rotationOrder!=='XYZ')||(p.serverRevision!==undefined&&(!Number.isSafeInteger(p.serverRevision)||p.serverRevision<0)))throw Error('房屋姿態資料不正確；原設定未更動');
 return structuredClone(p);
}
export function validateVillagePoses(data){
 if(!data||typeof data!=='object'||Array.isArray(data))throw Error('房屋設定格式不正確');const out={};
 for(const [id,p] of Object.entries(data)){if(!/^[1-5]$/.test(id))throw Error('只能匯入房屋 01–05');out[id]=validateVillagePose(p);}return out;
}
export function readVillagePoses(storage=globalThis.localStorage){const raw=storage?.getItem(VILLAGE_KEY);return raw?validateVillagePoses(JSON.parse(raw)):{};}
export function resolveVillagePose(index,layout,storage=globalThis.localStorage){
 const local=readVillagePoses(storage);if(local[index]&&!(Number.isInteger(local[index].serverRevision)&&layout?.revision>local[index].serverRevision&&layout?.poses?.[index]))return {pose:local[index],source:'local'};
 const saved=layout?.poses?.[index];return saved?{pose:validateVillagePose(saved,true),source:'server'}:null;
}
export function captureVillagePose(group,model){return validateVillagePose({position:group.position.toArray(),modelPosition:model.position.toArray(),rotation:[model.rotation.x,model.rotation.y,model.rotation.z],rotationOrder:'XYZ',scale:model.scale.toArray()},true);}
export function applyPose(p,group,model){validateVillagePose(p);group.position.fromArray(p.position);if(p.modelPosition)model.position.fromArray(p.modelPosition);model.rotation.set(p.rotation[0],p.rotation[1],p.rotation[2],'XYZ');model.scale.fromArray(p.scale);group.updateMatrixWorld(true);}
export function saveLocalPose(index,pose,storage=globalThis.localStorage){const data=readVillagePoses(storage);data[index]=validateVillagePose(pose,true);const raw=storage.getItem(VILLAGE_KEY);if(raw)storage.setItem(VILLAGE_KEY+'-backup',raw);storage.setItem(VILLAGE_KEY,JSON.stringify(data));}
