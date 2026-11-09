// External, already skinned characters. Game identities and professions stay on the server.
const base = (id, file) => Object.freeze({id, sourceId:'Quaternius',
  path:'/assets/models/characters/villagers/'+file+'.glb', category:'humanoid',
  rigFamily:'QUATERNIUS_UNIVERSAL', upAxis:'+Y', forwardAxis:'+Z', version:1,
  status:'skinned-animated', scale:1, yaw:0, height:1.78, animationPath:'/assets/models/characters/villagers/animations.glb'});
export const BASE_MODELS = Object.freeze({
  WOMAN_MEDIEVAL:base('WOMAN_MEDIEVAL','Woman_Medieval'),
  WOMAN_TRAVELLER:base('WOMAN_TRAVELLER','Woman_Adventurer'),
  WOMAN_HERBALIST:base('WOMAN_HERBALIST','Woman_Merchant'),
  WOMAN_MERCHANT:base('WOMAN_MERCHANT','Woman_Merchant'),
  MAN_RESIDENT:base('MAN_RESIDENT','Man_Resident'),
  MAN_CRAFTSMAN:base('MAN_CRAFTSMAN','Man_Farmer'),
  MAN_TRAVELLER:base('MAN_TRAVELLER','Man_Adventurer')
});
export const MODEL_VARIANTS = Object.freeze({
  VILLAGE_EILEEN:{baseId:'WOMAN_MEDIEVAL',displayName:'艾琳 · 公會接待',tint:'#e0e9f5',height:1.72},
  VILLAGE_MILA:{baseId:'WOMAN_MERCHANT',displayName:'米菈 · 雜貨商',tint:'#f4d9b9',height:1.69},
  VILLAGE_LUNE:{baseId:'WOMAN_HERBALIST',displayName:'露恩 · 草藥療癒師',tint:'#d3e4d4',height:1.71},
  VILLAGE_SMITH:{baseId:'MAN_CRAFTSMAN',beard:true,displayName:'鐵叔 · 工匠',tint:'#bfb1a5',height:1.85},
  VILLAGE_ORI:{baseId:'MAN_TRAVELLER',displayName:'奧里 · 製圖師',tint:'#e0d2b8',height:1.8},
  VILLAGE_GARDENER:{baseId:'MAN_CRAFTSMAN',displayName:'園丁 · 農務工作服',tint:'#d0dfb7',height:1.76},
  VILLAGE_TRAVELLER_F:{baseId:'WOMAN_TRAVELLER',displayName:'旅人 · 輕裝旅行服',tint:'#ece6d6',height:1.7},
  VILLAGE_TRAVELLER_M:{baseId:'MAN_RESIDENT',displayName:'居民 · 旅行工作服',tint:'#c7d8d5',height:1.77},
  VILLAGE_VILLAGER_F:{baseId:'WOMAN_MEDIEVAL',displayName:'居民 · 中世紀村服',tint:'#d6c5b5',height:1.68}
});
// Existing player and other-town procedural models remain available without importing a village skin.
export const ROLE_VARIANTS = Object.freeze({});
export function resolveVariant(id, bases = BASE_MODELS, variants = MODEL_VARIANTS) {
 const variant=variants[id];if(!variant)throw Error('Unknown variant: '+id);
 const model=bases[variant.baseId];if(!model)throw Error('Missing base model: '+variant.baseId);
 return {...model,...variant,baseId:model.id,variantId:id,path:model.path};
}
