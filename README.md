# 作品①
C++ゲームエンジンパーツ(一部抜粋) game-engine-components ※ソースコードのみ、実行不可能
実行はできませんが、、私のC++の技術や設計が最も表されているコード類です
ゲームのコアシステムとして制作していたものの一部となります
モダンな設計(コンポーネント志向,スマートポインタなど)や外部ライブラリなどを積極的に使った高度なモデル読み込み(読み込みでは部分的な非同期処理にもチャレンジしている)、DirectX9→11に移行してシェーダーを使った描画処理(renderer)などを盛り込んでいます

## 🛠 使用技術・主要ライブラリ
- **Graphics API**: DirectX 11 (DirectXTK, DirectXTex)
- **Window / Input**: SDL3
- **リソース管理・演算**:
  - Assimp (3Dモデル読み込み)
  - stb_image (テクスチャのRaw展開・画像読み込み)
  - FMOD (オーディオ制御)
  - Bullet Physics (物理演算)
- **データ形式・シリアライズ**: JsonCpp, yaml-cpp
- **UI・その他**: Dear ImGui,spdlog
※ ONNX Runtimeは導入していますが処理が完成していないため除外
