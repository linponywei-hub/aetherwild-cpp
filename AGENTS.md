# 界域紀元 C++20 工作站

先讀AI_HANDOFF.md與docs/04、docs/06。使用者已要求停止舊JavaScript架構；reference交接規則中要求沿用舊架構的部分不適用。

- 規則、資料、碰撞、戰鬥、物品、保存與多人狀態放C++；瀏覽器只呈現及送命令。
- 新功能先定義資料、前置條件、狀態轉移、失敗不變條件與測試，再設計畫面／動畫。
- 原ID與繁體中文保留。reference與assets/reference只作來源／美術素材，不恢復舊game.js或Worker。
- 建置python3 tools/build.py或CMake。工作站python3 tools/run_workstation.py；Windows有OPEN_WORKSTATION_WINDOWS.cmd。
- 資料變更後python3 tools/generate_catalog.py。runtime JSON、HTTP、物理變更要跑runtime_tests、http_runner及適當sanitizer。
- runtime-data含工作站登入映射，永不提交或放下載包；只保存新程式碼、素材及驗證結果。
- 正式OAuth、SQL持久化、部署、全部編輯器與負載驗證仍未完成，狀態必須如實描述。
- 不修改正式站／原玩家資料。本機Owner不等於正式站擁有者驗證。
