# 旅人介面與天賦驗證（2026-11-08 台灣）

- Release 核心：22 組、181 檢查通過。
- Release runtime：18 組、1254 檢查通過。
- 內容整合：72 檢查通過。
- HTTP：32 檢查；WebSocket：29 檢查，包含原生服務、來源與角色驗證、請求去重、移動、保存、重啟恢復；非阻塞連線測試通過。
- ASan/UBSan：核心、runtime、內容與 HTTP／WebSocket／重啟檢查通過；detect_leaks=0，未執行 LeakSanitizer。
- 前端：node --check web/client.js、git diff --check 通過。
- 新增回歸：36 節點、全部依賴路線、基本屬性、斬殺增傷、藥水增效、製作產量／經驗、點數不足、前置不足、重複學習、重複請求、死亡、角色隔離、世界 JSON 與 AWCHAR 重啟相容。
- 介面視覺與點擊：未驗收，預覽受環境 URL 存取政策阻擋。窄螢幕、硬體 WebGL 亦未驗收。
- 本次不修改正式站與正式玩家存檔；圖庫產製分支保持獨立。
