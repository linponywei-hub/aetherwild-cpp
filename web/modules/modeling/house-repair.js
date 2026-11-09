import * as T from '../../vendor/three.module.js';
// Sidecars modify geometry only; original baked materials, UVs and user poses remain.
export function applyHouseRepair(scene,patch){
 const meshes=[];scene.traverse(o=>{if(o.isMesh&&o.geometry?.attributes.position)meshes.push(o);});
 if(patch?.version!==1||meshes.length!==1)throw Error('房屋修補資料版本不符');const mesh=meshes[0],source=mesh.geometry,pos=source.attributes.position;
 if(pos.count!==patch.vertices||!Array.isArray(patch.updates)||patch.updates.length%4||!Array.isArray(patch.triangles)||patch.triangles.length%3)throw Error('房屋修補網格不符');
 source.computeBoundingBox();const maxShift=source.boundingBox.getSize(new T.Vector3()).length()*.00401;
 for(let i=0;i<patch.updates.length;i+=4){const [id,x,y,z]=patch.updates.slice(i,i+4);if(!Number.isInteger(id)||id<0||id>=pos.count||![x,y,z].every(Number.isFinite)||Math.hypot(x-pos.getX(id),y-pos.getY(id),z-pos.getZ(id))>maxShift)throw Error('房屋接縫修補超出範圍');}
 if(patch.triangles.some(id=>!Number.isInteger(id)||id<0||id>=pos.count))throw Error('房屋補面索引不正確');
 const geometry=source.clone(),hadNormals=!!source.attributes.normal;if(!hadNormals)geometry.computeVertexNormals();const positions=geometry.attributes.position;for(let i=0;i<patch.updates.length;i+=4)positions.setXYZ(...patch.updates.slice(i,i+4));positions.needsUpdate=true;
 const original=source.index?Array.from(source.index.array):Array.from({length:pos.count},(_,i)=>i),removed=new Set(patch.removeTriangles||[]);if([...removed].some(i=>!Number.isInteger(i)||i<0||i>=original.length/3))throw Error('房屋退化面索引不正確');const indices=[...original.filter((_,i)=>!removed.has(Math.floor(i/3))),...patch.triangles],normals=geometry.attributes.normal;
 let flipped=0;if(normals){for(let i=0;i<indices.length;i+=3){const a=indices[i],b=indices[i+1],c=indices[i+2],abx=positions.getX(b)-positions.getX(a),aby=positions.getY(b)-positions.getY(a),abz=positions.getZ(b)-positions.getZ(a),acx=positions.getX(c)-positions.getX(a),acy=positions.getY(c)-positions.getY(a),acz=positions.getZ(c)-positions.getZ(a);const nx=normals.getX(a)+normals.getX(b)+normals.getX(c),ny=normals.getY(a)+normals.getY(b)+normals.getY(c),nz=normals.getZ(a)+normals.getZ(b)+normals.getZ(c);if((aby*acz-abz*acy)*nx+(abz*acx-abx*acz)*ny+(abx*acy-aby*acx)*nz<0){indices[i+1]=c;indices[i+2]=b;flipped++;}}}
 geometry.setIndex(indices);if(!hadNormals){geometry.computeVertexNormals();for(const material of Array.isArray(mesh.material)?mesh.material:[mesh.material]){material.flatShading=false;material.needsUpdate=true;}}geometry.userData.reorientedTriangles=flipped;geometry.computeBoundingBox();geometry.computeBoundingSphere();mesh.geometry=geometry;source.dispose();scene.userData.repaired=true;return geometry;
}
export async function loadRepairedHouse(loader,url,{fetcher=fetch,digest=globalThis.crypto?.subtle}={}){
 const [source,repair]=await Promise.all([fetcher(url),fetcher(url.replace(/\.glb$/,'.repair.json')).catch(()=>null)]);if(!source.ok)throw Error('房屋模型載入失敗');
 const bytes=await source.arrayBuffer(),gltf=await loader.parseAsync(bytes,'');
 try{if(!repair?.ok)throw Error('房屋接縫修補檔載入失敗');const patch=await repair.json();if(digest){const sha=[...new Uint8Array(await digest.digest('SHA-256',bytes))].map(x=>x.toString(16).padStart(2,'0')).join('');if(sha!==patch.sourceSha256)throw Error('房屋與接縫修補版本不一致');}applyHouseRepair(gltf.scene,patch);}
 catch(error){gltf.scene.userData.repairError=error.message;console.warn('[Aetherwild] House repair unavailable; preserving original mesh',error.message);}
 return gltf;
}

export function seatHouse(group,height,{force=false}={}){
 const model=group.userData?.villageGLB;if(!model||(!force&&group.userData.groundManual))return false;
 group.updateMatrixWorld(true);const samples=[],point=new T.Vector3();model.traverse(o=>{const positions=o.geometry?.attributes?.position;if(!o.isMesh||!positions)return;const stride=Math.max(1,Math.floor(positions.count/20000));for(let i=0;i<positions.count;i+=stride){point.fromBufferAttribute(positions,i).applyMatrix4(o.matrixWorld);if(Number.isFinite(point.y))samples.push(point.y);}});
 if(!samples.length)return false;samples.sort((a,b)=>a-b);const foundation=samples[Math.floor(samples.length*.02)];
 const h=height(group.position.x,group.position.z);if(!Number.isFinite(h))return false;const delta=h-foundation-.08;
 if(!Number.isFinite(delta))return false;group.position.y+=delta;group.userData.groundManual=false;group.updateMatrixWorld(true);return true;
}
