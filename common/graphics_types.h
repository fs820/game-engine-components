//--------------------------------------------
//
// 描画型定義 [graphics_types.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "math_types.h"

constexpr size_t MAX_BONES = 256; // 最大ボーン数
constexpr size_t MAX_LIGHT = 8;   // 最大ライト数

constexpr float WORLD_SIZE = 100.0f; // 1,0f = 1mの世界 (主にmodel変換など用)

// デフォルトのビューポートサイズ (基準解像度)
// UIはこの基準の位置と大きさにしてrendererから比率を取り出して拡縮する
const Vector2 DEFAULT_SCREEN_SIZE = { 1920.0f, 1080.0f };

// レンダリングキュー
enum class RenderQueueMask
{
    None,
    Shadow = 1 << 0,
    Geometry = 1 << 1,
    Decal = 1 << 2,
    Sky = 1 << 3,
    Outline = 1 << 4,
    Transparent = 1 << 5,
    UI = 1 << 6,
    String = 1 << 7
};

// 頂点シェーダーの種類
enum class VertexShaderType : unsigned char
{
    Vertex2D,           // 2Dスプライト用
    Vertex3D,           // 3Dモデル用
    VertexModel,        // スキニングモデル用
    Max
};

// ピクセルシェーダーの種類
enum class PixelShaderType : unsigned char
{
    Unlit,      // 光なし
    BlinnPhong, // Blinn Phong
    Toon,       // Toon
    Max
};

// ポストプロセスシェーダーのマスク
enum class PostProcessShaderMask
{
    None,          // なし
    FXAA = 1 << 0, // FXAA
    Gray = 1 << 1, // グレースケール
    Bloom = 1 << 2 // Bloom
};

// トーンマッピングの種類 (画面の色合いが変わります)
enum class ToneMappingType : unsigned char
{
    Linear,   // トーンマッピングなし (そのままの色) (白飛び注意)
    Reinhard, // 落ち着いた色 (汎用的)
    ACEST,    // リアルより
    Anime,    // アニメ調
    Max
};

// ブレンド設定
enum class BlendMode : unsigned char
{
    None,
    Default,
    Add,
    Subtract,
    Opaque,
    Decal,
    Max
};

// ラスタライザー設定
enum class RasMode : unsigned char
{
    None,
    Front,
    Back,
    Wireframe,
    Scissor,
    Max
};

// Zバッファ設定
enum class SampMode : unsigned char
{
    Wrap,
    Clamp,
    Border,
    Max
};

// Zバッファ設定
enum class DepthMode : unsigned char
{
    None,
    Default,
    TestOnly,
    StencilWrite,
    StencilMask,
    Max
};

// テクスチャデータの種類
enum class TextureType : unsigned char
{
    None,
    Raw,
    Wic,
    Dds,
    Tga
};

// テクスチャハンドル構造体
struct TextureHandle
{
    uint32_t id; // 無効

    bool isValid() const { return id != ~0u; }

    TextureHandle() : id{ ~0u } {}
    explicit TextureHandle(uint32_t handleId) : id{ handleId } {}
    ~TextureHandle() = default;
};

// メッシュハンドル構造体
struct MeshHandle
{
    uint32_t id; // 無効

    bool isValid() const { return id != ~0u; }

    MeshHandle() : id{ ~0u } {}
    explicit MeshHandle(uint32_t handleId) : id{ handleId } {}
    ~MeshHandle() = default;
};

// モデルハンドル構造体
struct ModelHandle
{
    uint32_t id; // 無効

    bool isValid() const { return id != ~0u; }

    ModelHandle() : id{ ~0u } {}
    explicit ModelHandle(uint32_t handleId) : id{ handleId } {}
    ~ModelHandle() = default;
};

// ライト
struct LightData
{
    Vector4 position;  // ライトの位置
    Color color;       // ライトの色
    Vector4 direction; // ライトの方向
    LightData() : position{}, color{ 1,1,1,1 }, direction{ 0,-1,0,0 } {}
    LightData(Vector4 position, Vector4 direction, Color color) : position{ position }, direction{ direction }, color{ color } {}
    ~LightData() = default;
};

