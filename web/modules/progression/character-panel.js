// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderCharacterPanel({state, classIndex, esc, equipmentShowcase}) {
 const p=state.player;let html="";
let g=state.bonus;html=`<div class="section-title"><div><h3>${esc(p.name)}</h3><span class="badge">${esc(classIndex[p.profession]?.name||'隱藏職業')}</span><span class="badge">LV. ${p.level}</span></div><span class="count">${esc(p.id)}</span></div><div class="stat-grid">${[['生命',Math.ceil(p.hp)+' / '+p.maxHp],['魔力',Math.ceil(p.mp)+' / '+p.maxMp],['攻擊',Math.round(state.stats.attack+(p.level-1)*3+g.attack)],['防禦',Math.round(g.defense)],['金幣',p.coins],['採集熟練度',p.gatherXp]].map(([a,b])=>`<div><small>${a}</small><b>${b}</b></div>`).join('')}</div><div class="equipment-heading"><h4>目前裝備</h4><small>五個裝備槽</small></div>`+equipmentShowcase(p)+'<p class="note">可向城鎮公會詢問本職訓練。</p>';
 return html;
}
