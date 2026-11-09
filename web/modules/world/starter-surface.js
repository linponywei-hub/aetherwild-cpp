import * as T from '../../vendor/three.module.js';

let resources;
// Six shared samplers, loaded once from this deployment. No third-party runtime requests.
export function starterTextures(loader=typeof document==='undefined'?null:new T.TextureLoader()){
 if(resources)return resources;
 resources=createStarterTextures(loader);return resources;
}
export function createStarterTextures(loader){
 const ready={value:0},maps={},names=['grassColor','grassNormal','grassArm','gravelColor','gravelNormal','gravelArm'];
 let loaded=0;
 for(const name of names){
  const file=name.replace(/[A-Z]/g,c=>'-'+c.toLowerCase());
  maps[name]=new T.Texture();
  if(!loader)continue;
  const texture=loader.load('/art/terrain/'+file+'.webp',()=>{loaded++;if(loaded===names.length)ready.value=1;},undefined,()=>{/* Keep procedural fallback for the whole set. */});
  texture.wrapS=texture.wrapT=T.RepeatWrapping;texture.colorSpace=name.endsWith('Color')?T.SRGBColorSpace:T.NoColorSpace;
  texture.anisotropy=4;maps[name]=texture;
 }
 return {ready,maps};
}
export function installStarterSurface(shader,starter){
 if(!starter)return;
 const {maps,ready}=starterTextures();shader.uniforms.awSurfaceReady=ready;
 for(const [name,value] of Object.entries(maps))shader.uniforms['aw'+name[0].toUpperCase()+name.slice(1)]={value};
 shader.uniforms.awVillage={value:new T.Vector4((starter.pixel[0]-633)*64,(starter.pixel[1]-578)*64,starter.detailRadius,starter.roadHalfWidth)};
 shader.uniforms.awPlaza={value:starter.plazaRadius};const creek=starter.stream;shader.uniforms.awCreek={value:new T.Vector4(creek?.x||1e8,creek?.meander||0,creek?.period||40,(creek?.halfWidth||0)+(creek?.bankWidth||0))};shader.uniforms.awCreekRange={value:new T.Vector2(creek?.zMin||0,creek?.zMax||0)};
 shader.fragmentShader=`uniform float awSurfaceReady;uniform float awPlaza;uniform vec4 awVillage;uniform vec4 awCreek;uniform vec2 awCreekRange;
 uniform sampler2D awGrassColor;uniform sampler2D awGrassNormal;uniform sampler2D awGrassArm;
 uniform sampler2D awGravelColor;uniform sampler2D awGravelNormal;uniform sampler2D awGravelArm;
 `+shader.fragmentShader;
 shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',`#include <color_fragment>
  vec2 awLocal=awTerrainPosition.xz-awVillage.xy;
  float awVillageBlend=awSurfaceReady*(1.0-smoothstep(awVillage.z*.8,awVillage.z,length(awLocal)));
  float awRoadDistance=min(abs(awLocal.x+.3*sin(awLocal.y*.075)),abs(awLocal.y+.3*sin(awLocal.x*.075)));
  float awEdge=.22*sin(awLocal.x*1.7+awLocal.y*.8)+.18*sin(awLocal.y*2.8);
  float awRoad=1.0-smoothstep(awVillage.w-.45,awVillage.w+1.1,awRoadDistance+awEdge);
  awRoad=max(awRoad,1.0-smoothstep(awPlaza-1.0,awPlaza+1.2,length(awLocal)+awEdge));
  float awBankDistance=abs(awLocal.x-awCreek.x-awCreek.y*sin(awLocal.y/awCreek.z));
  float awBank=(1.0-smoothstep(max(0.0,awCreek.w-2.0),awCreek.w+1.0,awBankDistance))*smoothstep(awCreekRange.x,awCreekRange.x+8.0,awLocal.y)*(1.0-smoothstep(awCreekRange.y-8.0,awCreekRange.y,awLocal.y));awRoad=max(awRoad,awBank);
  vec2 awGrassUV=awTerrainPosition.xz/4.0,awGravelUV=awTerrainPosition.xz/3.0;
  vec3 awGrass=texture2D(awGrassColor,awGrassUV).rgb;
  vec3 awGravel=texture2D(awGravelColor,awGravelUV).rgb;
  vec3 awGroundColor=mix(awGrass,awGravel,awRoad);
  diffuseColor.rgb=mix(diffuseColor.rgb,awGroundColor,awVillageBlend);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <roughnessmap_fragment>',`#include <roughnessmap_fragment>
  vec3 awGroundARM=mix(texture2D(awGrassArm,awGrassUV).rgb,texture2D(awGravelArm,awGravelUV).rgb,awRoad);
  roughnessFactor=mix(roughnessFactor,clamp(awGroundARM.g,.45,1.0),awVillageBlend);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <normal_fragment_maps>',`#include <normal_fragment_maps>
  vec3 awGroundNormal=mix(texture2D(awGrassNormal,awGrassUV).xyz,texture2D(awGravelNormal,awGravelUV).xyz,awRoad)*2.0-1.0;
  normal=normalize(normal+mat3(viewMatrix)*vec3(awGroundNormal.x,0.0,awGroundNormal.y)*awVillageBlend*.45);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <aomap_fragment>',`#include <aomap_fragment>
  reflectedLight.indirectDiffuse*=mix(1.0,.65+.35*awGroundARM.r,awVillageBlend);`);
}
