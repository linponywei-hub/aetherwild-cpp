# 持續連線與即時診斷（realtime-1）

獨立分支ai/realtime-transport，以線上8776dcaf2174e2018a85c9f0b3e4140dd624d286為基準。只處理傳輸、呈現效能與診斷，不新增遊戲內容、角色存檔欄位或資料庫。

## 原因與證據的邊界

先前HTTP路徑仍由sending旗標等待前一個移動回應，才可送下一個輸入；網路慢時，按鍵更新頻率因此被往返時間限制，伺服器原0.45秒輸入有效期也可能過期。已有的非阻塞HTTP與本機預測仍保留，但不能取代持續傳送輸入。

此環境對測試站health測得約5–6秒；對照example.com同樣約5秒，故不能用這組執行環境數字宣稱Render或使用者路徑本身延遲5秒。新版直接在玩家瀏覽器量測連線往返、實際render FPS與預測校正，提供複製數據，避免繼續依推測調整。

## 新增傳輸（既有HTTP仍相容）

GET /api/socket使用WebSocket升級。公開站為wss://aetherwild-cpp-test.onrender.com/api/socket，與HTTP共用原生C++服務與PORT；不建立另一台主機或額外收費資源。HTTP API路徑與request去重規則不變。

升級要求同源Origin與Host、版本13及有效握手key。升級不代表登入；第一個JSON訊息必須驗證既有session token，不能將token放URL、日誌或複製數據。

| channel | JSON封套 | 回應 |
|---|---|---|
| auth | {channel:"auth",id:"auth",token:"既有session"} | 本人快照；無效token拒絕並關閉 |
| input | {channel:"input",id:"i123",body:{sequence,x,z,yaw,sprint,jump}} | 最新快照，最多約10Hz（跳躍可立即回應）；並非每個input都回覆 |
| command | {channel:"command",id:"r123",body:{request,type,target?,...}} | 沿用runtime.command與權限、去重、保存 |
| state | {channel:"state",id:"r124"} | 同GET /api/state |

所有JSON回應沿用原快照及_sync，額外_reply對應訊息id；錯誤{ok:false,error,_reply}。id僅做傳輸配對，command.body.request仍是業務去重ID，兩者不可互換。input的sequence仍由C++驗證；不接受客戶端指定權威位置。

前端每50ms直接送出新的移動意圖，不等待上一個回應；伺服器20Hz模擬維持不變。快照節流至約10Hz，避免每個輸入都產生完整世界資料。命令走Promise配對，不自動重送已逾時的命令。若WebSocket斷開／不可用，回退既有HTTP並嘗試重連；HTTP備援在高延遲路徑仍可能卡，不冒稱等同持續連線。

## 有界I/O與協定範圍

原生服務保留單執行緒世界所有權、非阻塞收送與48條在途連線上限。每輪每client最多處理4個frame，接收訊息上限16KiB、未送輸出上限512KiB；已送前綴定期壓縮，避免慢速接收時buffer無限增長。未驗證連線無進度5秒、已驗證45秒回收。前端socket傳送緩衝超過64KiB會略過當次移動發送，不無限排隊。

實作文字JSON子集：masked client frame、長度欄位、文字fragment重組、控制frame交錯、ping/pong、close、UTF-8檢查。未協商壓縮或binary channel；不是聲稱已完成完整協定相容認證或正式安全稽核。SHA-1僅用於RFC6455公開握手，不用於密碼。

## 呈現與數據

右上角顯示：目前傳輸（WebSocket／HTTP備援）、實際render FPS、瀏覽器往返ms與最近校正m；「複製效能數據」只複製build、傳輸、FPS、RTT、快照間隔、校正與renderer種類，不含token、玩家姓名、ID、座標或存檔。數據不自動上傳。

Canvas相容模式原本強制至少70ms才畫一次（理論上限約14FPS），改為33ms節奏，並把相容畫布內部解析度限制在960×640內，CSS仍填滿視窗。這是降低繪圖工作量，不保證裝置能達到30FPS。硬體WebGL的陰影、場景與模型不在此次變更。

前端有未收到訊息的連線watchdog、重連與HTTP備援；背景分頁停止送移動，返回前景再恢復。掉線時本機預測仍保留先前的停止條件，不允許永久自行移動。

## 修改檔案與整合

apps/workstation.cpp；新增include/aetherwild/websocket.hpp（header-only，不新增C++套件依賴）；新增web/modules/network/realtime-link.js；web/modules/core/client.js；web/modules/core/scene.js；web/modules/rendering/software-renderer.js；web/modules/physics/movement-preview.js（增加校正統計）；tests/http_runner.py；新增tests/websocket_http.py與tests/realtime_link.test.mjs；本文件。

API新增一個socket入口與JSON封套，存檔／資料庫變更為零。另一個遊戲AI請沿用runtime.input／runtime.command，不建立另一份玩家狀態；共享client.js／scene.js需逐段整合，不整檔覆蓋。

## 驗證

Release與ASan/UBSan各通過：17項真實socket WebSocket檢查（標準握手accept、身份、Origin、mask／fragment／UTF8／上限、ping／close、command去重、1秒不讀回應仍持續送20個輸入且移動超過4m），既有非阻塞回歸、32項HTTP與實際重啟、12項cloud access。

Node共12組，包括20個輸入不等回應可連續送出、RPC配對、backpressure、斷線拒絕與清除舊callback，及先前預測／順序測試。JS語法檢查通過。ASan使用detect_leaks=0；Windows／macOS、正式容量與玩家端實際改善尚未驗證。

部署後必須再確認Render狀態live與公開wss升級；不能只依本機socket測試宣稱公開持續連線成功。免費服務重新部署可能重設測試角色。

參考：RFC6455 https://www.rfc-editor.org/rfc/rfc6455.html ；Render https://render.com/docs/websocket
