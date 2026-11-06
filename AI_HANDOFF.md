# 界域紀元 C++ 0.2.0 接手入口

使用者要求：停止舊架構，先資料與子系統，再操作流程，之後畫面與動作；現在已進一步要求可直接操作的工作站。

1. 本版已有原生C++ HTTP服務與20Hz遊戲模擬、共享玩家／怪物／資源／副本／隊伍與版本化世界保存。
2. 新瀏覽器端只負責畫面、鏡頭、輸入與UI；web/vendor/three是渲染函式庫。沒有執行原game.js、Worker或舊API。不要誤稱純文字版，也不要誤稱全功能正式MMO。
3. 先讀README、docs/04與docs/06。docs/01..03是第一階段模型與流程基礎，與新版不同之處以本版狀態表與實際C++為準。
4. src/runtime.cpp是應用路由與模擬；領域規則繼續放combat/inventory/gathering等子系統。新功能先定義不變條件與測試，不堆到web/client.js中。
5. native服務使用runtime-data/world-v2.json及.bak，包含測試登入映射，禁止放入下載包／Git／公開分享。未完成採集預約重啟時取消，已完成資料由原子世界檢查點恢復。
6. 第一個工作站角色是本機測試Owner。Google OAuth、正式部署、負載與完整編輯器未完成；ports.hpp與SQLschema仍是正式系統的介面／設計草案，不能宣稱已接上。
7. 編譯與測試：python3 tools/build.py；ASAN_OPTIONS=detect_leaks=0 python3 tools/build.py --sanitize；python3 tests/http_runner.py。HTTP runner啟動隔離fixture，不使用或修改工作站角色。
8. 開啟：python3 tools/run_workstation.py，或Windows OPEN_WORKSTATION_WINDOWS.cmd。受管理環境使用sites-preview start專案路徑，不另開預覽伺服器。
9. 無WebGL時自動使用SoftwareRenderer。此環境實際操作的是Canvas相容顯示，不代表已測硬體WebGL。Windows／macOS與CMake尚未實機驗證。
10. 完整原始ZIP位於reference，內容與最初輸入相同。它是來源查核，不是執行架構，也沒有原站玩家資料、雲端GLB與OAuth秘密。

後續尚需完整模型／場景編輯器、所有職業特效與技能樹、正式認證與SQL交易、正式網路部署及負載測試；詳見docs/04。保留星穹上下站、翠冠斜坡、城下野外與摔落規則的驗收，不可加入離城空氣牆。
