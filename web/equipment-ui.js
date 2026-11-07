const labels={hp:'生命',mp:'魔力',attack:'攻擊',defense:'防禦',crit:'暴擊',speed:'速度'};
// Only formats authoritative server values; no cost or stat rules in the browser.
export function statLine(stats={},delta=false){return Object.entries(labels).filter(([k])=>Math.abs(stats[k]||0)>.0001).map(([k,label])=>{const n=stats[k];return label+' '+(n>0?'+':'')+(Math.round(n*100)/100);}).join(' · ')||(delta?'無變化':'無屬性');}
export function enhancementText(q){return q.coins.toLocaleString()+' 金幣 + 鐵礦 ×'+q.ore+' · 成功率 '+(q.chance*100).toFixed(1)+'%';}
