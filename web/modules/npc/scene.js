import {RemoteTrack} from '/modules/network/remote-motion.js';
export const VILLAGE_CAST=Object.freeze({
 windchime_guild:'VILLAGE_EILEEN',windchime_merchant:'VILLAGE_MILA',
 windchime_healer:'VILLAGE_LUNE',windchime_smith:'VILLAGE_SMITH',windchime_cartographer:'VILLAGE_ORI',
 aw_npc_guild:'VILLAGE_TRAVELLER_M',aw_exp_npc_windchime_local:'VILLAGE_GARDENER',
 aw_exp_npc_windchime_witness:'VILLAGE_TRAVELLER_M',
 windchime_resident_0:'VILLAGE_VILLAGER_F',windchime_resident_1:'VILLAGE_TRAVELLER_F',
 windchime_resident_2:'VILLAGE_GARDENER',windchime_resident_3:'VILLAGE_TRAVELLER_M',
 windchime_resident_4:'VILLAGE_TRAVELLER_F',windchime_resident_5:'VILLAGE_VILLAGER_F',
 windchime_resident_6:'VILLAGE_TRAVELLER_M',windchime_resident_7:'VILLAGE_VILLAGER_F',
 windchime_resident_8:'VILLAGE_TRAVELLER_F',windchime_resident_9:'VILLAGE_GARDENER'
});
export function makeNpcs(view){
 for(const n of view.catalog.npcs){
  const asset=n.region==='windchime'?(VILLAGE_CAST[n.id]||({guild:'VILLAGE_EILEEN',merchant:'VILLAGE_MILA',smith:'VILLAGE_SMITH',healer:'VILLAGE_LUNE',cartographer:'VILLAGE_ORI'})[n.role]||'VILLAGE_TRAVELLER_M'):null;
  const entity=view.human(n.role,n.id,asset);entity.position.set(n.position.x,n.position.y,n.position.z);
  entity.userData={...entity.userData,kind:'npc',data:n,externalVillager:!!asset};
  view.nameplates.register('npc:'+n.id,entity,n.name+' · '+(({guild:'公會',merchant:'商人',smith:'鍛造',healer:'療癒',cartographer:'製圖'})[n.role]||'居民'),true);
  view.root.add(entity);if(asset&&!view.renderer.software&&view.modelAssets.registry[asset])void view.modelAssets.attach(entity,asset);view.npcObjects.set(n.id,entity);entity.userData.track=new RemoteTrack();
 }
}