// マテリアル
struct Material
{
    Color Diffuse;                    // 物体の基本色 (例: 赤いボールなら赤)
    Color Specular;                   // 光沢の強さと色 (例: 白いハイライトなら白)
    Color Emissive;                   // 自己発光色 (例: 暗闇で光る目)
    float Power;                      // ハイライトの鋭さ (数値が大きいほど鋭い)
    float AlphaCutoff;                // アルファカットオフ値
    PixelShaderType  pixelShaderType; // ライトの有効/無効

    void reset()
    {
        Diffuse = Color::White();
        Specular = Color::Black();
        Emissive = Color::Black();
        Power = 32.0f;
        AlphaCutoff = 0.01f;
        pixelShaderType = PixelShaderType::Unlit;
    }

    void defaults()
    {
        Diffuse = Color::White();
        Specular = Color::Black();
        Emissive = Color::Black();
        Power = 32.0f;
        AlphaCutoff = 0.01f;
        pixelShaderType = PixelShaderType::BlinnPhong;
    }

    Material() : Diffuse{ Color::White() }, Specular{ Color::Black() }, Emissive{ Color::Black() }, Power{ 32.0f }, AlphaCutoff{ 0.01f }, pixelShaderType{ PixelShaderType::Unlit } {}
    Material(const Color& diffuse, const Color& specular, const Color& emissive, float power, float alphaCutoff, PixelShaderType pixelShaderType)
        : Diffuse{ diffuse }, Specular{ specular }, Emissive{ emissive }, Power{ power }, AlphaCutoff{ alphaCutoff }, pixelShaderType{ pixelShaderType } {}
    ~Material() = default;
};

// Fog
struct FogData
{
    Color color;         //色 
    float start;         // 開始距離
    float end;           // 終了距離 (完全にFogになる距離)
    float skyFogHeight;  // Fogの高さ (これ以上はFogがかからない)
    float horizonHeight; // 地平線の高さ
    float fogPower;      // Fogのかかり方の係数
    float skyFogPower;   // SkyFogのかかり方の係数

    FogData() : color{ 1,1,1,1 }, start{}, end{}, skyFogHeight{}, horizonHeight{}, fogPower{}, skyFogPower{} {}
    FogData(const Color& color, float start, float end, float skyFogHeight, float horizonHeight, float fogPower, float skyFogPower) : color{ color }, start{ start }, end{ end }, skyFogHeight{ skyFogHeight }, horizonHeight{ horizonHeight }, fogPower{ fogPower }, skyFogPower{ skyFogPower } {}
    ~FogData() = default;
};

// Outline
struct OutlineData
{
    Color color;     // 色
    float Thickness; // 太さ

    OutlineData() : color{ Color::Black() }, Thickness{ 0.01f } {}
    OutlineData(const Color& color, float Thickness) : color{ color }, Thickness{ Thickness } {}
    ~OutlineData() = default;
};

// 頂点情報の構造体
struct Vertex2D
{
    Vector4 pos; // 座標+RHW
    Color col;   // カラー
    Vector2 uv;  // テクスチャ座標

    Vertex2D() : pos{}, col{}, uv{} {}
    Vertex2D(float x, float y, float z, float w, Color col, float u, float v) : pos{ x, y, z, w }, col{ col }, uv{ u, v } {}
    Vertex2D(float x, float y, Color col, float u, float v) : pos{ x, y, 0.0f, 1.0f }, col{ col }, uv{ u, v } {}
    Vertex2D(Vector2 position, Color col, Vector2 uv) : pos{ position.x, position.y, 0.0f, 1.0f }, col{ col }, uv{ uv } {}
    Vertex2D(Vector2 position, Vector2 uv) : pos{ position.x, position.y, 0.0f, 1.0f }, col{ Color::White() }, uv{ uv } {}
    ~Vertex2D() = default;
};

// 頂点情報の構造体
struct Vertex3D
{
    Vector3 pos; // 座標
    Vector3 nor; // 法線ベクトル
    Color col;   // カラー
    Vector2 uv;  // テクスチャ座標

