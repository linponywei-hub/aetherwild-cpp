// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderCraftingPanel({state, catalog, forgePanel, filter, cost, row, button}) {
 const p=state.player;let html="";
html=forgePanel()+'<h3>製作與鍛造</h3><p class="note">基礎工具、藥水與箭矢可隨身製作。裝備需到鍛造師；二階以上劍在燼鐵、法杖在星穹、弓在翠冠。防具依職業分流。</p><div class="inventory-filter"><select id="recipe-filter"><option value="">全部配方</option><option value="portable">隨身製作</option><option value="smith">鍛造鋪</option></select></div>';for(let r of catalog.recipes.filter(r=>!filter||r.station===filter)){let enough=Object.entries(r.cost).every(([id,n])=>(p.materials[id]||0)>=n);html+=row(r.output,r.name+(r.count>1?' ×'+r.count:''),'T'+r.tier+' · '+cost(r.cost),button('製作','craft',{target:r.id},!enough));}
 return html;
}
