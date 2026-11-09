// Verified filenames from the uploaded Quaternius Stylized Nature MegaKit Standard archive.
// Bundle converted GLBs at the paths below before enabling external tree rendering.
export const TREE_SPECIES=Object.freeze([
 ['oak_1','CommonTree_1',['plain','forest'],-100,110,16],
 ['oak_2','CommonTree_2',['plain','forest'],-100,130,13],
 ['oak_3','CommonTree_3',['plain','forest'],0,180,15],
 ['oak_4','CommonTree_4',['forest','plain'],20,210,14],
 ['oak_5','CommonTree_5',['forest','plain'],35,240,12],
 ['pine_1','Pine_1',['forest','plain','snow'],75,380,21],
 ['pine_2','Pine_2',['forest','snow'],100,440,18],
 ['pine_3','Pine_3',['forest','snow'],130,500,20],
 ['pine_4','Pine_4',['forest','snow'],155,550,17],
 ['pine_5','Pine_5',['forest','snow'],170,620,15],
 ['twisted_1','TwistedTree_1',['marsh','forest'],-100,90,12],
 ['twisted_2','TwistedTree_2',['marsh','forest'],-100,120,13],
 ['twisted_3','TwistedTree_3',['marsh','forest'],0,145,11],
 ['twisted_4','TwistedTree_4',['marsh','forest'],0,165,14],
 ['twisted_5','TwistedTree_5',['marsh','forest'],20,190,12],
 ['dead_1','DeadTree_1',['desert','volcanic','mesa'],-100,250,8],
 ['dead_2','DeadTree_2',['desert','volcanic','mesa'],-100,280,9],
 ['dead_3','DeadTree_3',['desert','volcanic','mesa'],-100,300,7],
 ['dead_4','DeadTree_4',['desert','volcanic','mesa'],-100,330,10],
 ['dead_5','DeadTree_5',['desert','volcanic','mesa'],-100,350,8]
].map(([id,file,biomes,min,max,meters])=>Object.freeze({id,path:'/assets/models/environment/trees/'+file+'.glb',biomes,min,max,meters})));
export function speciesFor(biome,elevation,random=0){const choices=TREE_SPECIES.filter(s=>s.biomes.includes(biome)&&elevation>=s.min&&elevation<=s.max);return choices.length?choices[Math.min(choices.length-1,Math.floor(random*choices.length))]:null;}