    Vertex3D() : pos{}, nor{}, col{}, uv{} {}
    Vertex3D(float x, float y, float z, float nx, float ny, float nz, Color col, float u, float v) : pos{ x, y, z }, nor{ nx, ny, nz }, col{ col }, uv{ u, v } {}
    Vertex3D(Vector3 position, Vector3 normal, Color col, Vector2 uv) : pos{ position }, nor{ normal }, col{ col }, uv{ uv } {}
    Vertex3D(Vector3 position, Vector3 normal, Vector2 uv) : pos{ position }, nor{ normal }, col{ Color::White() }, uv{ uv } {}
    ~Vertex3D() = default;
};

// 頂点情報の構造体
struct VertexModel
{
    Vector3 pos; // 座標
    Vector3 nor; // 法線ベクトル
    Color col;   // カラー
    Vector2 uv;  // テクスチャ座標
    float weights[4];       // ボーンの重み (合計1.0)
    uint8_t boneIndices[4]; // ボーン番号 (0~255)

    VertexModel() : pos{}, nor{}, col{}, uv{}, weights{}, boneIndices{} {}
    VertexModel(float x, float y, float z, float nx, float ny, float nz, Color col, float u, float v, const float* weights, const uint8_t* boneIndices) : pos{ x, y, z }, nor{ nx, ny, nz }, col{ col }, uv{ u, v }
    {
        std::copy(weights, weights + 4, this->weights);
        std::copy(boneIndices, boneIndices + 4, this->boneIndices);
    }
    ~VertexModel() = default;
};

inline RenderQueueMask operator|(RenderQueueMask lhs, RenderQueueMask rhs)
{
    return static_cast<RenderQueueMask>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
        );
}

inline RenderQueueMask& operator|=(RenderQueueMask& lhs, RenderQueueMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline RenderQueueMask operator&(RenderQueueMask lhs, RenderQueueMask  rhs)
{
    return static_cast<RenderQueueMask>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
        );
}

inline RenderQueueMask& operator&=(RenderQueueMask& lhs, RenderQueueMask rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline RenderQueueMask operator^(RenderQueueMask lhs, RenderQueueMask rhs)
{
    return static_cast<RenderQueueMask>(
        static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs)
        );
}

inline RenderQueueMask& operator^=(RenderQueueMask& lhs, RenderQueueMask rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

inline RenderQueueMask operator~(RenderQueueMask mask)
{
    return static_cast<RenderQueueMask>(
        ~static_cast<uint32_t>(mask)
        );
}

inline bool HasFlag(RenderQueueMask mask, RenderQueueMask flag)
{
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

inline bool Any(RenderQueueMask mask)
{
    return static_cast<uint32_t>(mask) != 0;
}

inline PostProcessShaderMask operator|(PostProcessShaderMask lhs, PostProcessShaderMask rhs)
{
    return static_cast<PostProcessShaderMask>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
        );
}

inline PostProcessShaderMask& operator|=(PostProcessShaderMask& lhs, PostProcessShaderMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline PostProcessShaderMask operator&(PostProcessShaderMask lhs, PostProcessShaderMask  rhs)
{
    return static_cast<PostProcessShaderMask>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
        );
}

inline PostProcessShaderMask& operator&=(PostProcessShaderMask& lhs, PostProcessShaderMask rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline PostProcessShaderMask operator^(PostProcessShaderMask lhs, PostProcessShaderMask rhs)
{
    return static_cast<PostProcessShaderMask>(
        static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs)
        );
}

inline PostProcessShaderMask& operator^=(PostProcessShaderMask& lhs, PostProcessShaderMask rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

inline PostProcessShaderMask operator~(PostProcessShaderMask mask)
{
    return static_cast<PostProcessShaderMask>(
        ~static_cast<uint32_t>(mask)
        );
}

inline bool HasFlag(PostProcessShaderMask mask, PostProcessShaderMask flag)
{
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

inline bool Any(PostProcessShaderMask mask)
{
    return static_cast<uint32_t>(mask) != 0;
}
