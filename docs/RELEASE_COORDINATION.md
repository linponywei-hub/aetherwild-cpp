# 遊戲內容與連線共同發布

## 本次整合來源

- 遊戲main：a8737d27653bb473a33b22e468e7e20d390f80e1（PR #6、#8：內容、倉庫、舊同步整合）。
- WebSocket發布線：d971a192ad14c435cc02089927348f6249c4196f（PR #7，已在Render上線）。
- Canvas呈現優化：fe4b443dce0f4e63c3db07e7c4e51024c5e6e66f（PR #9，納入此次整合，不另覆蓋遊戲內容）。
- 三方共同基底：8776dcaf2174e2018a85c9f0b3e4140dd624d286。
- 獨立整合分支：ai/integrated-content-realtime；經驗證後以PR合併至main。

## 衝突處理

web/client.js為實際衝突檔：同時保留contentPanel/contentFilter與RealtimeLink imports、快照順序／間隔量測與NPC位置更新、十種起始職業建立、WebSocket輸入／命令／重連與HTTP備援，以及FPS診斷。其餘git三方合併後仍需測試，不因自動合併就宣稱相容。

src/runtime.cpp保留內容指令、職業、怪物技能與_motion。src/runtime_state.cpp保留warehouseItems與contentState；未新增另一份玩家資料庫或清理舊玩家存檔。擴圖時保留內容分支對舊平地MovementPreview的停用，避免1790m邊界與錯誤平地預測；不自行自動啟用64公里世界或開放後續批次。

apps/workstation.cpp保留最新受驗證的WebSocket與非阻塞HTTP。C++仍唯一權威；網頁內容按鈕走相同runtime.command，不繞過距離、NPC、權限或去重。傳輸契約見docs/12_WebSocket與即時效能數據.md，內容契約見docs/content/INTEGRATION.md。

## 單一發布流程

1. 遊戲AI與連線AI從最新main另開分支，以PR提供修改與測試。
2. 連線／部署負責人先整合並跑原核心、runtime、content、HTTP、WebSocket、公開權限、重啟及前端測試。
3. 合併受測PR至main，記錄合併commit。
4. 讀取現有發布分支ai/equipment-warehouse-v0.2.2的head；僅在main包含該head且沒有未整合發布變更時，用expected_sha保護的非force更新將發布分支fast-forward到受測main。
5. 現有Render服務仍追蹤該發布分支，auto deploy off；手動部署一次，不新增服務、不修改秘密、不升級方案。
6. 追蹤該commit的deploy至live，驗證公開HTTPS／WSS、十起始職業、圖鑑、內容靜態模組與效能面板。不得只用GitHub合併成功宣稱線上已更新。

main是之後兩方共同基準，ai/equipment-warehouse-v0.2.2只作既有Render的發布指標。若任何head在過程中改變，停止更新該ref，先重讀並整合新commit；不可force push。角色資料不得進Git、PR或交接ZIP。

## 資料與驗證界線

本次不新增SQL資料庫，不改驗證碼，不匯入原玩家資料。既有contentState與warehouseItems同屬world-v2.json、缺少新欄位仍按內容分支相容讀取。Free重新部署可能讓本機測試角色消失，不能把原子JSON保存宣稱為跨部署永久保存。

Docker建置入口現在同時執行http_runner（含慢連線與WS）、content_http與cloud_access；content C++測試已由tools/build.py執行。這不代表所有設計與美術已完成；仍以內容交接文件列出的實作／原型／未完成範圍為準。

登入介面整合：十職業選擇使用三欄可捲動grid，角色建立卡片限制視窗高度並可捲動，避免原flex排版把新增選項與進入按鈕擠出畫面。

## 整合檢查紀錄（2026-11-07）

Release與ASan/UBSan均通過341項C++（核心171／runtime67／content103）、非阻塞／跳躍回歸、32項HTTP與重啟、23項WebSocket（增加NPC好感去重及新職業存倉）、39項內容HTTP與重啟、12項公開權限。Node 13組與內容catalog引用檢查、所有主網頁模組語法及git diff --check通過。

ASan使用detect_leaks=0。7.1MiB編譯內容的儀器化啟動超過舊cloud_access fixture的5秒窗口；該fixture比照其他內容測試改成30秒readiness窗口並加上提早退出／逾時明確錯誤，之後一般與ASan權限12項皆通過。這只影響隔離測試等待，不放寬遊戲輸入／存取權限或正式服務限制。

Windows、macOS、正式容量與使用者端幀率仍未實機驗證；容器與公開換版仍須以Render該整合commit的live狀態及公開HTTPS/WSS檢查為準。

## 待批准：打擊感與復原補丁

分支ai/combat-save-recovery；保留線上785443b，禁止自動發布此分支。已觀察到現有6個測試角色，舊版無合法匯出途徑，免費服務亦無Shell。需要使用者批准一次性測試角色重置或提供舊世界備份；付費持久磁碟另需費用同意。新API/資料範圍見docs/16_打擊感與資料復原.md。
