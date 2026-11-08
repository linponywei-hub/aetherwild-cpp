// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderSkillsPanel({state, catalog, esc, row}) {
 const p=state.player;let html="";
html='<h3>五格技能配置</h3><p class="note">從已學會、目前職業可用的技能選擇五格。按 1–5 施放；F 使用第一格。每個技能只能裝備一次，空格不會自動施放其他技能。</p>';const options=catalog.skills.filter(s=>(p.skillOptions||p.skills).includes(s.id));for(let i=0;i<5;i++){const selected=p.skillSlots?.[i]||'';html+='<label class="skill-config-row">技能 '+(i+1)+'<select data-skill-choice="'+i+'"><option value="">空格</option>'+options.map(s=>'<option value="'+esc(s.id)+'" '+(selected===s.id?'selected':'')+'>'+esc(s.name)+' · '+s.mana+' MP</option>').join('')+'</select></label>';}html+='<h4>目前可用技能</h4>';for(const s of options)html+=row(s.id,s.name,`${s.mana}MP · 冷卻${s.cooldown}秒 · 傷害${s.damage} / 治療${s.heal}`);
 return html;
}
