// Visual asset identities are independent of game profession and server IDs.
// One base model can power many lightweight variants without duplicating GLB data.
export const BASE_MODELS = Object.freeze({
  HUMANOID_BASE_001: Object.freeze({
    id: 'HUMANOID_BASE_001',
    sourceId: 'TRIPO-001',
    path: '/assets/models/characters/TRIPO-001.glb',
    displayName: 'TRIPO-001 人形母模型',
    category: 'humanoid',
    rigFamily: 'HUM_BIPED',
    upAxis: '+Y',
    forwardAxis: '+Z',
    version: 1,
    status: 'static-unrigged',
    scale: 1,
    yaw: 0
  })
});
export const MODEL_VARIANTS = Object.freeze({
  HUM_WARRIOR_001: { baseId: 'HUMANOID_BASE_001', displayName: '戰士', tint: '#ffffff', scale: 1 },
  HUM_MAGE_001: { baseId: 'HUMANOID_BASE_001', displayName: '法師', tint: '#cbd9ff', scale: 1 },
  HUM_RANGER_001: { baseId: 'HUMANOID_BASE_001', displayName: '遊俠', tint: '#d1e9cb', scale: 1 },
  HUM_NPC_001: { baseId: 'HUMANOID_BASE_001', displayName: '村民', tint: '#f1e3d2', scale: 1 },
  HUM_GUARD_001: { baseId: 'HUMANOID_BASE_001', displayName: '守衛', tint: '#d9e2e8', scale: 1 },
  HUM_MERCHANT_001: { baseId: 'HUMANOID_BASE_001', displayName: '商人', tint: '#f4ddb4', scale: 1 }
});
export const ROLE_VARIANTS = Object.freeze({
  warrior: 'HUM_WARRIOR_001', mage: 'HUM_MAGE_001',
  ranger: 'HUM_RANGER_001', guild: 'HUM_NPC_001',
  merchant: 'HUM_MERCHANT_001', smith: 'HUM_NPC_001',
  healer: 'HUM_NPC_001', cartographer: 'HUM_NPC_001',
  guard: 'HUM_GUARD_001'
});
export function resolveVariant(id, bases = BASE_MODELS, variants = MODEL_VARIANTS) {
  const variant = variants[id];
  if (!variant) throw Error('Unknown variant: '+id);
  const base = bases[variant.baseId];
  if (!base) throw Error('Missing base model: '+variant.baseId);
  return { ...base, ...variant, baseId: base.id, variantId: id, path: base.path };
}
