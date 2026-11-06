# Render 雲端測試部署

## 本次設定

- 單一C++ Docker Web Service，名稱aetherwild-cpp-test。
- Free運算方案，新加坡區域；不建立付費資料庫、磁碟或其他服務。
- 根目錄Dockerfile建置原生C++，建置階段執行核心及工作站測試。
- 綁定0.0.0.0:$PORT，HTTP健康端點/api/health。
- 關閉自動部署，避免每次原始碼更新都直接替換測試世界。
- GAME_OWNER_CODE、GAME_INVITE_CODE由Render以generateValue產生，原始碼不包含實際值。
- SIGTERM/SIGINT會停止接收新操作，保存世界檢查點後結束程序。

這些檔案已準備，**不代表雲端服務已建立或部署成功**。GitHub上傳、Render工作區確認、Blueprint規劃與核准、Docker建置、live狀態及實際網址驗證都必須分別完成。

## 登入測試世界

在Render服務的Environment中查看GAME_OWNER_CODE；使用網站建立角色畫面的「擁有者驗證」欄位登入。一般測試玩家使用GAME_INVITE_CODE。不要把擁有者碼提供給一般玩家。

在雲端模式，第一個只有邀請碼的訪客仍是Player，不能搶先取得Owner。未正確配置足夠長度的兩個碼時，雲端服務會拒絕啟動。

不要在聊天、原始碼、Git提交、日誌或截圖公開實際碼。只有Game Session令牌與角色綁定會保存於執行資料，不保存原始邀請碼或擁有者碼。

## Free方案的保存限制

Render Free Web Service閒置15分鐘會休眠；休眠、重啟與重新部署會清除本機檔案。這個版本目前使用世界JSON檔，因此雲端測試角色進度並非永久。即使畫面顯示存檔成功，也只能保證當前執行個體內的檢查點，不會讓Free檔案系統變永久。

需要長期保存時，另行評估並確認付費磁碟或正式資料庫方案；不可自動升級付費。此測試服務不承諾正式MMO容量。

參考：https://render.com/docs/free

## 驗證與限制

- 原生GCC建置與202項既有核心／工作站檢查。
- tests/cloud_access.py：12項雲端啟動、身份、去重、秘密保存及正常關機檢查。
- tests/http_runner.py：18項HTTP整合與實際重啟檢查。
- render.yaml已通過本機YAML語法、單一Free服務與秘密生成設定檢查。
- 本環境無Docker及Render CLI，未實際建立容器，也未執行Render平台驗證。必須以Render規劃／建置結果補齊驗證，不能把原生建置通過當成容器或雲端部署通過。

## 原始交接包

部署倉庫只包含執行所需的新原始碼與素材。原v0.30.1完整封存仍在下載交接ZIP的reference/original_v0.30.1.zip；不另上傳到部署倉庫。較早的MANIFEST只適用對應下載ZIP，不作為此Git倉庫清單。
