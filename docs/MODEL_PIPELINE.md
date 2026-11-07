# Aetherwild 母模型系統｜第一階段

## 已提交的功能
- `web/model-catalog.js`：HUMANOID_BASE_001 母模型、六種人形變體、角色對照。
- `web/model-assets.js`：GLB 非同步讀取、母模型共用快取、靜態網格複製、變體材質色調、失敗時維持程式化模型。
- `web/vendor/GLTFLoader.js`：針對嵌入式 GLB 2.0 靜態三角網格與常見 PBR 貼圖的輕量讀取器；不是完整 Three.js 官方 GLTFLoader，仍需實機驗證與相容性測試。
- `web/model-preview.html`：獨立模型預覽頁，網址 `/model-preview.html`，可選變體、縮放、旋轉。
- `web/scene.js`：人物依職業/角色映射到變體。

## 使用方法
1. 將使用者提供的 `Aetherwild_TRIPO-001_30k_test.glb` 複製為 `web/assets/models/characters/TRIPO-001.glb`。
2. 啟動 C++ 工作站，開啟 `http://localhost:4173/model-preview.html`（以終端機實際埠號為準）。
3. 確認貼圖、朝向、腳底原點與尺寸，再測試遊戲中角色顯示。
4. 如需新增變體，編輯 `web/model-catalog.js`。所有變體共用同一 GLB 網格與貼圖快取。

## 尚未完成與限制
- **測試 GLB 二進位檔尚未提交至 GitHub**；目前 GitHub 寫入介面僅用於文字檔。未放入 GLB 時預覽會顯示載入失敗。
- 尚未實際在瀏覽器中驗證 Three.js 相容性、材質貼圖、效能或渲染畫面。
- 本階段不包含綁骨、蒙皮、動畫、換裝、骨架複製、LOD、裝備掛點或母模型網格形變。
- 目前變體僅是共享靜態母模型的視覺設定，不代表已經做出六種不同的 3D 外觀。
- 原模型規格：Y-up、正面 +Z、腳底中心為目標規範，仍須用實際模型確認並校正。
- 靜態讀取器支援常見 GLB bufferView 與 JPEG/PNG 貼圖，但不保證涵蓋所有 glTF 擴充、壓縮與貼圖配置。

## 驗收門檻
需將真實 GLB 放入專案，啟動 C++ HTTP 服務，完成預覽、遊戲場景、多人實例、失敗備援及效能測試，才可解除 PR Draft。
