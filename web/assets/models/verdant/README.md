# 翠冠城模型

目前已加入兩個**本專案原創的低面數 GLB 佔位模型**，可直接由場景載入：
- `aetherwild-tree.glb`：樹冠裝飾模型
- `aetherwild-rope-bridge.glb`：木板及繩索吊橋外觀

這兩個檔案**不是** Quaternius 或 Innerscene 下載素材；不得誤標外部作者。未來可在核對授權及實際模型檔後，以外部 CC0 素材替換。

模型目前僅負責視覺；C++ 伺服器仍須補齊吊橋通行碰撞。部署前應完成 GitHub CI、渲染與通行驗證。

## 外部真實素材接入

2026-11-09 起，翠冠城樹冠優先載入 Quaternius《Stylized Nature MegaKit》CommonTree 1 的實際 CC0 GLB（由 skyline-run 公開 GitHub 專案提供），網址：`https://raw.githubusercontent.com/anshaneja5/skyline-run/main/public/assets/models/tree1.glb`。來源：https://quaternius.com/packs/stylizednaturemegakit 。本地 `aetherwild-tree.glb` 為先前原創佔位模型，不再用於樹冠。吊橋仍為本地佔位模型，尚未完成外部替換。外部網路無法存取時會保留程序式樹冠。
