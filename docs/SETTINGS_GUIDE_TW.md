# DR_CursorTracker 設定使用說明（繁體中文）

以下為「準心追蹤器」各設定項目介紹。實際顯示會依勾選與模式自動隱藏不相干的項目。

## 方框設定
- **顯示方框 (Show Box)**: 開/關方框繪製。關閉時仍可調整大小（方便預先配置）。
- **方框大小 (Box Size)**: 方框邊長長度（像素）。
- **方框顏色 (Box Color)**: 方框顏色（ARGB）。
- **方框粗細 (Box Thickness)**: 方框邊框線寬（像素）。
- **方框透明度 (Box Alpha)**: 0.0～1.0，數值越大越不透明。

## 準心設定
- **準心模式 (Crosshair Mode)**:
  - **移動模式 (Movement Mode)**: 以滑鼠「移動量」驅動，並帶有回彈效果。
  - **座標模式 (Coordinate Mode)**: 以「螢幕座標」驅動，準心直接指向螢幕上的滑鼠位置。
- **準心最大偏移量 (Max Offset)**: 準心可離開中心的最大距離（像素）。
- **使用自訂圖片 (Use Custom Image)**: 開啟後以圖片作為準心，並隱藏內建十字準心的相關設定；同時「圓圈設定」整組會隱藏。
- **準心圖片 (Crosshair Image)**: 指定自訂準心圖片檔案路徑。
- 內建十字準心（僅在未使用自訂圖片時顯示）：
  - **準心十字長度 (Crosshair Cross Length)**: 內建十字的臂長（像素）。
  - **準心顏色 (Crosshair Color)**: 內建十字顏色（ARGB）。
  - **準心粗細 (Crosshair Thickness)**: 內建十字線寬（像素）。
  - **準心透明度 (Crosshair Alpha)**: 0.0～1.0。

## 圓圈設定（使用自訂圖片時自動隱藏整組）
- **圓圈顏色 (Circle Color)**: 外圈顏色（ARGB）。
- **圓圈粗細 (Circle Thickness)**: 外圈線寬（像素）。
- **圓圈半徑 (Circle Radius)**: 外圈半徑（像素）。
- **圓圈透明度 (Circle Alpha)**: 0.0～1.0。

## 追蹤線設定
- **顯示追蹤線 (Show Tracking Line)**: 開/關追蹤線整體功能。
- **追蹤線模式 (Tracking Line Mode)**：
  - **線性模式 (Linear Mode)**: 從中心指向準心偏移方向的一條線。顯示下列參數：
    - **追蹤線顏色 (Tracking Line Color)**: 線色（ARGB）。
    - **追蹤線粗細 (Tracking Line Thickness)**: 線寬（像素）。
    - **追蹤線透明度 (Tracking Line Alpha)**: 0.0～1.0。
  - **路徑模式 (Path Mode)**: 沿準心移動軌跡生成小圈路徑。顯示下列參數：
    - **路徑圈圈顏色 (Path Circle Color)**: 路徑小圈的顏色（ARGB）。
    - **路徑圈圈半徑 (Path Circle Radius)**: 小圈半徑（像素）。
    - **路徑生成間隔 (Path Generation Interval)**: 新點生成的距離間隔（像素）。
    - **路徑存活時間 (Path Lifetime)**: 每個路徑點的存活時間（秒）。

## 速度設定
- **回彈速度 (Rebound Speed)**: 回到中心的整體速度倍率。
- **中心回彈速度 (Center Rebound Speed)**: 準心靠近中心時的回彈速度（可與外圍分離）。
- **外圍回彈速度 (Outer Rebound Speed)**: 準心遠離中心時的回彈速度。
- **準心靈敏度 (Crosshair Sensitivity)**: 準心對滑鼠移動的敏感度。
- **準心中心移速 (Crosshair Center Move Speed)**: 準心在靠近中心時的移動速度倍率。
- **準心外圍移速 (Crosshair Outer Move Speed)**: 準心在靠近外圍時的移動速度倍率。
- **啟用靜止回彈加速 (Enable Idle Recenter)**: 啟用後，滑鼠靜止一段時間會加速回彈。
  - **靜止延遲時間 (Idle Recenter Delay)**: 停止移動多久後開始加速（秒）。
  - **靜止回彈加速時間 (Idle Recenter Time)**: 從開始加速到達最大加速所需時間（秒）。
  - **靜止回彈速度增加值 (Idle Recenter Boost)**: 最大加速帶來的速度增加量。

 
