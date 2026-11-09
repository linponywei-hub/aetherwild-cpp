# 2026-11-09 新手村外部 NPC 與對話

見 docs/npc/starter-village.md。18 位新手村 NPC 使用六款 Quaternius Universal 外部角色與共用真實骨架動畫，TRIPO-001 已移除。C++ 控制平滑走路、貼地、碰撞與交談停步保留；前端只呈現，原 ID／服務／信任條件保留。素材來源與校驗碼在 villagers/PROVENANCE.json。正常 C++、HTTP／WebSocket、重啟與骨架取樣驗證通過；GPU 最終外觀因測試瀏覽器 WebGL 停用尚未驗收。部署狀態以此分支的 PR 與 Render 實際結果為準。

# 最新程式結構：功能模組整理

先讀 docs/architecture/README.md。C++ 實作依功能位於 src/modules，生成資料位於 src/generated；瀏覽器模組位於 web/modules。舊 src/runtime.cpp 已拆成物理、怪物 AI、網路投影、存檔備份與核心協調。build.py、CMake、資料生成器及網頁入口已同步新路徑。services.hpp 只保留向後相容的聚合 include，定義在各系統 system.hpp。以下舊交接的功能與版本狀態仍須依實際測試確認。

# PR #14 / #15 整合：ai/combat-save-recovery

2026-11-07 使用者要求「先把目前功能上線」、「PR15」、「直接合併」，已授權整合及現有免費站部署（已說明測試角色可能一次性重置）。PR #15 的小地圖／晝夜／拉弓／雙匕首與 PR #14 的羅盤／遠端平順播放／武器前景／打擊回饋／世界備份共同保留。付費磁碟仍未批准，不建立付費資源。新功能、接口與限制見docs/16_打擊感與資料復原.md；具體待批准方案見deploy/persistent-storage-proposal.json。

本補丁加入羅盤跟隨視角、遠端角色/NPC緩衝插值、所有武器獨立前景層、命中回饋，以及Owner瀏覽器備份/匯出/冷啟動還原。角色檔仍world-v2，無SQL變更。

---

# 最新補丁入口（2026-11-07）

後文為歷史交接，部署／世界尺寸等舊敘述請以本段及 docs/15_參考地圖與完整世界補丁.md 為準。

