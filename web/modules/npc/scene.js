import {RemoteTrack} from '/modules/network/remote-motion.js';
export function makeNpcs(view){
 for(const n of view.catalog.npcs){const eileenTest=n.region==='windchime'&&n.name==='艾琳';const entity=view.human(n.role,n.id,eileenTest?'HUM_EILEEN_TEST_001':null);if(eileenTest)entity.userData.eileenTest=true;entity.position.set(n.position.x,n.position.y,n.position.z);entity.userData={...entity.userData,kind:'npc',data:n};view.nameplates.register('npc:'+n.id,entity,n.name+' · '+(({guild:'公會',merchant:'商人',smith:'鍛造',healer:'療癒',cartographer:'製圖'})[n.role]||'居民'),true);view.root.add(entity);view.npcObjects.set(n.id,entity);entity.userData.track=new RemoteTrack({changesOnly:true});}
}
