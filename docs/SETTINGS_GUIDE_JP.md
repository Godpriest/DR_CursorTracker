# DR_CursorTracker 設定ガイド（日本語）

以下は各設定項目の概要です。選択やモードに応じて、不要な項目は自動的に非表示になります。

## ボックス設定
- **ボックスを表示 (Show Box)**: ボックス描画のオン/オフ。オフでも「ボックスサイズ」は編集可能（レイアウト調整用）。
- **ボックスサイズ (Box Size)**: 辺の長さ（px）。
- **ボックスカラー (Box Color)**: ボックスの ARGB 色。
- **ボックス太さ (Box Thickness)**: 枠線の太さ（px）。
- **ボックスアルファ (Box Alpha)**: 0.0～1.0 の透明度。

## クロスヘア設定
- **クロスヘアモード (Crosshair Mode)**:
  - **移動モード (Movement Mode)**: マウス移動量で駆動、リバウンドあり。
  - **座標モード (Coordinate Mode)**: 画面上のマウス位置に追従。
- **最大オフセット (Max Offset)**: 中心からの最大距離（px）。
- **カスタム画像を使用 (Use Custom Image)**: 画像をクロスヘアとして使用。内蔵クロスヘア設定と「円設定」グループを非表示。
- **クロスヘア画像 (Crosshair Image)**: カスタム画像ファイルのパス。
- 内蔵クロスヘア（カスタム画像未使用時のみ表示）：
  - **クロスヘア十字長さ (Crosshair Cross Length)**: 腕の長さ（px）。
  - **クロスヘアカラー (Crosshair Color)**: ARGB 色。
  - **クロスヘア太さ (Crosshair Thickness)**: 線幅（px）。
  - **クロスヘアアルファ (Crosshair Alpha)**: 0.0～1.0。

## 円設定（カスタム画像使用中はグループごと非表示）
- **円カラー (Circle Color)**: 外円の ARGB 色。
- **円太さ(ピクセル) (Circle Thickness)**: 円の線幅。
- **円半径(ピクセル) (Circle Radius)**: 円の半径。
- **円アルファ (Circle Alpha)**: 0.0～1.0 の透明度。

## トラッキングライン設定
- **トラッキングラインを表示 (Show Tracking Line)**: トラッキングラインのオン/オフ。
- **トラッキングラインモード (Tracking Line Mode)**：
  - **線形モード (Linear Mode)**: 中心からオフセット方向への直線。以下を表示：
    - **トラッキングラインカラー (Tracking Line Color)**
    - **トラッキングライン太さ (Tracking Line Thickness)**
    - **トラッキングラインアルファ (Tracking Line Alpha)**
  - **パスモード (Path Mode)**: 移動軌跡に沿って小さな円を生成。以下を表示：
    - **パス円カラー (Path Circle Color)**
    - **パス円半径 (Path Circle Radius)**
    - **パス生成間隔(ピクセル) (Path Generation Interval)**
    - **パス生存時間(秒) (Path Lifetime)**

## 速度設定
- **リバウンド速度 (Rebound Speed)**: 中心へ戻る全体速度スケール。
- **中心リバウンド速度 (Center Rebound Speed)**: 中心付近の速度。
- **外側リバウンド速度 (Outer Rebound Speed)**: 外側付近の速度。
- **クロスヘア感度 (Crosshair Sensitivity)**: マウス移動への感度。
- **クロスヘア中心移動速度 (Crosshair Center Move Speed)**: 中心付近での移動速度倍率。
- **クロスヘア外側移動速度 (Crosshair Outer Move Speed)**: 外側付近での移動速度倍率。
- **アイドルリセンターを有効化 (Enable Idle Recenter)**: アイドル後、所定の遅延を経てリバウンドが加速。
  - **アイドルリセンター遅延(秒) (Idle Recenter Delay)**: 加速開始までの待機時間。
  - **アイドルリセンター時間(秒) (Idle Recenter Time)**: 最大加速に達するまでの時間。
  - **アイドルリセンターブースト (Idle Recenter Boost)**: 最大の速度加算量。

 
