# Render 連線與部署（目前實際配置）

服務：https://aetherwild-cpp-test.onrender.com/
後台：https://dashboard.render.com/web/srv-dbn5vdd9fdbs739adpk0

工作區Pony's workspace已由使用者確認。Free／Singapore／Docker，單一服務，不含付費磁碟或資料庫；auto deploy off。直接建立服務，未建立Blueprint。render.yaml是配置參考；目前平台健康檢查使用預設TCP，原生/api/health另以HTTPS檢查。

已上線來源：ai/equipment-warehouse-v0.2.2，commit a5b533b54785a85957a0495d5c7dc1085dc8d9a1；deploy dep-dbn60st6e07s73dnvjv0。後續部署修正獨立在ai/render-deployment分支，與遊戲內容分開整合；服務不會因這個分支推送而自動換版。

## 玩家登入

從Render後台進入Environment查看GAME_INVITE_CODE，貼到遊戲「測試邀請碼」。擁有者使用GAME_OWNER_CODE，展開「擁有者驗證」後填入；擁有者碼不可交給一般玩家。不要把任何碼貼入原始碼、PR或交接文件。

## 建置與環境

Dockerfile使用GCC14編譯原生C++，執行核心／runtime／HTTP／cloud-access測試後產生執行映像。tools/build.py必須存在遠端倉庫及Docker context；.dockerignore僅排除建置輸出目錄，不得用build*排掉build.py，也不得排除assets/reference執行素材。

PORT=10000，監聽0.0.0.0；AETHERWILD_PUBLIC=1；AETHERWILD_DATA_DIR=/app/runtime-data。GAME_OWNER_CODE和GAME_INVITE_CODE已以隨機值設定於Render（不記錄真值）；本服務由MCP直接建立，不是generateValue同步中的Blueprint。SIGTERM/SIGINT保存後退出。

## 持久化與整合邊界

角色JSON與API介面保持現有設計；共同契約及分工請讀docs/07_連線與儲存介面.md。本部署分支不修改領域、角色序列化、技能、任務或資料庫。

Free服務休眠、重啟或重新部署可能清除本機檔案，world-v2.json不是永久保存；目前沒有外部資料庫或備份。不得未經使用者同意升級付費。長期保存需另外完成存儲設計與遷移。

## 驗證

Render確認live；HTTPS /api/health、受邀Player登入、首頁與client.js／scene.js資產檢查通過；瀏覽器顯示角色建立介面與3D背景。尚未驗證雲端大量玩家容量或長期保存；兩客戶端持續input的完整雲端多人驗收仍待執行。

部署新內容前先合併受測分支、確認來源commit，告知Free存檔影響，手動觸發一次部署並追蹤至live，再實際檢查HTTPS與登入；不要以建立服務成功代替上線驗證。
