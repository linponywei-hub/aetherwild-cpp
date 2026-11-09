import {roadSegments} from './village-layout.js';
import * as T from '../../vendor/three.module.js';

let resources;
// Nine shared samplers, loaded once from this deployment. No third-party runtime requests.
export function starterTextures(loader=typeof document==='undefined'?null:new T.TextureLoader()){
 if(resources)return resources;
 resources=createStarterTextures(loader);return resources;
}
export function createStarterTextures(loader){
 const ready={value:0},maps={},names=['grassColor','grassNormal','grassArm','gravelColor','gravelNormal','gravelArm','mudColor','mudNormal','mudArm'];
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
 const segments=roadSegments(starter);if(segments.length>32)throw Error('Village path budget exceeded');shader.uniforms.awRoadCount={value:segments.length};shader.uniforms.awRoadSegments={value:Array.from({length:32},(_,i)=>new T.Vector4(...(segments[i]||[0,0,0,0])))};
 shader.uniforms.awPlaza={value:starter.plazaRadius};const creek=starter.stream;shader.uniforms.awCreek={value:new T.Vector4(creek?.x||1e8,creek?.meander||0,creek?.period||40,(creek?.halfWidth||0)+(creek?.bankWidth||0))};shader.uniforms.awCreekRange={value:new T.Vector2(creek?.zMin||0,creek?.zMax||0)};
 shader.fragmentShader=`uniform int awRoadCount;uniform vec4 awRoadSegments[32];
 uniform float awSurfaceReady;uniform float awPlaza;uniform vec4 awVillage;uniform vec4 awCreek;uniform vec2 awCreekRange;
 uniform sampler2D awGrassColor;uniform sampler2D awGrassNormal;uniform sampler2D awGrassArm;
 uniform sampler2D awGravelColor;uniform sampler2D awGravelNormal;uniform sampler2D awGravelArm;
 uniform sampler2D awMudColor;uniform sampler2D awMudNormal;uniform sampler2D awMudArm;
 `+shader.fragmentShader;
 shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',`#include <color_fragment>
  vec2 awLocal=awTerrainPosition.xz-awVillage.xy;
  float awVillageBlend=awSurfaceReady*(1.0-smoothstep(awVillage.z*.8,awVillage.z,length(awLocal)));
  float awRoadDistance=10000.0;
  for(int i=0;i<32;i++){if(i>=awRoadCount)break;vec4 s=awRoadSegments[i];vec2 d=s.zw-s.xy;float t=clamp(dot(awLocal-s.xy,d)/max(dot(d,d),.001),0.0,1.0);awRoadDistance=min(awRoadDistance,length(awLocal-s.xy-t*d));}
  float awEdge=.22*sin(awLocal.x*1.7+awLocal.y*.8)+.18*sin(awLocal.y*2.8);
  float awRoad=1.0-smoothstep(awVillage.w-.45,awVillage.w+1.1,awRoadDistance+awEdge);
  awRoad=max(awRoad,1.0-smoothstep(awPlaza-1.0,awPlaza+1.2,length(awLocal)+awEdge));
  float awBankDistance=abs(awLocal.x-awCreek.x-awCreek.y*sin(awLocal.y/awCreek.z));
  float awBank=(1.0-smoothstep(max(0.0,awCreek.w-2.0),awCreek.w+1.0,awBankDistance))*smoothstep(awCreekRange.x,awCreekRange.x+8.0,awLocal.y)*(1.0-smoothstep(awCreekRange.y-8.0,awCreekRange.y,awLocal.y));awRoad=max(awRoad,awBank);
  float awPatches=awNoise(vec3(awLocal*.045,17.0))*.7+awNoise(vec3(awLocal*.12,4.0))*.3;
  float awBare=smoothstep(.40,.65,awPatches);
  float awMud=max(awBare*.85,awRoad*.65);float awStone=max(awRoad*smoothstep(.35,.68,awPatches),smoothstep(.65,.8,awPatches)*.75);
  vec2 awWarp=vec2(awNoise(vec3(awLocal*.17,7.0)),awNoise(vec3(awLocal*.17,13.0)))*.65;
  vec2 awGrassUV=awTerrainPosition.xz/4.0+awWarp,awGravelUV=awTerrainPosition.xz/3.0+awWarp,awMudUV=awTerrainPosition.xz/3.5-awWarp;
  vec3 awGrass=texture2D(awGrassColor,awGrassUV).rgb;
  vec3 awGravel=texture2D(awGravelColor,awGravelUV).rgb;
  vec3 awDirt=texture2D(awMudColor,awMudUV).rgb;
  vec3 awGroundColor=mix(mix(awGrass,awDirt,awMud),awGravel,awStone);
  awGroundColor*=.82+.3*awNoise(vec3(awLocal*.025,1.0));
  diffuseColor.rgb=mix(diffuseColor.rgb,awGroundColor,awVillageBlend);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <roughnessmap_fragment>',`#include <roughnessmap_fragment>
  vec3 awGroundARM=mix(mix(texture2D(awGrassArm,awGrassUV).rgb,texture2D(awMudArm,awMudUV).rgb,awMud),texture2D(awGravelArm,awGravelUV).rgb,awStone);
  roughnessFactor=mix(roughnessFactor,clamp(awGroundARM.g,.45,1.0),awVillageBlend);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <normal_fragment_maps>',`#include <normal_fragment_maps>
  vec3 awGroundNormal=mix(mix(texture2D(awGrassNormal,awGrassUV).xyz,texture2D(awMudNormal,awMudUV).xyz,awMud),texture2D(awGravelNormal,awGravelUV).xyz,awStone)*2.0-1.0;
  normal=normalize(normal+mat3(viewMatrix)*vec3(awGroundNormal.x,0.0,awGroundNormal.y)*awVillageBlend*.45);`);
 shader.fragmentShader=shader.fragmentShader.replace('#include <aomap_fragment>',`#include <aomap_fragment>
  reflectedLight.indirectDiffuse*=mix(1.0,.65+.35*awGroundARM.r,awVillageBlend);`);
}
