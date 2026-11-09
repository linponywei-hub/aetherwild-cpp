# 世界 CC0 素材整合｜第一階段

狀態：候選來源已登記；**尚未下載 GLB、驗證二進位模型或完成遊戲實裝**。此清單不代表素材已可在場景中看到。

## 官方來源
| 素材 | 官方來源 | 候選用途 | 授權檢查 |
|---|---|---|---|
| Kenney Fantasy Town Kit | https://kenney.nl/assets/fantasy-town-kit | 翠冠城木屋、橋樑、樓梯、村落 | 官網 CC0；下載時保存授權快照 |
| Quaternius Medieval Village | https://quaternius.com/ | 一般村莊、室內可進入建築 | 逐包確認 CC0、檔案與版本 |
| Quaternius Fantasy Props | https://quaternius.com/ | 家具、桶、箱、野外營地 | 逐包確認 CC0、檔案與版本 |

非 CC0 素材（如 Shuriken Studio）暫不自動加入公開遊戲倉庫，需另外確認原始模型再散布限制。

## 現有專案接入規則
1. 下載模型後，檢查來源、授權、模型面數、材質貼圖、大小、旋轉、碰撞空間和 SHA256。
2. 將經檢查的 GLB 放在 `web/assets/models/world/`，不可僅以來源網址充當已導入。
3. 透過 `web/modules/modeling/model-assets.js` 的 `ModelAssets` 及既有 `model-catalog.js` 註冊本地 GLB。
4. 在 `web/modules/world/city-scene.js` / `web/modules/core/scene.js` 對應區域放置視覺模型；C++ 碰撞與通行邏輯仍為權威。
5. 翠冠城（verdant）現有 32m 樓板與 Z+150 斜坡不可因視覺替換而失效；樹洞、吊橋、階梯需單獨測試。
6. 依照 `AGENTS.md` 執行測試；禁止修改正式玩家資料或直接推送 main。

## 第一批區域分配
- 翠冠城：Fantasy Town 的木製建築、欄杆、平台；需另補古樹及可進入樹幹。
- 一般村落：Medieval Village 的房屋、牆壁、屋頂。
- 野外與室內：Fantasy Props 的家具、木箱、桶與裝飾。
- 霜白、潮港、燼鐵、曜砂、星穹：待對應主題素材及授權核對後再導入。

## 驗收條件
- 每個已用模型都有本地 GLB、來源及授權紀錄。
- GLB 能被現有 Three.js loader 正確載入；模型沒有破面或遺失材質。
- 不穿牆、不封閉城門或既有路線，C++ 伺服器碰撞測試通過。
- 實測 FPS、模型實例數、貼圖大小與瀏覽器 WebGL / Canvas 回退表現。
