# 作品①
C++ゲームエンジンパーツ(一部抜粋) game-engine-components ※ソースコードのみ、実行不可能  
実行はできませんが、私のC++の技術や設計が最も表されているコード類です
ゲームのコアシステムとして制作していたものの一部となります
モダンな設計(コンポーネント指向,スマートポインタなど)や外部ライブラリなどを積極的に使った高度なモデル読み込み(読み込みでは部分的な非同期処理にもチャレンジしている)、DirectX9→11に移行してシェーダーを使った描画処理(renderer)などを盛り込んでいます

## 挑戦した技術・アピールポイント
1. **モダンなC++設計（コンポーネント指向とメモリ管理）**
   - 従来の継承による設計を避け、Component指向を意識した設計を採用しました。
   - std::shared_ptrやstd::weak_ptr,ComPtr等のスマートポインタを徹底し、安全に管理を行っています。
2. **非同期リソース読み込み**
   - 外部ライブラリを活用した読み込みにおいて部分的な非同期処理（スレッド処理）の実装にチャレンジしました。
3. **DirectX 11ベースのレンダリングパイプライン**
   - DirectX 9等のレガシーAPIからDX11へ移行し、独自のシェーダー（HLSL）を組み込んだ描画処理（Renderer）を構築しました。
  
## 💻 主なソースコード説明

特に私の設計意図や技術力が反映されているソースコードです。リンクから該当のコードを直接ご確認いただけます。

| ファイル | 概要・アピールポイント |
| :--- | :--- |
| **【SDL3を用いたWindowsとの連携】** | |
| [`common/entry.h`](common/entry.h)<br>[`common/window.h`](common/window.h)<br>[`common/window.cpp`](common/window.cpp)<br>[`common/input.h`](common/input.h)<br>[`common/input.cpp`](common/input.cpp) | SDL3を使い手軽で安全にWindowsとの連携を行っています |
| **【アーキテクチャ基盤】** | |
| [`common/object.h`](common/object.h)<br>[`common/component.h`](common/component.h) | Component志向を実践したクラスです。スマートポインタを用いた安全な参照管理と、各コンポーネントへの処理の委譲（Update/Draw）を実装しています。 |
| **【非同期処理・リソース管理】** | |
| [`common/model.cpp`](common/model.cpp) | Assimpを用いた3Dモデル読み込み処理です。`std::async`等を活用し、重いロード処理を非同期化する工夫を行っています。 |
| [`common/texture.cpp`](common/texture.cpp) | テクスチャの読み込み処理と管理を行うクラスです。stb_imageなどを用いてRaw展開にも対応しています |
| **【描画・グラフィックス】** | |
| [`common/renderer.cpp`](common/renderer.cpp) | DirectX 11の描画パイプライン全体を管理・隠蔽するクラスです。外部（ゲーム側）から扱いやすいAPI設計を心がけました。HDRやトゥーンなど様々なシェーダーに対応しています |
| [`data/SHADER/`](data/SHADER/) | 本プロジェクト用に記述したHLSLシェーダー群です。hlsliなどを使い分かりやすく、使いやすくすることを心掛けました |
| **【外部ライブラリの統合】** | |
| [`common/physics.cpp`](common/physics.cpp) | Bullet Physics（物理演算）の複雑なセットアップと更新処理をラップし、エンジン内で扱いやすいように統合しています。 |
| **【型/計算ライブラリの作成と依存性の分離】** | |
| [`common/math_types.h`](common/math_types.h)<br>[`common/mymath.h`](common/mymath.h) | Vector3型などを定義し、計算関数も調べながら実装、DirectXなどに依存せずに座標管理や計算を行えます。 |
| **【実装サンプル（ゲーム側での利用例）】** | |
| [`game/player.cpp`](game/player.cpp) | 上記のコアシステムを実際にどう呼び出し、コンポーネントをアタッチしてゲームオブジェクトを構築しているかが分かる実装サンプルです。 |

## 🛠 使用技術・主要ライブラリ
- **Graphics API**: DirectX 11 (DirectXTK, DirectXTex)
- **Window / Input**: SDL3
- **リソース管理・演算**:
  - Assimp (3Dモデル読み込み)
  - stb_image (テクスチャのRaw展開・画像読み込み)
  - FMOD (オーディオ制御)
  - Bullet Physics (物理演算)
- **データ形式・シリアライズ**: JsonCpp, yaml-cpp
- **UI・その他**: Dear ImGui, spdlog
※ ONNX Runtimeは導入していますが処理が完成していないため除外
