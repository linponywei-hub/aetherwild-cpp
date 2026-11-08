// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderWorldPanel({state, catalog, esc}) {
 const p=state.player;let html="";
html='<h3>全域城鎮與原野</h3><p class="note">與城鎮公會交談後可搭乘驛車（25金幣）。地圖需向製圖師購買，才能在右側顯示。</p><div class="button-grid">';for(let r of catalog.regions.filter(r=>!r.wilderness))html+=`<button data-command="travel" data-target="${esc(r.id)}"><strong>${esc(r.name)}</strong><small>建議 Lv.${r.level} · ${p.maps.includes(r.id)?'地圖已解鎖':'尚未購圖'}</small></button>`;html+='</div>';
 return html;
}
