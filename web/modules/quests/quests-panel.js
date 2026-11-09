// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderQuestsPanel({state, catalog, row, button}) {
 const p=state.player;let html="";
html='<h3>冒險手冊</h3><p class="note">任務依實際採集、擊殺與副本通關累計，完成後回原委託人領取。</p>';let list=catalog.quests.filter(q=>p.quests[q.id]);for(let q of list){let progress=p.quests[q.id];html+=row('',q.name,`Lv.${q.level} · ${progress?progress.count:0}/${q.count} · ${q.coins}金幣 / ${q.xp}經驗`,progress?.claimed?'<span class="badge">已完成</span>':progress?button('回報任務','quest',{target:q.id,npc:q.npc},progress.count<q.count):button('接取','accept',{target:q.id,npc:q.npc}),false);}
 return html;
}
