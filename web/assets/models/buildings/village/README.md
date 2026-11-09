# Village building models

Place the five optimized Tripo house models in this folder:

- `village_building_01_lod0.glb`
- `village_building_02_lod0.glb`
- `village_building_03_lod0.glb`
- `village_building_04_lod0.glb`
- `village_building_05_lod0.glb`

The scene loads these paths at runtime. Keep original material and texture data embedded in each GLB.

自然村落修補：每棟原始 GLB 旁的 `.repair.json` 是已接入遊戲及世界編輯器的幾何修補，不是候選檔。載入時核對原始 SHA-256，限制邊界頂點位移、縫合窄裂、補平面小孔、刪除退化面，並對齊三角面方向。原始 UV／貼圖及姿態資料不覆寫；原檔缺少頂點法線時建立平滑法線。`repair-report.json` 記錄修補前後邊界數量；保留大型門窗／複雜開口，不能把所有開放邊界都當作裂縫封死。工具：`tools/repair_village_seams.py`（numpy、scipy）。
