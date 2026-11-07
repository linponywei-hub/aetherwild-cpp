# 線上版本相容整合

本次整合PR #6世界內容、PR #2部署修正及PR #1既有線上裝備倉庫／同步功能。沒有清除玩家資料、改驗證碼、升級方案或自動開放後續區域。

- 衝突解法：同時保留contentState、warehouseItems、_motion與_sync欄位；職業、折扣與故事命令保留，另加入unequip/store_item/withdraw_item。
- 網頁同時提供內容圖鑑、新職業選擇、羈絆、故事與裝備倉庫。保留非阻塞HTTP與快照排序。
- 舊世界保留移動預覽；擴圖時停用只認識舊1790公尺邊界及平地的預覽，依伺服器位置呈現。後續可使用伺服器地形網格擴充視覺預覽；不得自行發送玩家座標取代伺服器規則。
- tools/build.py會編譯並執行新的content C++測試；Docker部署仍使用既有入口。

## 驗證

Linux g++：核心171、runtime67、內容103，共341項C++檢查通過；相同341項ASan/UBSan通過（detect_leaks=0，未測LeakSanitizer）。一般及ASan/UBSan內容HTTP39項含實際重啟通過，涵蓋新職業武器存倉、不同玩家隔離、擴圖後存檔與重啟保留。

一般及ASan/UBSan HTTP32項含重啟、非阻塞慢連線／慢讀者／跳躍檢查通過。公開權限12項在一般及ASan/UBSan通過。瀏覽器移動預覽／快照排序9項Node測試通過；四個主網頁模組語法、內容資料檢查及git diff --check通過。儀器化HTTP測試啟動等候上限為30秒，不影響正式程式啟動設定。

CMake、Windows、容器建置與線上換版仍需各自驗收。先前的VALIDATION.md是第一階段305項的紀錄，本文件記錄合併後的檢查。每一筆技能規格仍不等於一套完成動畫；完整功能範圍見INTEGRATION.md。

## 上線步驟

目前原線上來源為ai/equipment-warehouse-v0.2.2，auto deploy off。僅合併GitHub不會更新服務；需將既有Render服務srv-dbn5vdd9fdbs739adpk0的來源切成受測main整合commit，再手動部署，追蹤至live，驗證health、catalog十起始職業及圖鑑/羈絆/故事模組。Free服務重新部署可能重設本機測試角色進度，已在本次對話告知。

部署完成後，以擁有者登入「管理」頁啟用64公里世界，第一批五區先開，王城維持第二批。若有玩家仍在副本，伺服器會拒絕切換；先讓隊伍退出，不直接改玩家存檔。

## WebSocket後續整合

最新連線以docs/12_WebSocket與即時效能數據.md為準。遊戲內容與WebSocket／Canvas效能分支已在獨立ai/integrated-content-realtime進行三方整合，保留既有profile、warehouseItems、contentState、_sync與_motion；擴圖仍停用舊平地預測。發布來源會先合併至main，再以安全fast-forward同步既有Render追蹤分支；不必改Render來源設定或新增服務。詳見docs/RELEASE_COORDINATION.md。
