> 功能模組入口與依賴請先讀 [模組架構](docs/architecture/README.md)。本次保留 C++ 規則與存檔格式，詳細搬移及刪除記錄見該目錄。

# 界域紀元｜C++ 工作站測試版 0.2.2

本版已由純文字核心，接成可操作的第一人稱遊戲工作站。C++20 原生程式執行角色、移動、碰撞、物理、戰鬥、AI、採集、製作、任務、共享副本、隊伍、PvP、權限與保存。瀏覽器只負責輸入、場景與UI，沒有恢復舊game.js或Worker遊戲規則。

**這是可玩、可測試的工作站版本，還不是原遊戲所有功能等價的正式MMORPG。** 正式Google登入、跨機器部署、大量玩家負載、完整模型／場景編輯器，以及所有隱藏職業特效尚未完成，詳見docs/04_移植狀態.md。

## 雲端部署準備

已加入Dockerfile、render.yaml、邀請碼與擁有者驗證，以及正常關機保存。完整設定與Free存檔限制請讀[DEPLOY_RENDER.md](DEPLOY_RENDER.md)。**倉庫上傳不代表服務已上線，尚須完成Render規劃、建置與網址驗證。**

## 直接測試

在已開啟的工作站畫面，選擇角色名稱與戰士／法師／遊俠，按「踏入界域」。如果已有本機角色，選「繼續上次冒險」。

| 操作 | 按鍵 |
|---|---|
| 進入滑鼠環視 | 點擊畫面／「開始探索」 |
| 移動／奔跑／跳躍 | W A S D／Shift／Space |
| 普攻／武器技／副本技能 | 滑鼠左鍵／Q／F |
| 互動／採集／進副本／機關／寶箱 | 走近後 E |
| 治療藥水 | R |
| 旅人終端／設定／聊天 | B或V／P／C |
| 釋放游標 | Esc |

第一個建立的角色是**工作站測試擁有者**。在旅人終端→管理，可領取Lv.60測試補給、前往六城或副本入口、開關功能與PvP、發放測試管理員。這不是正式登入，也不會修改原站玩家資料。

完整測試路線在 `docs/06_工作站測試.md`。原v0.30.1完整封存於下載交接ZIP的reference，只供核對，不參與執行；部署倉庫不附該歷史archive。

## Windows 本機執行

需求：Visual Studio 2022 Build Tools「使用C++的桌面開發」與CMake。解壓後雙擊 **OPEN_WORKSTATION_WINDOWS.cmd**。

腳本會編譯、跑核心與工作站測試、啟動C++服務，再開啟本機瀏覽器。結束時關閉C++服務視窗即可。沒有預先編譯的Windows EXE；Windows／macOS尚未實機測試。

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\aetherwild_workstation.exe --root . --port 4173
```

之後用瀏覽器開啟程式輸出的本機網址。若使用單組態產物，執行檔在build/aetherwild_workstation.exe。

## Linux／macOS

```sh
python3 tools/build.py
python3 tools/run_workstation.py
```

或使用CMake建置。一般建置與執行不需Node或npm；package.json的dev指令只供工作站預覽管理器啟動C++程式。

可用的原生執行檔：

- aetherwild_workstation：原生C++ HTTP服務與權威20Hz模擬。
- aetherwild：原本的文字核心測試介面。
- aetherwild_tests：原核心測試。
- aetherwild_runtime_tests：工作站、共享副本、保存、樓層與升降台測試。

## 架構

| 路徑 | 職責 |
|---|---|
| include/aetherwild/domain.hpp | 明確的角色、物品、怪物、任務、資源與副本資料模型 |
| src/modules/combat/combat.cpp、inventory.cpp等 | 原生C++領域規則 |
| src/modules/physics/geometry.cpp | 原生建物碰撞、分層樓板、翠冠斜坡與遮擋 |
| src/modules/runtime/ | 固定tick、初始化與命令交易協調；物理、AI、登入、快照及備份已分離至各功能模組 |
| src/modules/storage/runtime_state.cpp | 世界、角色、隊伍、獎勵、請求去重及升降狀態的版本化保存 |
| src/modules/core/json.cpp | 有深度、長度、整數與Unicode檢查的JSON解析器 |
| apps/workstation.cpp | C++ HTTP、靜態資產、請求限制與API介面 |
| web/modules/core/scene.js | 新的場景、模型、光影與鏡頭呈現 |
| web/modules/rendering/software-renderer.js | 無WebGL時使用相同場景與鏡頭的Canvas相容顯示 |
| web/modules/core/client.js | 新的UI、按鍵與命令送出；不計算權威傷害或發物品 |
| data/legacy_catalog.json | 從原遊戲取出的內容資料，保留原ID |
| tools/generate_catalog.py | 離線產生型別化C++目錄，執行時不讀舊JS模組 |
| tests/ | 核心、工作站、HTTP、重啟與CLI驗證 |
| docs/ | 模型、邊界、流程、驗收與實際移植狀態 |

瀏覽器支援WebGL時使用3D光影；無WebGL的工作站會自動改用Canvas相容顯示，不把白畫面當成啟動成功。模型是新的程式化低多邊形呈現，沒有宣稱已完成正式美術模型。

## 保存與多人測試

原生服務會寫入 `runtime-data/world-v2.json`，重要命令成功前保存，另外每10秒檢查點；前一份保留.bak。保存包含角色、物品、世界王、共享副本、隊伍、權限、獎勵去重、升降台與工作站登入映射。

下載包不含目前工作站的執行資料或登入憑證；解壓後是全新測試世界。開始但尚未完成的採集預約於重啟時取消，已完成的材料與獎勵按共同檢查點恢復。這個檔案不是正式站SQL匯入器。

不同視窗可各自建立角色。各視窗會保留自己當次選用的登入；在同一視窗重新整理時使用瀏覽器最後保存的角色。請在同一C++服務內測試隊伍、聊天、共同副本與PvP；未設定正式網際網路部署。

## 驗證

```sh
python3 tools/build.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/build.py --sanitize
python3 tests/http_runner.py
python3 tools/smoke.py
```

最後實測輸出與限制見 VALIDATION.txt。此環境LeakSanitizer不能在受追蹤程序下執行，因此只將ASan非法存取與UBSan結果列為通過，沒有宣稱通過洩漏檢查。

