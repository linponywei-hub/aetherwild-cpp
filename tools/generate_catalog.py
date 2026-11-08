"""Offline data compiler. Only Python stdlib; no legacy JS code runs during build."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
d=json.loads((root/'data/legacy_catalog.json').read_text(encoding='utf-8'))
q=lambda s:json.dumps(str(s or ''),ensure_ascii=False)
num=lambda x:str(float(x or 0))
def stats(x):return 'Stats{'+','.join(num(x.get(k,0)) for k in ['hp','mp','attack','defense','crit','speed'])+'}'
def cost(x):return 'Materials{'+','.join('{'+q(k)+','+str(int(v))+'}' for k,v in x.items())+'}'
def pos(x):return 'Vec3{'+','.join(num(x.get(k,0)) for k in ['x','y','z'])+'}'
def res(f,t):return ('wood' if t==1 else 'hardwood' if t==2 else 'wood'+str(t)) if f=='wood' else ('ore' if t==1 else 'silverore' if t==2 else 'ore'+str(t)) if f=='ore' else f if t==1 else f+str(t)
slots={'weapon':'Weapon','armor':'Armor','helm':'Helm','boots':'Boots','charm':'Charm'}
magic={'mage','battlemage','archmage','sage','priest','witch','necromancer'}
bow={'ranger','hawkeye','windlord'}
weapon=lambda id:'Staff' if id in magic else 'Bow' if id in bow else 'Sword'
lines=['// Generated from data/legacy_catalog.json. Do not edit by hand.','#include "aetherwild/domain.hpp"','namespace aw { Catalog makeCatalog(){ Catalog c;']
for k,v in d['ITEMS'].items():
    lines.append(f'c.items.emplace({q(k)},ItemDefinition{{{q(k)},{q(v["name"])},Slot::{slots[v["slot"]]},WeaponKind::{str(v.get("type","none")).title()},{stats(v)},{v.get("requiredLevel",1)},{q(v.get("set"))}}});')
for k,v in d['CLASS'].items():
    lines.append(f'c.classes.emplace({q(k)},ClassDefinition{{{q(k)},{q(v["name"])},{stats(v)},{num(v.get("range",5))},{v.get("level",1)},{q(v.get("parent"))},{q(v.get("secret"))},{cost(d["JOB_COSTS"].get(k,{}))},WeaponKind::{weapon(k)}}});')
for v in d['REGIONS']:
    lines.append(f'c.regions.emplace({q(v["id"])},RegionDefinition{{{q(v["id"])},{q(v["name"])},{pos(v)},{num(v["radius"])},{v["level"]},{str(v.get("city",False)).lower()},{str(v.get("wilderness",False)).lower()}}});')
for k,v in d['RESOURCES'].items():
    lines.append(f'c.resources.emplace({q(k)},ResourceDefinition{{{q(k)},{q(d["MATERIALS"].get(k,k))},{q(v.get("family",k))},{q(v.get("tool"))},{v["tier"]},{v["need"]},{v["xp"]},{1.5+v["tier"]*2+v["need"]/40}}});')
for k,v in d['TOOLS'].items():
    lines.append(f'c.tools.emplace({q(k)},ToolDefinition{{{q(k)},{q(v["name"])},{q(v["kind"])},{v["tier"]},{cost(v["cost"])}}});')
    lines.append(f'c.recipes.emplace({q(k)},Recipe{{{q(k)},{q(k)},{v["tier"]},{cost(v["cost"])},{q("portable" if v["tier"]==1 else "smith")},false,1}});')
for k,v in d['GEAR_RECIPES'].items():lines.append(f'c.recipes.emplace({q(k)},Recipe{{{q(k)},{q(v["code"])},{v["tier"]},{cost(v["cost"])},"smith",true,1}});')
for k,v in d['ARROWS'].items():
    lines.append(f'c.recipes.emplace({q(k)},Recipe{{{q(k)},{q(k)},1,{cost(v["cost"])},"portable",false,{v["count"]}}});')
    lines.append(f'c.arrowBonus.emplace({q(k)},{num(v["bonus"])});')
for k,v in {'potion':{'herb':2},'energy_potion':{'herb':2,'twig':1}}.items():lines.append(f'c.recipes.emplace({q(k)},Recipe{{{q(k)},{q(k)},1,{cost(v)},"portable",false,1}});')
for k,v in d['SKILLS'].items():lines.append(f'c.skills.emplace({q(k)},SkillDefinition{{{q(k)},{q(v["name"])},{q(v.get("effect"))},'+','.join(num(v.get(x,0)) for x in ['mana','cd','damage','heal','range','radius','duration','shield'])+'});')
for v in d['DUNGEONS']:
    secret={'shadow':'shadow','stars':'sage'}.get(v['id'],v.get('legacy',''))
    lines.append(f'c.dungeons.emplace({q(v["id"])},DungeonDefinition{{{q(v["id"])},{q(v["region"])},{q(v["name"])},{q(v["objective"])},{q(v["entrance"])},{v["level"]},{v["guards"]},{q(v.get("skill"))},{q(secret)},{pos(v)}}});')
for v in d['QUESTS']:
    target=v.get('dungeon',v.get('boss',v.get('region','')))
    lines.append(f'c.quests.emplace({q(v["id"])},QuestDefinition{{{q(v["id"])},{q(v["name"])},{q(v["type"])},{q(target)},{q(v["npc"])},{v["level"]},{v["count"]},{v["coins"]},{v["xp"]}}});')
for v in d['BOSSES']:lines.append(f'c.bosses.emplace({q(v["id"])},BossDefinition{{{q(v["id"])},{q(v["region"])},{q(v["name"])},{pos(v)},{v["level"]},{num(v.get("hp"))},{num(v.get("attack"))}}});')
for k,v in d['BOSS_SETS'].items():lines.append(f'c.sets.emplace({q(k)},SetDefinition{{{q(k)},{stats(v["two"])},{stats(v["four"])}}});')
# NPC service anchors: deliberately flat reference positions, NOT converted 3D city floors.
for r in d['REGIONS']:
    for i,role in enumerate(['guild','merchant','smith','healer','cartographer']):
        if i>=len(r.get('npcNames',[])):continue
        k=r['id']+'_'+role
        lines.append(f'c.npcs.emplace({q(k)},NpcDefinition{{{q(k)},{q(r["id"])},{q(r["npcNames"][i])},{q(role)},{pos(r)}}});')
for k,v in d['SHOP_CATALOGUE'].items():
    if not v.get('craftOnly') and v.get('price',0)>0 and (k in d['RESOURCES'] or k in d['ITEMS'] or k in ['potion','energy_potion']):lines.append(f'c.prices.emplace({q(k)},{v["price"]});')
lines+=['c.validate(); return c; } }']
groups={}
for line in lines[3:-1]:
    group=line.split('c.',1)[1].split('.',1)[0]
    groups.setdefault(group,[]).append(line)
output=['// Generated from data/legacy_catalog.json. Do not edit by hand.','#include "aetherwild/domain.hpp"','namespace aw { namespace {']
for group,entries in groups.items():
    output.append('void load_'+group+'(Catalog& c) {')
    output.extend('    '+line for line in entries)
    output.append('}')
output+=['}', 'Catalog makeCatalog() { Catalog c;']
output.extend('    load_'+group+'(c);' for group in groups)
output+=['    c.validate(); return c;', '}', '}']
(root/'src/generated/catalog.cpp').write_text('\n'.join(output)+'\n',encoding='utf-8')
print('Catalog generated:',len(d['ITEMS']),'items,',len(d['DUNGEONS']),'dungeons')
