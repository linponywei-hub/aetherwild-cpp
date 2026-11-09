# 第一階段驗證紀錄

基底 main: `4c23347028feff491ca50d817e15a19edd219910`。Linux g++ / C++20，測試資料均在隔離暫存目錄。

| 檢查 | 結果 |
|---|---|
| 原核心 | 152 checks 通過 |
| 原 runtime | 50 checks 通過 |
| 內容整合 C++ | 103 checks 通過 |
| ASan + UBSan C++ | 同上305 checks 通過，detect_leaks=0；未測LeakSanitizer |
| 內容 HTTP | 34 checks，包含真程序重啟，通過 |
| ASan + UBSan 內容 HTTP | 34 checks含重啟，通過；啟動等待從7.5秒延長為30秒供儀器化建置使用 |
| 原 HTTP | 18 checks與存檔重啟通過 |
| 公開連線與權限 | 12 checks 通過 |
| 內容資料 | 數量、唯一ID、職業技能引用、每批五區與第二批王城通過 |
| 網頁 JS | client、scene、content-panel、content-scene 的 node --check 通過 |
| diff | git diff --check 通過 |

未執行 CMake（環境未安裝）、Windows/MSVC、Render正式部署、負載與長時間多人測試。網頁畫面驗收受環境限制：Chromium程序socket權限被阻擋，雲端瀏覽器不能存取測試loopback；沒有宣稱視覺或動畫驗收通過。

此驗證不代表所有素材設計已完整實作，範圍及待辦請讀 [INTEGRATION.md](INTEGRATION.md)。
