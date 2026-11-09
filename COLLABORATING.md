# 與其他 AI 共同編輯

共同來源：https://github.com/linponywei-hub/aetherwild-cpp

1. 先讀 AI_HANDOFF.md、AGENTS.md，確認最新 main 與其他進行中的 PR。
2. 有 Git 工具的 AI：clone 倉庫，從最新 main 開分支 ai/工作內容；每位 AI 使用不同分支。
3. 僅能閱讀的 AI：產出 patch 或 fork 後提出 PR；公開連結不會自動給寫入權。
4. 每次任務先說明資料模型、流程與修改檔案範圍，避免多人同改同一段。
5. 提交前完成相關測試；以 PR 說明行為改變、測試與尚未完成事項，合併前處理衝突。
6. 勿 force push main；勿提交 runtime-data、.env、憑證、玩家登入映射或原玩家資料。
7. 回報使用可點擊的 commit／PR 連結，並更新交接內容。

程式核心 C++、畫面 web、部署 Dockerfile/render.yaml 可分工；src/runtime.cpp 為共享協調點，同時修改需先協調。

可直接交給下一位 AI：
請接手《界域紀元》C++20 專案 https://github.com/linponywei-hub/aetherwild-cpp ，先讀 AI_HANDOFF.md 與 AGENTS.md，從最新 main 建立自己的分支並以 PR 交付。保留 C++ 主導規則與多人狀態、瀏覽器只負責呈現的架構。已通過202項核心／工作站檢查、18項HTTP及重啟、12項雲端權限檢查；目前尚無公開遊戲網址，下一步請完成 Docker／Render 驗證與實際部署，提供使用者自己電腦可打開的 HTTPS 測試網址，並如實處理免費方案存檔會消失的限制。不要提交密碼、session或玩家存檔。
