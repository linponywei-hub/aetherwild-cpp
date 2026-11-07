# Render共同發布

遊戲：https://aetherwild-cpp-test.onrender.com/
後台：https://dashboard.render.com/web/srv-dbn5vdd9fdbs739adpk0

Pony's workspace；srv-dbn5vdd9fdbs739adpk0；Free／Singapore／Docker；auto deploy off。現有服務仍追蹤ai/equipment-warehouse-v0.2.2。main作遊戲與連線共同來源，受測main合併後，安全fast-forward發布分支再手動部署；不另開服務。流程與本輪來源見docs/RELEASE_COORDINATION.md。

公開WebSocket為wss://aetherwild-cpp-test.onrender.com/api/socket，必須先用既有session token驗證；HTTP保留作初始登入與備援。契約、診斷與限制見docs/12_WebSocket與即時效能數據.md。平台目前使用預設TCP健康檢查，/api/health另行HTTPS驗證。

玩家在Render Environment取得GAME_INVITE_CODE；擁有者使用GAME_OWNER_CODE填「擁有者驗證」，不要將實際碼放進Git、PR或公開對話。PORT=10000，AETHERWILD_PUBLIC=1，AETHERWILD_DATA_DIR=/app/runtime-data。未更改已部署私密值。

Dockerfile從GCC14編譯，tools/build.py包含核心／runtime／content測試，接著跑HTTP／WebSocket／慢連線／內容HTTP／重啟／公開權限。工具腳本與assets/reference必須存在建置context。Windows、macOS與正式多人容量仍另待驗收。

角色資料仍為world-v2.json，沒有新SQL或付費磁碟。Free休眠、重啟、重建可能清除本機檔案；不可承諾永久保存或擅自升級付費。部署不自動啟用擴圖／後續開區，需擁有者在遊戲管理／故事介面依內容流程操作。
