import {humanoid} from '/modules/actors/actor-scene.js';
// Rendering adapters; authoritative simulation remains in C++.
export const actorInstanceMethods = {
 human(role='warrior',id='',assetOverride=null){const g=humanoid(role,id||role);g.userData.proceduralParts=[...g.children];const assetId=({warrior:'HUM_WARRIOR_001',mage:'HUM_MAGE_001',ranger:'HUM_RANGER_001',merchant:'HUM_MERCHANT_001',guard:'HUM_GUARD_001'})[role]||'HUM_NPC_001';g.userData.modelAssetId=assetOverride||assetId;if(this.renderer.software)this.contactShadow(g,.45,.35);else if(this.modelAssets.registry[g.userData.modelAssetId])queueMicrotask(()=>this.modelAssets.attach(g,g.userData.modelAssetId));return g;}
};
