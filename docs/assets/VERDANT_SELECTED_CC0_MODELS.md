# 翠冠城外部模型選型（待二進位匯入）

本文件記錄已確認的 CC0 候選，**不代表模型已下載或上線**。

| 用途 | 選定來源 | 格式 | 授權 | 來源 |
|---|---|---|---|---|
| 樹冠、枝葉、外圍森林 | Quaternius Stylized Nature MegaKit（選用其樹木變體，非實心主樹幹） | glTF / 轉 GLB | CC0 | https://quaternius.com/packs/stylizednaturemegakit |
| 樹屋間懸索吊橋 | Innerscene Rope Bridge | GLB | CC0（依來源頁宣告） | https://www.innerscene.com/tools/library/3d-parts/rope-bridge-4cb20765 |

## 遊戲整合規格

1. 將原始素材與來源、授權證明存於素材紀錄；僅將驗證過的 GLB 放進 `web/assets/models/`。
2. 主樹幹維持可通行的中空程式模型，外部樹木素材只用於樹冠及裝飾，不覆蓋入口。
3. 吊橋模型須按實際平台距離調整，**不得直接用渲染網格取代 C++ 碰撞**；通行需另建伺服器碰撞與測試。
4. 大型吊橋模型可能影響效能；匯入前檢查三角形數、貼圖大小及 LOD。
5. 在完成資產匯入、瀏覽器渲染、C++ 樓梯／吊橋通行測試前，PR 保持草稿。
