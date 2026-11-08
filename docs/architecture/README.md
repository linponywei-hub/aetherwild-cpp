# 遊戲功能模組

程式以功能分工。C++ 決定遊戲規則與共享狀態；瀏覽器模組負責畫面、輸入及預測。這次整理包含先前 PR #24 的世界編輯器，沒有發布至線上服務或更動玩家資料。

| 功能 | C++ 實作 | 瀏覽器實作 | 責任 |
|---|---|---|---|
| 遊戲核心 | `src/modules/core/`、`src/modules/runtime/` | `web/modules/core/` | 共用領域資料、JSON、固定 tick、命令交易與場景組合 |
| 物理引擎 | `src/modules/physics/` | `web/modules/physics/` | 碰撞、地面、重力、跳躍、移動驗證；前端僅預測 |
| NPC 系統 | `src/modules/npc/` | `web/modules/npc/` | 服務距離與職責、好感／對話／私人委託、位置與生活狀態 |
| 怪物系統 | `src/modules/monsters/` | `web/modules/monsters/` | 追擊、重生、施法；哥布林展示與模型呈現 |
| 建模系統 | 尚無伺服器發布接口 | `web/modules/modeling/` | GLB 載入、母模型／變體、房屋調整、世界編輯器及碰撞箱草稿 |
| 戰鬥與技能 | `src/modules/combat/` | `web/modules/combat/` | 命中、武器、傷害、獎勵投遞及畫面回饋 |
| 角色成長 | `src/modules/progression/` | `web/modules/progression/` | 經驗、等級、轉職、角色與天賦面板 |
| 背包與倉庫 | `src/modules/inventory/` | `web/modules/inventory/` | 裝備、材料、穿戴、存取 |
| 製作與強化 | `src/modules/crafting/` | `web/modules/crafting/` | 配方、強化報價與規則 |
| 採集 | `src/modules/gathering/` | 共用場景及終端 | 節點、工具、預約、完成與重生 |
| 任務 | `src/modules/quests/` | `web/modules/quests/` | 接取、計數、領獎與任務面板 |
| 副本 | `src/modules/dungeons/` | 共用場景 | 入場、機關、首領、寶箱及離場 |
| 商店與經濟 | `src/modules/economy/` | NPC 及物品面板 | 交易、金幣及支付驗證 |
| 多人與隊伍 | `src/modules/social/` | `web/modules/social/` | 隊伍、管理與 PvP 規則、隊伍面板 |
| 網路 | `src/modules/network/`、`apps/workstation.cpp` | `web/modules/network/` | 登入驗證、catalog／玩家快照、HTTP／WebSocket、插值與順序 |
| 存檔 | `src/modules/storage/` | `web/modules/storage/` | 世界序列化、原子保存、Owner 備份與還原 |
| 世界與地形 | `src/modules/world/` | `web/modules/world/` | 區域、城市、地形、擴圖、光照及內容投影 |
| 渲染與角色外觀 | 共用快照 | `web/modules/rendering/`、`web/modules/actors/` | 繪製流程、Canvas 降級、合批、角色與武器外觀 |
| 介面與圖庫 | 共用權限規則 | `web/modules/ui/`、`artwork/`、`admin/`、`settings/` | 操作綁定、地圖、圖庫、管理及設定面板 |

## 入口與依賴

- `apps/cli.cpp`：命令列工作站入口；`apps/workstation.cpp`：HTTP／WebSocket 服務入口。
- `include/aetherwild/<功能>/system.hpp`：各功能的公開介面。`services.hpp` 僅聚合 include 供既有呼叫者相容使用，沒有重複定義。
- `domain.hpp`：各系統共享的領域資料；`runtime.hpp`：應用協調介面。協調器只在固定流程中呼叫各系統，沒有另建一份世界資料庫。
- `src/generated/`：由 `data/` 生成的 C++ catalog、atlas 與 content 分片。分片用於 MSVC 常值大小限制，是完整內容的不同區段，不是重複檔案。
- `web/index.html`、`model-preview.html`、`world-editor.html`：穩定頁面入口；`web/vendor/`：第三方程式及授權。
- `tools/`：建置、資料生成、模型檢查與 Blender 加工；`.github/workflows/`：相關 CI。不因某次執行失敗就刪除仍有用途的工具。

核心協調 → 物理、NPC、怪物、戰鬥、物品等功能；功能依賴共用領域資料。規則仍由 C++ 執行。前端按功能匯入模組，終端各面板以參數取得狀態和顯示工具；不私自改寫伺服器規則。

`RealmScene` 使用明確匯入的功能 adapter 組合原有 API，避免改變玩家操作：房屋、怪物與角色載入已各自分離。哥布林展示 AI 屬既有視覺測試，不是伺服器傷害規則。

## 清理與保留

[刪除明細](cleanup.json) 記錄每個刪除路徑、保留版本、大小及重複檔的 SHA-256；合計刪除 7 個檔案、50,316,042 bytes（約 48 MiB）。

- 刪除根目錄房屋 01–03、重複艾琳 GLB、錯誤雙點檔名的哥布林 GLB，以及兩個只有換行的占位檔。
- 房屋 04、05 根目錄版本與正在使用的版本內容不同，移至 `assets/source/village/`，保留原始素材。
- NPC 肖像、物品圖示的相同圖案可能對應不同遊戲 ID；它們是有用途的語義別名。第三方授權也各有歸屬，保留這些檔案。
- `runtime-data/` 及原玩家資料不清除、不提交。建置與模型加工暫存目錄加入忽略規則，防止誤收錄。
- 舊的第二套內容測試編譯腳本改為呼叫共同建置入口，避免兩套編譯邏輯漂移。

[路徑對照](path-migration.json) 可查原檔案的位置。`runtime.cpp` 的物理、AI、快照、登入及備份函式已拆到對應模組；該對照的單一路徑指向剩餘核心協調器。

## 驗證

```sh
python3 tools/check_modules.py
node --test tests/*.test.mjs
python3 tools/build.py
python3 tests/http_runner.py
python3 tests/content_http.py
python3 tests/cloud_access.py
```

CMake 會為每個原生功能建立獨立 object target，再組合為核心函式庫；Python 建置以完整模組路徑命名 `.o`，避免同名來源互相覆蓋。生成器均寫入新路徑。驗證結果見 [VALIDATION.md](VALIDATION.md)。

世界編輯器的碰撞箱仍是可保存的草稿資料，尚未成為原生物理碰撞。這次整理不把尚未完成的功能宣稱為已上線。
