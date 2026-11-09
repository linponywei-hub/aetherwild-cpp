// Quaternius CC0 GLB candidates. Files must be bundled under /assets/models/environment/trees/.
export const TREE_SPECIES=Object.freeze([
 ['oak','CommonTree_1', ['plain','forest'],-100,110,16],
 ['broadleaf','CommonTree_3',['plain','forest'],-100,130,13],
 ['birch','BirchTree_1',['forest','plain'],15,200,17],
 ['silver_birch','BirchTree_4',['forest','plain'],35,250,14],
 ['willow','Willow_1',['marsh','plain'],-100,65,11],
 ['marsh_willow','Willow_3',['marsh'],-100,55,9],
 ['pine','PineTree_1',['forest','plain','snow'],75,380,21],
 ['highland_pine','PineTree_3',['forest','snow'],120,480,17],
 ['snow_pine','PineTree_Snow_1',['snow'],160,620,13],
 ['snow_birch','BirchTree_Snow_1',['snow'],110,370,10],
 ['palm','PalmTree_1',['desert','plain'],-100,45,12],
 ['coastal_palm','PalmTree_3',['desert','plain'],-100,30,9],
 ['dead_tree','DeadTree_1',['volcanic','mesa','desert'],-100,350,8],
 ['dry_birch','BirchTree_Dead_1',['mesa','volcanic'],-100,320,7]
].map(([id,file,biomes,min,max,meters])=>Object.freeze({id,path:'/assets/models/environment/trees/'+file+'.glb',biomes,min,max,meters})));
export function speciesFor(biome,elevation,random=0){const choices=TREE_SPECIES.filter(s=>s.biomes.includes(biome)&&elevation>=s.min&&elevation<=s.max);return choices.length?choices[Math.min(choices.length-1,Math.floor(random*choices.length))]:null;}
