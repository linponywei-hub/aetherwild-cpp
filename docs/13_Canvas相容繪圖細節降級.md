# Canvas相容繪圖降級與實測界線

分支ai/canvas-render-performance。只修改web/modules/rendering/software-renderer.js與web/modules/core/client.js及本文件，不更動C++權威、API、角色存檔、資料庫或硬體WebGL場景。

新增即時面板後，雲端測試瀏覽器在可見的建立角色畫面連續顯示約1FPS。這是雲端瀏覽器資料，不能宣稱使用者電腦也只有1FPS；但可證實fallback繪圖有需處理的路徑。

Canvas逐三角形投影原始球體、圓柱與圓環，且每面都作填色／描邊。現在只在SoftwareRenderer內以WeakMap快取低細節替代幾何：球體／多面球用8面Octahedron、圓柱／圓錐4邊、圓環3×8分段。原Three場景、模型、mesh world transform、raycast和C++碰撞不變；硬體WebGL仍使用原始幾何。相容模式遠景剔除距離從170縮到110m，副本保持90m。這是降低呈現工作量，不是刪除實體或修改遊戲內容。

右上角面板補上「相容／硬體」標示，協助判讀FPS與網路延遲。先前realtime-1的960×640相容畫布上限與33ms繪圖節奏保留。

已通過JS語法與既有12組前端傳輸／預測測試。部署後需用同一瀏覽器比較實際FPS，不能僅從面數變少宣稱幀率改善倍數；使用者端也需提供面板數據。
