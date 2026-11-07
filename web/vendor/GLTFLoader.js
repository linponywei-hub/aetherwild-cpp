import * as T from './three.module.js';

// Lightweight GLB 2.0 static-mesh importer for the workstation.
// Handles embedded bufferViews, textures and PBR metallic-roughness.
// Animated/skinned meshes are deliberately rejected until the rigging stage.
const components={5120:[Int8Array,1],5121:[Uint8Array,1],5122:[Int16Array,2],5123:[Uint16Array,2],5125:[Uint32Array,4],5126:[Float32Array,4]};
const dimensions={SCALAR:1,VEC2:2,VEC3:3,VEC4:4,MAT4:16};
function accessor(json,bin,index) {
  const a=json.accessors[index],view=json.bufferViews[a.bufferView],info=components[a.componentType];
  if(!view||!info||!dimensions[a.type]||a.sparse)throw Error('Unsupported GLB accessor');
  const count=a.count*dimensions[a.type],offset=(view.byteOffset||0)+(a.byteOffset||0),bytes=info[1]*count;
  if(offset+bytes>bin.byteLength)throw Error('GLB accessor outside buffer');
  const raw=new Uint8Array(bin,offset,bytes);
  const copy=new Uint8Array(bytes);copy.set(raw);
  return {array:new info[0](copy.buffer),itemSize:dimensions[a.type],normalized:!!a.normalized};
}
function nodeTransform(o,n) {
  if(n.matrix){const m=new T.Matrix4().fromArray(n.matrix);m.decompose(o.position,o.quaternion,o.scale);}
  else {if(n.translation)o.position.fromArray(n.translation);if(n.rotation)o.quaternion.fromArray(n.rotation);if(n.scale)o.scale.fromArray(n.scale);}
}
export class GLTFLoader {
  async loadAsync(url) {
    const response=await fetch(url);
    if(!response.ok)throw Error('GLB HTTP '+response.status);
    const data=await response.arrayBuffer(),view=new DataView(data);
    if(view.getUint32(0,true)!==0x46546c67||view.getUint32(4,true)!==2||view.getUint32(8,true)!==data.byteLength)throw Error('Invalid GLB 2.0');
    let cursor=12,json=null,bin=null;
    while(cursor<data.byteLength){
      const length=view.getUint32(cursor,true),type=view.getUint32(cursor+4,true);cursor+=8;
      if(cursor+length>data.byteLength)throw Error('Invalid GLB chunk');
      if(type===0x4e4f534a)json=JSON.parse(new TextDecoder().decode(new Uint8Array(data,cursor,length)));
      if(type===0x004e4942)bin=data.slice(cursor,cursor+length);
      cursor+=length;
    }
    if(!json||!bin)throw Error('GLB missing JSON or BIN');
    if(json.skins?.length||json.animations?.length)throw Error('Rigged GLB requires animation importer');
    const objectUrls=[];
    const textures=await Promise.all((json.textures||[]).map(async tex=>{
      const image=json.images?.[tex.source];
      if(!image?.bufferView) {if(image?.bufferView!==0)throw Error('Only embedded GLB images supported');}
      const v=json.bufferViews[image.bufferView];
      const bytes=new Uint8Array(bin,v.byteOffset||0,v.byteLength);
      const blob=new Blob([bytes],{type:image.mimeType||'image/png'});
      const blobUrl=URL.createObjectURL(blob);objectUrls.push(blobUrl);
      const bitmap=await createImageBitmap(blob);
      const texture=new T.CanvasTexture(bitmap);
      texture.flipY=false;
      const sampler=json.samplers?.[tex.sampler]||{};
      texture.wrapS=sampler.wrapS===33071?T.ClampToEdgeWrapping:sampler.wrapS===33648?T.MirroredRepeatWrapping:T.RepeatWrapping;
      texture.wrapT=sampler.wrapT===33071?T.ClampToEdgeWrapping:sampler.wrapT===33648?T.MirroredRepeatWrapping:T.RepeatWrapping;
      texture.needsUpdate=true;
      return texture;
    }));
    objectUrls.forEach(URL.revokeObjectURL);
    const materials=(json.materials||[]).map(m=>{
      const p=m.pbrMetallicRoughness||{},c=p.baseColorFactor||[1,1,1,1];
      const mat=new T.MeshStandardMaterial({color:new T.Color().setRGB(c[0],c[1],c[2],T.LinearSRGBColorSpace),opacity:c[3],metalness:p.metallicFactor??1,roughness:p.roughnessFactor??1,side:m.doubleSided?T.DoubleSide:T.FrontSide});
      if(m.alphaMode==='BLEND'){mat.transparent=true;mat.depthWrite=false;}
      if(m.alphaMode==='MASK'){mat.alphaTest=m.alphaCutoff??0.5;}
      if(p.baseColorTexture){mat.map=textures[p.baseColorTexture.index];mat.map.colorSpace=T.SRGBColorSpace;}
      if(p.metallicRoughnessTexture){mat.metalnessMap=textures[p.metallicRoughnessTexture.index];mat.roughnessMap=mat.metalnessMap;}
      if(m.normalTexture){mat.normalMap=textures[m.normalTexture.index];mat.normalScale.set(m.normalTexture.scale??1,m.normalTexture.scale??1);}
      if(m.emissiveTexture){mat.emissiveMap=textures[m.emissiveTexture.index];mat.emissiveMap.colorSpace=T.SRGBColorSpace;}
      if(m.emissiveFactor)mat.emissive.setRGB(...m.emissiveFactor);
      return mat;
    });
    const nodes=(json.nodes||[]).map(n=>{
      const group=new T.Group();group.name=n.name||'';nodeTransform(group,n);
      if(n.mesh!==undefined){
        for(const primitive of json.meshes[n.mesh].primitives){
          if(primitive.mode!==undefined&&primitive.mode!==4)throw Error('Only GLB triangle primitives supported');
          const g=new T.BufferGeometry();
          const attrs={POSITION:'position',NORMAL:'normal',TEXCOORD_0:'uv',COLOR_0:'color',TANGENT:'tangent'};
          for(const [key,name] of Object.entries(attrs))if(primitive.attributes[key]!==undefined){
            const a=accessor(json,bin,primitive.attributes[key]);
            g.setAttribute(name,new T.BufferAttribute(a.array,a.itemSize,a.normalized));
          }
          if(primitive.indices!==undefined){const a=accessor(json,bin,primitive.indices);g.setIndex(new T.BufferAttribute(a.array,1));}
          if(!g.getAttribute('normal'))g.computeVertexNormals();
          const mesh=new T.Mesh(g,materials[primitive.material]||new T.MeshStandardMaterial());
          mesh.castShadow=true;mesh.receiveShadow=true;group.add(mesh);
        }
      }
      return group;
    });
    (json.nodes||[]).forEach((n,i)=>(n.children||[]).forEach(j=>nodes[i].add(nodes[j])));
    const scene=new T.Group(),selected=json.scenes?.[json.scene??0];
    for(const index of selected?.nodes||[])scene.add(nodes[index]);
    return {scene,animations:[]};
  }
}
