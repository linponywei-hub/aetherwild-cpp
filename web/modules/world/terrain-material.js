import * as T from '../../vendor/three.module.js';
import {installStarterSurface} from './starter-surface.js';

// World-space detail avoids stretched UVs on cliffs. Both editor and game use
// this material; geometry and collision heights still come from native tiles.
export function terrainMaterial(options={}) {
 const {starter,...surfaceOptions}=options;
 const material=new T.MeshStandardMaterial({roughness:.94,...surfaceOptions});
 material.extensions={...(material.extensions||{}),derivatives:true};
 material.onBeforeCompile=shader=>{
  shader.vertexShader='attribute float awClimate; attribute float awBiome; varying float awBiomeType; varying float awSnow; varying float awBiomeType; varying vec3 awTerrainPosition; varying vec3 awTerrainNormal;\n'+shader.vertexShader;
  shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>',`#include <begin_vertex>
   awBiomeType=awBiome;awTerrainPosition=(modelMatrix*vec4(position,1.0)).xyz;
   awTerrainNormal=normalize(mat3(modelMatrix)*normal);awSnow=awClimate;`);
  shader.fragmentShader=`varying float awSnow; varying vec3 awTerrainPosition; varying vec3 awTerrainNormal;
   float awHash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
   float awNoise(vec3 p){vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(mix(awHash(i),awHash(i+vec3(1,0,0)),f.x),mix(awHash(i+vec3(0,1,0)),awHash(i+vec3(1,1,0)),f.x),f.y),
     mix(mix(awHash(i+vec3(0,0,1)),awHash(i+vec3(1,0,1)),f.x),mix(awHash(i+vec3(0,1,1)),awHash(i+vec3(1,1,1)),f.x),f.y),f.z);}
   `+shader.fragmentShader;
  shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',`#include <color_fragment>
   float awSlope=1.0-abs(normalize(awTerrainNormal).y);
   float awFine=awNoise(awTerrainPosition*2.2),awPatch=awNoise(awTerrainPosition*.12);
   vec3 awRock=mix(vec3(.21,.22,.20),diffuseColor.rgb*.72,.45);
   diffuseColor.rgb=mix(diffuseColor.rgb,awRock,smoothstep(.26,.65,awSlope)*.85);
   diffuseColor.rgb*=.84+.24*awPatch+.12*awFine;
   // Region-specific microdetail: dark forest litter, dry sandstone and
   // volcanic scree remain distinct even when their base vertex colors blend.
   float awDry=clamp((diffuseColor.r-diffuseColor.b)*2.5,0.0,1.0);
   float awForest=clamp((diffuseColor.g-diffuseColor.r)*3.0,0.0,1.0);
   float awGrain=awNoise(vec3(awTerrainPosition.xz*.9,12.0));
   float awStrata=awNoise(vec3(awTerrainPosition.xz*.08,awTerrainPosition.y*.12));
   diffuseColor.rgb*=1.0+awDry*(awGrain-.5)*.24+awForest*(awStrata-.5)*.15;
   // Explicit map biome drives distinct surface patterning, not a shared
   // color-tinted ground. Triplanar cliff detail is evaluated in world space.
   float awSand=1.0-smoothstep(.35,.8,abs(awBiomeType-2.0));
   float awMesa=1.0-smoothstep(.35,.8,abs(awBiomeType-3.0));
   float awWet=1.0-smoothstep(.35,.8,abs(awBiomeType-4.0));
   float awLava=1.0-smoothstep(.35,.8,abs(awBiomeType-6.0));
   float awForestFloor=1.0-smoothstep(.35,.8,abs(awBiomeType-1.0));
   float awRipple=sin(awTerrainPosition.x*.32+awTerrainPosition.z*.17+awPatch*2.5);
   vec3 awDuneColor=mix(vec3(.59,.39,.18),vec3(.88,.70,.39),clamp(.5+.5*awRipple,0.0,1.0));
   diffuseColor.rgb=mix(diffuseColor.rgb,awDuneColor,awSand*.68*(1.0-smoothstep(.20,.55,awSlope)));
   float awLayers=sin(awTerrainPosition.y*.15+awStrata*3.0);
   diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.57,.28,.14)*(0.85+.25*awLayers),awMesa*.68);
   diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.16,.24,.14)+vec3(.09,.10,.04)*awFine,awWet*.65);
   diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.12,.12,.13)+vec3(.16,.07,.03)*awPatch,awLava*.82);
   diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.12,.20,.10)+vec3(.09,.07,.02)*awFine,awForestFloor*.50);
   // Snow accumulation follows slope; rock faces stay exposed.
   float awIce=awSnow*(1.0-smoothstep(.35,.7,awSlope));
   diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.88,.91,.90),awIce*.65);`);
  shader.fragmentShader=shader.fragmentShader.replace('#include <normal_fragment_maps>',`#include <normal_fragment_maps>
   normal=normalize(normal+vec3(dFdx(awFine),dFdy(awFine),0.0)*.16);`);
  installStarterSurface(shader,starter);
 };
 material.customProgramCacheKey=()=> 'atlas-terrain-material-v7' + (starter?'-starter':'');
 return material;
}

export function clipTerrainPatch(material,patch){
 if(!material.userData.patchBounds){
  material.userData.patchBounds=new T.Vector4();const compile=material.onBeforeCompile,cacheKey=material.customProgramCacheKey();
  material.onBeforeCompile=shader=>{
   compile.call(material,shader);shader.uniforms.patchBounds={value:material.userData.patchBounds};
   shader.vertexShader='varying vec2 patchWorld;\n'+shader.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\npatchWorld=(modelMatrix*vec4(position,1.0)).xz;');
   shader.fragmentShader='uniform vec4 patchBounds; varying vec2 patchWorld;\n'+shader.fragmentShader.replace('#include <clipping_planes_fragment>','#include <clipping_planes_fragment>\nif(patchWorld.x>patchBounds.x && patchWorld.y>patchBounds.y && patchWorld.x<patchBounds.z && patchWorld.y<patchBounds.w) discard;');
  };
  material.customProgramCacheKey=()=>cacheKey+'-patch-clip';material.needsUpdate=true;
 }
 material.userData.patchBounds.set(patch.x+1,patch.z+1,patch.x+(patch.side-1)*patch.step-1,patch.z+(patch.side-1)*patch.step-1);
}
