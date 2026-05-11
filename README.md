# 作品①
C++ゲームエンジンパーツ(一部抜粋) game-engine-components ※ソースコードのみ、実行不可能
実行はできませんが、、私のC++の技術や設計が最も表されているコード類です
ゲームのコアシステムとして制作していたものの一部となります
コンポーネント志向やスマートポインタなどモダンな設計を意識しながら書いたものです

## 🛠 使用技術・主要ライブラリ
- **Graphics API**: DirectX 11 (DirectXTK, DirectXTex)
- **Window / Input**: SDL3
- **リソース管理・演算**:
  - Assimp (3Dモデル読み込み)
  - FMOD (オーディオ制御)
  - Bullet Physics (物理演算)
- **データ形式・シリアライズ**: JsonCpp, yaml-cpp
- **UI・その他**: Dear ImGui
※ ONNX Runtimeは導入していますが処理が完成していないため除外
