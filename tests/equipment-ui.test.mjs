import {test} from 'node:test';
import assert from 'node:assert/strict';
import {statLine,enhancementText} from '../web/equipment-ui.js';
test('equipment comparisons preserve negative changes and server precision',()=>{assert.equal(statLine({attack:14.456,defense:-3},true),'攻擊 +14.46 · 防禦 -3');assert.equal(statLine({},true),'無變化');});
test('enhancement warning displays authoritative chance and price',()=>{assert.equal(enhancementText({coins:220,ore:2,chance:1}),'220 金幣 + 鐵礦 ×2 · 成功率 100.0%');assert.match(enhancementText({coins:1000,ore:2,chance:.75}),/75.0%/);});
