# 翠冠城重新建置

舊程序式圓地板、欄柱、建築與先前兩個本專案佔位 GLB 已移除。舊 Quaternius 遠端樹及臨時村莊房屋不再載入。

所有新素材均由 3DAssets.dev 以 CC0 1.0 Universal 發布，已下載隨遊戲提供；玩家不向第三方模型站連線。

| 檔案 | 原始素材與來源 |
| --- | --- |
| rebuilt/bellroot-tree.glb | 原始 Bellroot 樹，僅供加工重現；https://3dassets.dev/assets/bellroot-fen-floodward-marches-ceremonial-bellroot-tre-aec588ae |
| rebuilt/hollow-bellroot.glb | 上列外部樹模型的衍生版本：放大、樹內中空、上下入口、根部與樹幹輪廓修整；不是原封不動的外部檔案 |
| rebuilt/timber-deck.glb | https://3dassets.dev/assets/quiet-lake-fishing-dock-platform-a467764f |
| rebuilt/rope-bridge.glb | https://3dassets.dev/assets/medieval-mmo-starter-realm-rope-bridge-a8d75580 |
| rebuilt/rhododendron.glb | https://3dassets.dev/assets/dense-temperate-forest-flora-rhododendron-bush-aac3fb0a |
| rebuilt/jungle-canopy.glb | https://3dassets.dev/assets/jungle-temple-and-stone-city-jungle-canopy-tree-6e668f41 |

樹木加工：`blender -b -t 2 --python tools/blender/rebuild_verdant_assets.py`。切割工具不會匯出；成品為外部樹的實際網格。這些素材原始作者自述使用 AI 工具製作，並非掃描或手工精雕的電影級素材。

配置：中央中空樹與環形木平台，四片分離的葉形木平台，以四座下垂吊橋連通；100 階木製螺旋梯由林地通往平台。橋端伸入兩端平台 0.3m，橋面端點高度按原始 GLB 量測。平台採相鄰模組拼接，使用 InstancedMesh 合批；解碼量化頂點後才烘焙節點變換。橋梁去掉石質橋台，保留外部木板、繩索與支柱。

C++ `verdant_layout.hpp` 負責地板／橋面碰撞，JS `verdant-layout.js` 負責視覺布局與移動預測。市區不再有舊的 94m 圓形空氣地板或舊建築碰撞。NPC 與抵達點位於新平台。距離翠冠城 500m 內才載入全部新模型；失敗會顯示提示並間隔重試。

尚未重製設計圖中的精靈住宅、商店、瀑布與裝飾；本次範圍為使用者指定的樹幹、樹叢、木平台、吊橋與接合。Blender 視覺檢查不等於手機 WebGL 實機驗收。
