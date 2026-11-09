# 翠冠城外部 GLB 檔案放置說明

目前已完成 Three.js 的可選模型載入與場景接線，但**尚未取得二進位模型**。

需放置：
- `quaternius-tree.glb`：由 Quaternius Stylized Nature MegaKit 的 CC0 樹木 glTF 轉換，僅作樹冠裝飾。來源：https://quaternius.com/packs/stylizednaturemegakit
- `innerscene-rope-bridge.glb`：Innerscene Rope Bridge，CC0。來源：https://www.innerscene.com/tools/library/3d-parts/rope-bridge-4cb20765

路徑為 `web/assets/models/verdant/`。下載時保留授權來源證據，核對 GLB magic、貼圖和尺寸，完成後才能在 PR 宣告資產實裝。缺檔時仍使用程序化樹木和木橋。

注意：載入的外部 GLB 僅為視覺模型，尚無伺服器橋面碰撞。實際上線前須執行渲染、效能與 C++ 通行驗收。