- 共同修改與檔案清單：[PR #12](https://github.com/linponywei-hub/aetherwild-cpp/pull/12)，獨立分支 ai/reference-world-patch。
- 既有 Render 測試站：https://aetherwild-cpp-test.onrender.com/；使用既有免費服務與驗證碼，不建立新付費資源。新補丁上線狀態以該服務實際部署為準。
- 原圖25區、96km世界、全部現有內容直接載入，後續用程式補丁。不要恢復批次開放介面、全職業／隱藏入口／羈絆列表。
- 角色仍用 world-v2.json；contentState.layoutVersion=2 負責一次性座標遷移。input/command/state HTTP／WebSocket 保持相容。
- 原圖地形、海岸與港口模型：data/atlas_surface.json → tools/generate_atlas.py → src/generated/atlas_data.cpp；請同步更新原生碰撞與呈現。
- 設定：終端固定分頁，自訂按鍵、音量、渲染距離保存於瀏覽器，無資料庫變更。第一人稱武器使用獨立繪製層。

---

# 界域紀元 C++ 0.2.2：下一位 AI 接手入口

交接日期：2026-11-07（台灣）。本次收尾與共同編輯入口；尚未部署公開遊戲服務。

## 唯一共同程式來源

- Repository：https://github.com/linponywei-hub/aetherwild-cpp
- 預設分支：main。程式與雲端準備檢查點：d014855f974eaf04c65f9f69033f52942d4c5994。
- 本文件另以後續文件提交更新；開始前讀取最新 main，勿覆蓋其他人的變更。
- 公開倉庫可讀取／下載；直接推送需要已獲授權的 GitHub 連線，其他帳號使用 fork + pull request。
- 每位 AI 使用自己的工作分支（例如 ai/combat、ai/ui、ai/deploy）；任務有檔案重疊時先協調，提交前更新 main，經檢查再合併，禁止 force push main。
- 不要將任務交給彼此不能看見的本機資料夾後，聲稱已同步。以提交 SHA／PR 連結交接。

## 使用者方向與架構

停止沿用舊 JavaScript 架構；先定義子系統、資料屬性、操作與流程，再製作畫面與動作。C++20 原生服務負責規則、權限、碰撞、戰鬥、資源、保存及多人共享世界；瀏覽器負責渲染與輸入。不執行舊 game.js、Worker 或舊 API。

先讀 README.md、docs/01_子系統與依賴.md、docs/02_資料模型.md、docs/03_流程與操作.md，再讀 docs/04_移植狀態.md、docs/06_工作站測試.md。新版本以實際 C++ 與移植狀態為準。

## 已完成可操作測試版

- 20Hz C++ 世界與原生 HTTP；嚴格 JSON、角色綁定、指令去重與失敗回復。
- 玩家／生命魔力／等級職業、裝備背包倉庫、戰鬥技能、採集、鍛造、商人、任務、隊伍、PVP、管理員操作。
- 六城、共享怪物／世界王、資源重生、多人副本進度與個人寶箱、重啟恢復與原子存檔。
- 城下野外與高架城市分層、星穹升降與翠冠斜坡、摔落規則；不要增加離城空氣牆。
- 新第一人稱客戶端、終端與管理介面、Three.js 與 Canvas 相容 renderer；測試環境實際驗證 Canvas，未驗證硬體 WebGL。
- 內容 catalog 保留原 ID：84裝備、15職業、55副本、8世界王、63任務、8區域、40配方、28資源。資料存在不等於所有美術／技能／劇情已完成。

## 程式入口

include/aetherwild/domain.hpp 是領域模型，combat/inventory/gathering 等是各子系統；src/modules/runtime/runtime.cpp 是應用協調與模擬，src/modules/storage/runtime_state.cpp 是世界保存，apps/workstation.cpp 是原生 HTTP，web/ 是呈現與輸入。ports.hpp 與 SQL schema 的正式外部整合仍是介面／設計，不可宣稱 Google OAuth 或正式 SQL 已完成。

## 建置與驗證

python3 tools/build.py
python3 tests/http_runner.py
python3 tests/cloud_access.py
ASAN_OPTIONS=detect_leaks=0 python3 tools/build.py --sanitize
python3 tools/run_workstation.py

Windows：先裝 C++ Build Tools 與 CMake，再執行 OPEN_WORKSTATION_WINDOWS.cmd。沒有預編譯 exe。當程式真的在使用者本機啟動，才使用該程式顯示的 localhost 連結；遠端工作站的 localhost 不能當成使用者電腦可連的網址。

本輪紀錄：Release 與 ASan/UBSan 各202項檢查通過；原生 HTTP 18項與實際重啟通過；雲端權限12項在 Release 與 sanitizer 通過。完整輸出見 VALIDATION.txt。LeakSanitizer 未執行；Windows/macOS、CMake、Docker 容器及正式雲端未實機驗證。

## 部署停點：下一個優先事項

使用者希望自己的電腦可開啟網址測試。Dockerfile、render.yaml、DEPLOY_RENDER.md 已準備；尚無公開遊戲網址、Render 服務或付費資源。GitHub 連線與 Render 連線成功不代表遊戲已上線。

Render 已知工作區 Pony's workspace（tea-dbn4kqhsrm7s73dbpev0），目前尚未由使用者明確選定。若繼續部署，先依連線要求確認工作區，使用既有部署檔建立可審閱方案，再建置、檢查健康端點、登入與多人互動，取得真實 HTTPS 網址後才提供玩家。

目前方案為 Singapore 單一 Free Docker web service，PORT=10000，/api/health，auto deploy off。Free 存檔可能隨休眠／重啟／重新部署消失，不能宣稱持久保存；未加入付費磁碟或資料庫。Docker 未建置，Render 完整 schema／平台驗證未完成；部署時必須實際驗證。

AETHERWILD_PUBLIC=1 時必須設定至少24字元的 GAME_OWNER_CODE 與 GAME_INVITE_CODE；由部署平台生成，不能提交真值。邀請碼進入是 Player，即使是第一位訪客；擁有者碼才是 Owner。本機未設定保護時第一個角色是測試 Owner，這不是正式帳戶認證。存檔目錄可用 AETHERWILD_DATA_DIR 指定。SIGTERM/SIGINT 會保存並退出。

## 已知未完成與資料界線

完整模型與場景編輯器、所有職業技能樹／特效、正式帳戶與 Google 登入、SQL交易與持久化、大量玩家負載及正式網路安全驗證仍待做。本版是測試工作站，不能宣稱正式完整 MMO。

runtime-data/world-v2.json 及備份含角色與 session 登入映射，永不提交、公開或放交接 ZIP；不得以測試 Owner 作為正式站權限證據。密碼、環境變數真值、原玩家資料均不在本包。

下載交接包另保留完整原始 reference/original_v0.30.1.zip 與本機完整來源素材；GitHub 部署倉庫只包含執行所需素材，歷史 ZIP 不在 Git。原版 ZIP 是查核來源，不可恢復其舊架構。下載包有 MANIFEST.sha256 可核對內容。


## 內容整合分支交接

第一階段世界／職業／NPC內容整合請讀 [docs/content/INTEGRATION.md](docs/content/INTEGRATION.md) 與 [驗證紀錄](docs/content/VALIDATION.md)。設計資料的筆數不等於獨立已完成機制或美術資產數；合併部署前先檢查此處列出的原型限制與共用檔案。


## 0.2.2 裝備倉庫增量

已加入獨立裝備倉庫、卸裝、商人附近存取、JSON與AWCHAR 1→2相容讀取，以及HTTP與重啟測試；先讀 docs/08_裝備倉庫.md。此輪前端只做語法檢查，尚未手動點擊驗收。部署停點不變。
