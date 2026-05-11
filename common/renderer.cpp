//--------------------------------------------
//
// レンダラー定義 [renderer.cpp]
// Author: Fuma Sato
//
//--------------------------------------------

#include <d3dcompiler.h> // シェーダーコンパイル用
#include <DirectXMath.h> // 本来は数学用だがこのrendererではTextでの受け渡しにのみ使用
#include <DirectXTex.h>  // テクスチャ用

#include <DirectXTK/SpriteBatch.h> // Text用
#include <DirectXTK/SpriteFont.h>  //

#pragma comment(lib, "d3dcompiler.lib")

// ComPtrを使います
using Microsoft::WRL::ComPtr;

namespace
{
    template<size_t N, typename T>
    std::array<T*, N> MakeRawArray(const std::array<ComPtr<T>, N>& src)
    {
        std::array<T*, N> dst{};
        for (size_t i = 0; i < N; ++i)
            dst[i] = src[i].Get();
        return dst;
    }
}

#include "renderer.h"
#include "texture.h"
#include "mymath.h"
#include "scene.h"
#include "camera.h"
#include "render.h"
#include "camera_comp.h"
#include "light_comp.h"

static constexpr wchar_t SHADER_DIRECTORY[] = L"data/SHADER";

//------------------------
// レンダーパス
//------------------------
enum class RenderPass
{
    Shadow,   // シャドウマップ生成パス
    Geometry, // ジオメトリパス (Gバッファ生成)
    Decal,    // デカールパス
    Forward,  // フォワードパス
    UI,       // UIパス
    Max
};

//------------------------
// フォワードサブパス
//------------------------
enum class ForwardSubPass
{
    Sky,         // スカイ
    Outline,     // アウトライン
    Transparent, // トランスペアレント
    Max
};

//------------------------
// ポストプロセスシェーダーの種類
//------------------------
enum class PostProcessShaderType : unsigned char
{
    None,         // コピーシェーダ
    FXAA,         // FXAA
    Gray,         // グレースケール
    BloomExtract, // BloomExtract
    GaussianBlur, // ブラー
    Compossite,   // ブルームを合成し,色調補正とHDR->LDRを行う
    Max
};

//------------------------
// 定数バッファ用構造体
//------------------------

// ビュー・プロジェクション行列
struct VPMatBufferData
{
    Matrix View;
    Matrix Proj;

    VPMatBufferData() : View{}, Proj{} {}
    ~VPMatBufferData() = default;
};

//ライト
struct LightBufferData
{
    Color GlobalAmbient;          // 環境光 (全体で1つ)
    Vector4 CameraPos;            // カメラ位置
    int     LightCount;           // 現在有効なライトの数
    float   padding[3];           // 16バイト境界合わせ用パディング
    LightData Lights[MAX_LIGHT];  // ライトの配列

    LightBufferData() : GlobalAmbient{}, CameraPos{}, LightCount{}, padding{}, Lights{} {}
    ~LightBufferData() = default;
};

// ワールド行列
struct WorldMatBufferData
{
    Matrix World;

    WorldMatBufferData() : World{} {}
    ~WorldMatBufferData() = default;
};

// シャドウ用
struct ShadowBufferData
{
    Matrix LightViewProj; // ライトの View * Proj 行列

    ShadowBufferData() : LightViewProj{} {}
    ~ShadowBufferData() = default;
};

// シャドウ用
struct FogBufferData
{
    Color color;
    float start;
    float end;
    float skyFogHeight;  // Fogの高さ (これ以上はFogがかからない)
    float horizonHeight; // 地平線の高さ
    float fogPower;      // Fogのかかり方の係数
    float skyFogPower;   // SkyFogのかかり方の係数
    float pad[2];

    FogBufferData() : color{}, start{}, end{}, skyFogHeight{}, horizonHeight{}, fogPower{}, skyFogPower{}, pad{} {}
    ~FogBufferData() = default;
};

// マテリアル
struct MaterialBufferData
{
    Color Diffuse;   // (R, G, B, A)
    Color Specular;  // (R, G, B, A)
    Color Emissive;  // (R, G, B, A)

    float Power;           // ハイライトの鋭さ
    float AlphaCutoff;     // アルファテストのしきい値
    int PixelShaderType;   // ライトの有効/無効
    float padding;         // 16バイト境界合わせ用

    MaterialBufferData() : Diffuse{}, Specular{}, Emissive{}, Power{}, PixelShaderType{}, AlphaCutoff{}, padding{} {}
    ~MaterialBufferData() = default;
};

// ボーン
struct BoneBufferData
{
    Matrix BoneTransforms[MAX_BONES];

    BoneBufferData() : BoneTransforms{} {}
    ~BoneBufferData() = default;
};

// ボーン
struct OutlineBufferData
{
    Color OutlineColor;
    float OutlineWidth;
    float padding[3];

    OutlineBufferData() : OutlineColor{}, OutlineWidth{}, padding{} {}
    ~OutlineBufferData() = default;
};

// デカール
struct DecalBufferData
{
    Matrix InverseWorld;
    Color DecalColor;

    DecalBufferData() : InverseWorld{}, DecalColor{} {}
    ~DecalBufferData() = default;
};

// ポストプロセス
struct PostProcessBufferData
{
    Vector4 ScreenSize;   // x:幅, y:高さ, z:1/幅, w:1/高さ
    Vector2 BlurDir;      // x,y: ぼかす方向 (1,0) or (0,1)
    float bloomThreshold; // ブルームで抽出する明るさの閾値
    float bloomIntensity; // ブルーム強度
    int toneMappingType;  // トーンマッピングの種類
    float padding[3];

    PostProcessBufferData() : ScreenSize{}, BlurDir{}, bloomThreshold{}, bloomIntensity{}, toneMappingType{}, padding{} {}
    ~PostProcessBufferData() = default;
};

// メッシュ
struct MeshData
{
    VertexShaderType vertexhaderType; // 頂点シェーダーの種類
    ComPtr<ID3D11Buffer> pVertex;     // 頂点バッファ
    ComPtr<ID3D11Buffer> pIndex;      // インデックスバッファ
    unsigned int stride;              // 頂点サイズ
    size_t indicesCount;              // インデックスカウント

    MeshData() : vertexhaderType{}, pVertex{}, pIndex{}, stride{}, indicesCount{} {}
    ~MeshData() = default;
};

static constexpr std::array<const wchar_t*, size_t(PostProcessShaderType::Max)> POST_PROCESS_SHADER_FILE_NAMES =
{
    L"PP_None.hlsl",
    L"PP_Fxaa.hlsl",
    L"PP_Gray.hlsl",
    L"PP_BloomExtract.hlsl",
    L"PP_GaussianBlur.hlsl",
    L"PP_Composite.hlsl"
};

//----------------------------
// レンダラー
//----------------------------
class RendererImpl
{
public:
    // MRT用の定数
    static constexpr int GBUFFER_COUNT = 4;                   // ColorD, Normal, Position, ColorE
    static constexpr int DEFAULT_SHADOWMAP_RESOLUTION = 8192; // シャドウマップの基本解像度 2048,4096,8192

    RendererImpl();
    ~RendererImpl();

    void init(HWND handle, long width, long height);
    void uninit();
    bool render(const Scene& scene, std::function<void()> guiRender, Renderer& inter);
    void beginShadow(Matrix lightView, Matrix lightProj);
    void endShadow();
    void beginGeometry(Matrix cameraView, Matrix cameraProj);
    void endGeometry();
    void beginDecal(Matrix cameraView, Matrix cameraProj);
    void endDecal();
    void beginForward(Matrix cameraView, Matrix cameraProj);
    void endForward();
    void beginUI();
    void endUI();
    void present();

    void clearRenderTarget(ID3D11RenderTargetView* rtv, const Color& color = Color::Black());
    void clearDepthStencil(ID3D11DepthStencilView* dsv, float depth = 1.0f, UINT8 stencil = 0u);

    bool uploadTextures(const TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback = {});

    MeshHandle createMesh(VertexShaderType type, const void* vertices, size_t verticesCount, const void* indices, size_t indicesCount);
    bool setMesh(const MeshHandle& handle);

    bool setTexture(const TextureHandle& handle);
    bool setTransformWorld(const Matrix& matrix);
    bool setCameraPosition(const Vector3& cameraPos);
    bool setMaterial(const Material& material);
    bool setLight(std::span<const LightData> lights, const Color& ambient);
    bool setFog(const FogData& fog);
    bool drawMesh(const MeshHandle& handle);
    bool drawIndexedPrimitive(VertexShaderType vertexShaderType, unsigned int indexCount, unsigned int startIndexLocation, unsigned int baseVertexLocation);
    bool setBoneTransforms(std::span<const Matrix> boneTransforms);
    void drawDecal(Matrix transform, const MeshHandle& handle, Color color);
    void drawLightingPass();
    void drawPostProcessPass(PostProcessShaderMask mask, ToneMappingType type);
    void drawString(std::string_view string, Vector2 pos = { 0,0 }, Color color = Color::White(), float angle = 0.0f, Vector2 scale = { 1,1 });

    void setShadowMode();
    void setGeometryMode();
    void setDecalMode();
    void setForwardMode();
    void setUIMode();
    void setOutlineMode();
    void setSkyMode();
    void setBlendMode(BlendMode blendMode);
    void setRasMode(RasMode rasMode);
    void setDepthMode(DepthMode depthMode);
    void setSampMode(SampMode sampMode);
    void setOutlineData(OutlineData data);
    void setScissorRect(int left, int top, int right, int bottom);
    void setPass(RenderPass pass) { m_currentPass = pass; }
    void setForwardPass(ForwardSubPass subPass) { m_currentForwardSubPass = subPass; }

    void setPostProcessShaderMask(PostProcessShaderMask mask) { m_postProcessMask = mask; }
    void setToneMappingType(ToneMappingType type) { m_toneMappingType = type; }
    void setShadowMapResolution(int resolution) { m_shadowMapResolution = resolution; }
    void setShadowMapArea(float width, float height, float near, float fur) { m_shadowMapProj = Matrix::Orthographic(width, height, near, fur); }
    void setAmbient(const Color& ambient) { m_ambient = ambient; }

    void onResize(int width, int height);
    void getViewportSize(Vector2& size) const { size = m_viewportSize; }
    void getScreenSizeMagnification(Vector2& magnification) const { magnification = m_screenMagnification; }

    ID3D11Device* getDevice() const;
    ID3D11DeviceContext* getContext() const;
    HWND getRegisteredHWND() const;

private:
    void setupDevice(HWND handle);
    void setupShader();
    void setupDummy();
    void setupState();
    void setupSceneBuffer();
    void setupGBuffer();
    void setupShadowMap();
    void setupFont();
    void releaseSceneBuffer();
    void releaseGBuffer();
    void releaseShadowMap();
    void setLightingPassMode();
    void setPostProcessMode();
    void setVPMatrix(Matrix view, Matrix proj);
    void setOrthographic();
    void createTexture(std::shared_ptr<TextureData> spTextureData, uint32_t id);

    // 核
    ComPtr<ID3D11Device> m_pDevice;         // デバイス
    ComPtr<ID3D11DeviceContext> m_pContext; // コンテキスト
    ComPtr<IDXGISwapChain> m_pSwapChain;    // スワップチェイン

    HWND m_hWnd; // アウトプット先のWindow

    // 最終描画先 (バックバッファ)
    ComPtr<ID3D11RenderTargetView> m_pRenderTargetView; // レンダーターゲット (バックバッファへの書き込み)
    ComPtr<ID3D11Texture2D> m_pDepthStencilTexture;     // Zバッファ
    ComPtr<ID3D11DepthStencilView> m_pDepthStencilView; // Zバッファ書き込み

    // scene描画先
    ComPtr<ID3D11Texture2D> m_pSceneTexture;      // シーン画像本体
    ComPtr<ID3D11RenderTargetView> m_pSceneRTV;   // 書き込み用
    ComPtr<ID3D11ShaderResourceView> m_pSceneSRV; // 読み込み用

    // scene加工用描画先
    ComPtr<ID3D11Texture2D> m_pWorkTexture;      // シーン画像本体
    ComPtr<ID3D11RenderTargetView> m_pWorkRTV;   // 書き込み用
    ComPtr<ID3D11ShaderResourceView> m_pWorkSRV; //　読み込み用

    // ブルーム用描画先
    std::array<ComPtr<ID3D11RenderTargetView>, 2> m_pBloomRTVs;
    std::array<ComPtr<ID3D11ShaderResourceView>, 2> m_pBloomSRVs;

    // G-Buffer描画先
    std::array<ComPtr<ID3D11Texture2D>, GBUFFER_COUNT> m_pGBufferTextures;      // 実体 (テクスチャ)
    std::array<ComPtr<ID3D11RenderTargetView>, GBUFFER_COUNT> m_pGBufferRTVs;   // 書き込み用
    std::array<ComPtr<ID3D11ShaderResourceView>, GBUFFER_COUNT> m_pGBufferSRVs; // 読み込み用

    // シャドウマップ描画先
    ComPtr<ID3D11Texture2D> m_pShadowTexture;      // 実体 (テクスチャ)
    ComPtr<ID3D11DepthStencilView> m_pShadowDSV;   // 書き込み用
    ComPtr<ID3D11ShaderResourceView> m_pShadowSRV; // 読み込み用

    // 影用シェーダ
    ComPtr<ID3D11PixelShader> m_pShadowPS;         // アルファテストして深度を返す

    // シェーダ (G-Buffer)
    ComPtr<ID3D11VertexShader> m_pVertexShader2D;    // 2D頂点シェーダ
    ComPtr<ID3D11VertexShader> m_pVertexShader3D;    // 3D頂点シェーダ
    ComPtr<ID3D11VertexShader> m_pVertexShaderModel; // スキニング頂点シェーダ
    ComPtr<ID3D11PixelShader> m_pGeometryPS;         // ピクセルシェーダ (G-Bufferに分割して送るためのシェーダ)

    // デカール用シェーダ
    ComPtr<ID3D11VertexShader> m_pDecalVS; // デカール頂点シェーダ
    ComPtr<ID3D11PixelShader> m_pDecalPS;  // デカールピクセルシェーダ

    // ライティング用
    ComPtr<ID3D11VertexShader> m_pScreenVS;             // 画面全体用VS
    ComPtr<ID3D11PixelShader> m_pUnifiedLighting_DL_PS; // ライティング用PS

    // Forward用
    ComPtr<ID3D11VertexShader> m_pOutline3DVS;    // アウトライン3D頂点シェーダ
    ComPtr<ID3D11VertexShader> m_pOutlineModelVS; // アウトラインModel頂点シェーダ
    ComPtr<ID3D11PixelShader> m_pSkyPS;           // Skyピクセルシェーダ
    ComPtr<ID3D11PixelShader> m_pOutlinePS;       // アウトラインピクセルシェーダ
    ComPtr<ID3D11PixelShader> m_pTransparentPS;   // 半透明シェーダ

    // UI用
    ComPtr<ID3D11PixelShader> m_pUIPS;   // UIシェーダ

    // ポストプロセス用シェーダ
    std::array<ComPtr<ID3D11PixelShader>, size_t(PostProcessShaderType::Max)> m_pPostProcessShaders;

    // レイアウト
    ComPtr<ID3D11InputLayout> m_pInputLayout2D;    // 2D頂点レイアウト
    ComPtr<ID3D11InputLayout> m_pInputLayout3D;    // 3D頂点レイアウト
    ComPtr<ID3D11InputLayout> m_pInputLayoutModel; // スキニング頂点レイアウト

    // 定数バッファ
    ComPtr<ID3D11Buffer> m_pVPMatBuffer;          // 行列のバッファ
    VPMatBufferData m_vpMatData;                  // 行列のキャッシュ
    ComPtr<ID3D11Buffer> m_pLightBuffer;          // ライトのバッファ
    LightBufferData m_lightData;                  // ライトのキャッシュ
    ComPtr<ID3D11Buffer> m_pWMatBuffer;           // 行列のバッファ
    WorldMatBufferData m_wMatData;                // 行列のキャッシュ
    ComPtr<ID3D11Buffer> m_pMtlBuffer;            // マテリアルのバッファ
    MaterialBufferData m_mtlData;                 // マテリアルのキャッシュ
    ComPtr<ID3D11Buffer> m_pShadowConstantBuffer; // シャドウのバッファ
    Matrix m_lightVPMatrix;                       // ライトの View * Proj
    ComPtr<ID3D11Buffer> m_pBoneBuffer;           // ボーン行列のバッファ
    BoneBufferData m_boneData;                    // ボーン行列のキャッシュ
    ComPtr<ID3D11Buffer> m_pOutlineBuffer;        // アウトラインのバッファ
    OutlineBufferData m_outlineData;              // アウトラインのキャッシュ
    ComPtr<ID3D11Buffer> m_pFogBuffer;            // フォグのバッファ
    FogBufferData m_fogData;                      // フォグのキャッシュ
    ComPtr<ID3D11Buffer> m_pDecalBuffer;          // デカールのバッファ
    ComPtr<ID3D11Buffer> m_pPostProcessBuffer;    // ポストプロセスのバッファ

    // State (設定)
    std::array<ComPtr<ID3D11SamplerState>, size_t(SampMode::Max)> m_samplerStates;     // サンプラー
    std::array<ComPtr<ID3D11BlendState>, size_t(BlendMode::Max)> m_blendStates;        // ブレンド
    std::array<ComPtr<ID3D11RasterizerState>, size_t(RasMode::Max)> m_rasStates;       // ラスタライザー
    std::array<ComPtr<ID3D11DepthStencilState>, size_t(DepthMode::Max)> m_depthStates; // Zバッファ

    // 現在の描画モード
    RenderPass m_currentPass;               // 現在のレンダーパス
    ForwardSubPass m_currentForwardSubPass; // 現在のフォワードサブパス

    int m_shadowMapResolution; // シャドウマップの解像度 2048,4096,8192 (上げるほど影がきれいですが処理が増えます) ※影の範囲には関係ありません
    Matrix m_shadowMapProj;    // シャドウマップ時のプロジェクション行列 (影を落とす範囲)

    Color m_ambient;                         // 環境光
    PostProcessShaderMask m_postProcessMask; // ポストプロセス
    ToneMappingType m_toneMappingType;       // 色調補正

    // 登録されたメッシュのキャッシュ
    std::vector<MeshData> m_meshs;

    // 登録されたテクスチャのキャッシュ
    std::unordered_map<uint32_t, ComPtr<ID3D11ShaderResourceView>> m_textures;
    std::mutex m_texMutex;

    // テクスチャなしの時に使うテクスチャ
    ComPtr<ID3D11ShaderResourceView> m_pDummyTextureWhite; // 白
    ComPtr<ID3D11ShaderResourceView> m_pDummyTextureBlack; // 黒

    Vector2 m_screenSize;          // 画面サイズ
    Vector2 m_screenMagnification; // 画面サイズ倍率
    Vector2 m_viewportSize;        // ビューポートサイズ

    // スプライトバッチとフォント
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> m_spriteFont;
};

RendererImpl::RendererImpl() : m_pDevice(nullptr), m_pContext(nullptr), m_pSwapChain(nullptr), m_hWnd{}, m_pRenderTargetView(nullptr), m_pDepthStencilView(nullptr), m_pDepthStencilTexture(nullptr), m_pSceneTexture{}, m_pSceneRTV{}, m_pSceneSRV{}, m_pVertexShader2D(nullptr), m_pVertexShader3D(nullptr), m_pGeometryPS(nullptr), m_pInputLayout2D(nullptr), m_pInputLayout3D(nullptr), m_pWMatBuffer(nullptr), m_wMatData{}, m_pMtlBuffer(nullptr), m_mtlData{}, m_pVPMatBuffer(nullptr), m_vpMatData{}, m_pLightBuffer(nullptr), m_lightData{}, m_samplerStates{}, m_pDummyTextureWhite(nullptr), m_pDummyTextureBlack(nullptr), m_pInputLayoutModel(nullptr), m_pBoneBuffer(nullptr), m_boneData{}, m_pVertexShaderModel(nullptr), m_pGBufferTextures{}, m_pGBufferRTVs{}, m_pGBufferSRVs{}, m_pScreenVS{}, m_blendStates{}, m_depthStates{}, m_rasStates{}, m_textures{}, m_screenSize{}, m_screenMagnification{}, m_viewportSize{}, m_pShadowTexture{}, m_pShadowDSV{}, m_pShadowSRV{}, m_currentPass{}, m_currentForwardSubPass{}, m_pShadowConstantBuffer{}, m_lightVPMatrix{}, m_pSkyPS{}, m_pTransparentPS{}, m_pOutline3DVS{}, m_pOutlineModelVS{}, m_pOutlinePS{}, m_pOutlineBuffer{}, m_outlineData{}, m_pShadowPS{}, m_pFogBuffer{}, m_texMutex{}, m_spriteBatch{}, m_spriteFont{}, m_pDecalBuffer(nullptr), m_pDecalVS(nullptr), m_pDecalPS(nullptr), m_pPostProcessShaders{}, m_pPostProcessBuffer{}, m_pWorkTexture{}, m_pWorkRTV{}, m_pWorkSRV{}, m_pBloomRTVs{}, m_pBloomSRVs{}, m_meshs{}, m_pUnifiedLighting_DL_PS{}, m_pUIPS{}, m_postProcessMask{}, m_toneMappingType{}, m_ambient{}, m_shadowMapResolution{ DEFAULT_SHADOWMAP_RESOLUTION }, m_shadowMapProj{} {}
RendererImpl::~RendererImpl() { uninit(); }

//-------------------------------------------
// 初期化
//-------------------------------------------
void RendererImpl::init(HWND handle, long width, long height)
{
    if (handle == nullptr || width <= 0 || height <= 0)return;

    // サイズと倍率を保存する
    m_screenSize = Vector2(static_cast<float>(width), static_cast<float>(height));
    m_screenMagnification = Vector2(static_cast<float>(width) / DEFAULT_SCREEN_SIZE.x, static_cast<float>(height) / DEFAULT_SCREEN_SIZE.y);

    // デバイスなどの核を生成する
    setupDevice(handle);

    // シェーダーを生成する
    setupShader();

    // シーン描画先を生成する
    setupSceneBuffer();

    // MRTを生成する
    setupGBuffer();

    // シャドウマップを生成する
    setupShadowMap();

    // ダミーテクスチャを生成する
    setupDummy();

    // フォントを生成する
    setupFont();

    // 描画設定
    setupState();
}

//-------------------------------------------
// 破棄
//-------------------------------------------
void RendererImpl::uninit()
{
    // ダミーテクスチャ破棄
    m_pDummyTextureBlack.Reset();
    m_pDummyTextureWhite.Reset();

    // 登録テクスチャ破棄
    m_textures.clear();

    // メッシュ破棄
    m_meshs.clear();

    // State破棄
    m_samplerStates.fill(nullptr);
    m_depthStates.fill(nullptr);
    m_rasStates.fill(nullptr);
    m_blendStates.fill(nullptr);

    // シャドウマップ破棄
    releaseShadowMap();

    // MRT破棄
    releaseGBuffer();

    // シーン描画先破棄
    releaseSceneBuffer();

    // 定数バッファ破棄
    m_pPostProcessBuffer.Reset();
    m_pDecalBuffer.Reset();
    m_pFogBuffer.Reset();
    m_pOutlineBuffer.Reset();
    m_pBoneBuffer.Reset();
    m_pMtlBuffer.Reset();
    m_pShadowConstantBuffer.Reset();
    m_pWMatBuffer.Reset();
    m_pLightBuffer.Reset();
    m_pVPMatBuffer.Reset();

    // 入力レイアウト破棄
    m_pInputLayout2D.Reset();
    m_pInputLayout3D.Reset();
    m_pInputLayoutModel.Reset();

    // シェーダー破棄
    for (auto& pPostProcessShader : m_pPostProcessShaders)
    {
        pPostProcessShader.Reset();
    }
    m_pUIPS.Reset();
    m_pOutlinePS.Reset();
    m_pOutlineModelVS.Reset();
    m_pOutline3DVS.Reset();
    m_pTransparentPS.Reset();
    m_pSkyPS.Reset();
    m_pUnifiedLighting_DL_PS.Reset();
    m_pScreenVS.Reset();
    m_pDecalPS.Reset();
    m_pDecalVS.Reset();
    m_pShadowPS.Reset();
    m_pGeometryPS.Reset();
    m_pVertexShaderModel.Reset();
    m_pVertexShader2D.Reset();
    m_pVertexShader3D.Reset();

    // 深度ステンシルビュー破棄
    m_pDepthStencilView.Reset();

    // 深度ステンシルテクスチャ破棄
    m_pDepthStencilTexture.Reset();

    // レンダーターゲットビュー破棄
    m_pRenderTargetView.Reset();

    // スワップチェーン破棄
    m_pSwapChain.Reset();

    // コンテキスト破棄
    m_pContext.Reset();

    // デバイス破棄
    m_pDevice.Reset();
}

//-------------------------------------------
// 描画
//-------------------------------------------
bool RendererImpl::render(const Scene& scene, std::function<void()> guiRender, Renderer& inter)
{
    auto cameras = scene.getGameObjectsOfType<CameraComponent>();
    auto lights = scene.getGameObjectsOfType<LightComponent>();
    auto renderComponents = scene.getGameObjectsOfType<RenderComponent>();

    // ライトを設定
    std::vector<LightData> lightDatas{};
    for (const auto& light : lights)
        lightDatas.push_back(light->get());
    setLight(lightDatas, m_ambient);

    for (size_t cnt = 0; cnt < cameras.size(); cnt++)
    {
        auto& camera = cameras[cnt]->get();
        camera.Set();

        // レンダラーにカメラの位置を渡す(スペキュラー用)
        setCameraPosition(camera.GetPosition());

        // Cameraの行列
        Matrix CameraView = camera.GetViewMatrix(), CameraProj = camera.GetProjectionMatrix();

        //-------------------------
        // シャドウ開始
        //-------------------------

        // ShadowInfoがtrueのものの先頭が適応される
        for (const auto& light : lights)
        {
            float radius{}, theta{}, phi{};
            Vector3 target{}, up{};
            if (light->getShadowInfo(&radius, &theta, &phi, &target, &up))
            {
                // 視点を算出
                Vector3 eye = Vector3::FromSpherical(radius, theta, phi);
                if (std::abs(Vector3::Dot(eye, up)) > 0.99f)
                    up = { 0,0,1 };

                beginShadow(Matrix::LookAtLH(eye, target, up), m_shadowMapProj); // シャドウマップ範囲

                // 影を落とすオブジェクトを描画
                for (auto& renderComponent : renderComponents)
                {
                    if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Shadow))
                        renderComponent->render(inter);
                }

                endShadow();
                break;
            }
        }

        //-------------------------
        // シャドウ終了
        //-------------------------

        //------------------
        // ジオメトリ開始
        //------------------
        beginGeometry(CameraView, CameraProj);

        for (auto& renderComponent : renderComponents)
        {
            if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Geometry))
            {
                setRasMode(renderComponent->getRasMode());
                renderComponent->render(inter);
            }
        }

        endGeometry();
        //-------------------------
        // ジオメトリ終了
        //-------------------------

        //------------------
        // デカール開始
        //------------------
        beginDecal(CameraView, CameraProj);

        for (auto& renderComponent : renderComponents)
        {
            if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Decal))
                renderComponent->render(inter);
        }

        endDecal();
        //-------------------------
        // デカール終了
        //-------------------------

        //------------------
        // ライティング
        //------------------
        drawLightingPass();

        //----------
        // フォワード開始
        //----------
        beginForward(CameraView, CameraProj);

        // 空
        setSkyMode();

        for (auto& renderComponent : renderComponents)
        {
            if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Sky))
            {
                setRasMode(renderComponent->getRasMode());
                renderComponent->render(inter);
            }
        }

        setForwardMode();

        //-----------------------------------------------------------------------
        // アウトライン (フォワードの中でsetOutlineModeを呼ぶとアウトライン)
        //-----------------------------------------------------------------------
        setOutlineMode();

        for (auto& renderComponent : renderComponents)
        {
            if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Outline))
                renderComponent->render(inter);
        }

        setForwardMode();
        //-----------------------------------------------------------------------
        // アウトライン終了
        //-----------------------------------------------------------------------

        // 半透明オブジェクト
        for (auto& renderComponent : renderComponents)
        {
            if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::Transparent))
            {
                setRasMode(renderComponent->getRasMode());
                renderComponent->render(inter);
            }
        }

        endForward();
        //-------------------------
        // フォワード終了
        //-------------------------
    }

    //------------------
    // ポストプロセス
    //------------------
    drawPostProcessPass(m_postProcessMask, m_toneMappingType);

    //----------
    // UI開始
    //----------
    beginUI();

    for (auto& renderComponent : renderComponents)
    {
        if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::UI))
            renderComponent->render(inter);
    }

    endUI();
    //-------------------------
    // UI終了
    //-------------------------

    //----------
    // String開始
    //----------

    for (auto& renderComponent : renderComponents)
    {
        if (HasFlag(renderComponent->getRenderQueueMask(), RenderQueueMask::String))
            renderComponent->render(inter);
    }

    //-------------------------
    // String終了
    //-------------------------

    // Gui描画(あれば)
    if (guiRender != nullptr) guiRender();

    // 全描画を終了し切り替えを行う
    present();

    return true;
}

//-------------------------------------------
// 影描画開始
//-------------------------------------------
void RendererImpl::beginShadow(Matrix lightView, Matrix lightProj)
{
    // 行列のセット
    setVPMatrix(lightView, lightProj);

    // 影用デプスステンシルビューを設定 (RTVはなし)
    m_pContext->OMSetRenderTargets(0, nullptr, m_pShadowDSV.Get());

    // デプスクリア
    clearDepthStencil(m_pShadowDSV.Get());

    // シャドウマップ用ビューポート
    D3D11_VIEWPORT vp = {};
    vp.Width = float(m_shadowMapResolution);
    vp.Height = float(m_shadowMapResolution);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    // プリミティブトポロジーの設定 (三角形リスト)
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ライトのVP行列を保存
    m_lightVPMatrix = lightView * lightProj;

    // 影用モードに変更
    setShadowMode();
}

//-------------------------------------------
// 影描画終了
//-------------------------------------------
void RendererImpl::endShadow()
{
    // ターゲット解除
    m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
}

//-------------------------------------------
// ジオメトリ描画開始
//-------------------------------------------
void RendererImpl::beginGeometry(Matrix cameraView, Matrix cameraProj)
{
    // 行列のセット
    setVPMatrix(cameraView, cameraProj);

    // レンダーターゲットビューとデプスステンシルビューを設定
    m_pContext->OMSetRenderTargets(GBUFFER_COUNT, MakeRawArray(m_pGBufferRTVs).data(), m_pDepthStencilView.Get());

    // 画面クリア (Clear)
    for (auto& pRTV : m_pGBufferRTVs)
    {
        clearRenderTarget(pRTV.Get());
    }
    clearDepthStencil(m_pDepthStencilView.Get());

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = m_screenSize.x;
    vp.Height = m_screenSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    // プリミティブトポロジーの設定 (三角形リスト)
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Geometry描画モードにする
    setGeometryMode();
}

//-------------------------------------------
// ジオメトリ描画終了
//-------------------------------------------
void RendererImpl::endGeometry()
{
    // 書き込みターゲットを解除する
    ID3D11RenderTargetView* nullRTVs[GBUFFER_COUNT + 1] = { nullptr, nullptr, nullptr,nullptr,nullptr };
    m_pContext->OMSetRenderTargets(GBUFFER_COUNT + 1, nullRTVs, nullptr);
}

//-------------------------------------------
// デカール描画開始
//-------------------------------------------
void RendererImpl::beginDecal(Matrix cameraView, Matrix cameraProj)
{
    // 行列のセット
    setVPMatrix(cameraView, cameraProj);

    // Viewリセット
    ID3D11RenderTargetView* nullRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
    m_pContext->OMSetRenderTargets(4, nullRTVs, nullptr);

    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    m_pContext->PSSetShaderResources(0, 5, nullSRVs);

    // 書き込み設定
    // 色を上書きする
    ID3D11RenderTargetView* rtv = m_pGBufferRTVs[0].Get();
    m_pContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());

    // 読み込み(設定
    // 位置を参照する
    ID3D11ShaderResourceView* srv = m_pGBufferSRVs[2].Get();
    m_pContext->PSSetShaderResources(1, 1, &srv);

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = m_screenSize.x;
    vp.Height = m_screenSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    // プリミティブトポロジーの設定 (三角形リスト)
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Decal描画モードにする
    setDecalMode();
}

//-------------------------------------------
// デカール描画終了
//-------------------------------------------
void RendererImpl::endDecal()
{
    // リソース解除
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_pContext->PSSetShaderResources(1, 1, &nullSRV);
}

//-------------------------------------------
// フォワード描画開始
//-------------------------------------------
void RendererImpl::beginForward(Matrix cameraView, Matrix cameraProj)
{
    // 行列のセット
    setVPMatrix(cameraView, cameraProj);

    // レンダーターゲットビューとデプスステンシルビューを設定
    ID3D11RenderTargetView* rtv = m_pSceneRTV.Get();
    m_pContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = m_screenSize.x;
    vp.Height = m_screenSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    // プリミティブトポロジーの設定 (三角形リスト)
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Forward描画モードにする
    setForwardMode();
}

//-------------------------------------------
// フォワード描画終了
//-------------------------------------------
void RendererImpl::endForward()
{
    // ターゲット解除
    m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
}

//-------------------------------------------
// UI描画開始
//-------------------------------------------
void RendererImpl::beginUI()
{
    // 正射影行列のセット
    setOrthographic();

    // レンダーターゲットビューを設定
    ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
    m_pContext->OMSetRenderTargets(1, &rtv, nullptr);

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = m_screenSize.x;
    vp.Height = m_screenSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    // プリミティブトポロジーの設定 (三角形リスト)
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // UI描画モードにする
    setUIMode();
}

//-------------------------------------------
// UI描画終了
//-------------------------------------------
void RendererImpl::endUI()
{

}

//-------------------------------------------
// 画面切り替え (全描画終了)
//-------------------------------------------
void RendererImpl::present()
{
    // 切り替え
    m_pSwapChain->Present(1, 0); // 1: VSync有効
}

//-------------------------------------------
// レンダーターゲットをクリア
//-------------------------------------------
void RendererImpl::clearRenderTarget(ID3D11RenderTargetView* rtv, const Color& color)
{
    if (rtv != nullptr)
    {
        float clearColor[4] = { color.r, color.g, color.b, color.a }; // RGBA
        m_pContext->ClearRenderTargetView(rtv, clearColor);
    }
}

//-------------------------------------------
// レンダーターゲットをクリア
//-------------------------------------------
void RendererImpl::clearDepthStencil(ID3D11DepthStencilView* dsv, float depth, UINT8 stencil)
{
    if (dsv != nullptr)
    {
        m_pContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
    }
}

//-------------------------------------------
// テクスチャを読み込んで保存
//-------------------------------------------
bool RendererImpl::uploadTextures(const TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback)
{
    std::shared_ptr<TextureData> spTextureData{};

    std::vector<size_t> loadIndexList{};

    // 読み込むテクスチャのインデックスを収集
    size_t cnt{};
    while (true)
    {
        if (m_textures.contains(uint32_t(cnt))){ ++cnt; continue; } // すでに登録されている

        auto textureData = textureManager.getTextureData(cnt);
        if (!(spTextureData = textureData.lock())) break;

        if (spTextureData->isValid)
        {
            loadIndexList.push_back(cnt);
        }
        ++cnt;
    }

    std::vector<std::future<void>> futures{};
    std::atomic<int> finishedCount{ 0 }; // 完了した数
    std::atomic<int> activeThreads{ 0 };

    futures.reserve(loadIndexList.size());

    for (const auto& index : loadIndexList)
    {
        auto textureData = textureManager.getTextureData(index);
        if ((spTextureData = textureData.lock()))
        {
            if (spTextureData->isValid)
            {
                // 空きが出るまで待つ
                while (activeThreads.load() >= (int)maxThread)
                {
                    // コールバックがあれば呼び出す
                    if (progressCallback != nullptr)
                    {
                        if (!progressCallback("GPU Extract", finishedCount.load(), int(loadIndexList.size())))
                        {// 中止
                            for (auto& future : futures)
                            {
                                if (future.valid()) future.get();
                            }
                            return false;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                // 実行数
                ++activeThreads;

                // 読み込む
                futures.push_back(std::async(std::launch::async, [this, index, &activeThreads, &finishedCount, spTextureData]()
                    {
                        createTexture(spTextureData, uint32_t(index));
                        --activeThreads;
                        ++finishedCount;
                    }));
            }
        }
    }

    // 残りのタスクが終わるのを待つ
    while (finishedCount.load() < loadIndexList.size())
    {
        if (progressCallback != nullptr)
        {
            if (!progressCallback("GPU Extract", finishedCount.load(), int(loadIndexList.size())))
            {// 中止
                for (auto& future : futures)
                {
                    if (future.valid()) future.get();
                }
                return false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
    }

    // この関数はすべて読み込み終わってから終わります
    for (auto& future : futures)
    {
        if (future.valid()) future.get();
    }
    if (progressCallback != nullptr)
    {
        return progressCallback("GPU Extract", finishedCount.load(), int(loadIndexList.size()));
    }

    return true;
}

//-------------------------------------------
// メッシュデータを生成
//-------------------------------------------
MeshHandle RendererImpl::createMesh(VertexShaderType type, const void* vertices, size_t verticesCount, const void* indices, size_t indicesCount)
{
    MeshData mesh{};                   // メッシュ
    D3D11_BUFFER_DESC bd{};            // バッファ設定
    D3D11_SUBRESOURCE_DATA initData{}; // データ

    unsigned int stride{}; // 頂点のサイズ (ストライド)
    switch (type)
    {
    case VertexShaderType::Vertex2D:
        stride = sizeof(Vertex2D);
        break;
    case VertexShaderType::Vertex3D:
        stride = sizeof(Vertex3D);
        break;
    case VertexShaderType::VertexModel:
        stride = sizeof(VertexModel);
        break;
    }

    // 頂点バッファ
    bd.ByteWidth = static_cast<UINT>(stride * verticesCount);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    // 初期化データ
    initData.pSysMem = vertices;

    if (FAILED(m_pDevice->CreateBuffer(&bd, &initData, mesh.pVertex.GetAddressOf())))
    {
        return MeshHandle();
    }

    // インデックスバッファに切り替え
    bd.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indicesCount);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    // 初期化データ
    initData.pSysMem = indices;

    if (FAILED(m_pDevice->CreateBuffer(&bd, &initData, mesh.pIndex.GetAddressOf())))
    {
        return MeshHandle();
    }

    mesh.vertexhaderType = type;
    mesh.stride = stride;
    mesh.indicesCount = indicesCount;

    MeshHandle handle{};
    handle.id = uint32_t(m_meshs.size());
    m_meshs.push_back(mesh);
    return handle;
}

//-------------------------------------------
// メッシュデータを設定
//-------------------------------------------
bool RendererImpl::setMesh(const MeshHandle& handle)
{
    if (m_meshs.size() > handle.id)
    {
        const auto& mesh = m_meshs[handle.id];
        UINT offset = 0;
        const auto& pVertex = mesh.pVertex.Get();
        m_pContext->IASetVertexBuffers(0, 1, &pVertex, &mesh.stride, &offset);
        m_pContext->IASetIndexBuffer(mesh.pIndex.Get(), DXGI_FORMAT_R32_UINT, 0); // 32bit Index
        return true;
    }
    return false;
}

//-------------------------------------------
// テクスチャデータを設定
//-------------------------------------------
bool RendererImpl::setTexture(const TextureHandle& handle)
{
    ID3D11ShaderResourceView* pSRV = nullptr;
    if (handle.isValid())
    {// テクスチャが指定されている場合
        if (m_textures.find(handle.id) == m_textures.end())
        {// 未登録の場合は失敗
            pSRV = m_pDummyTextureWhite.Get();
            m_pContext->PSSetShaderResources(0, 1, &pSRV);
            return false;
        }
        pSRV = m_textures[handle.id].Get();; // 登録されたテクスチャを使用
    }
    else
    {// テクスチャが指定されていない場合
        pSRV = m_pDummyTextureWhite.Get(); // ダミーテクスチャを使用
    }
    m_pContext->PSSetShaderResources(0, 1, &pSRV);
    return true;
}

//-------------------------------------------
// ワールド行列を設定
//-------------------------------------------
bool RendererImpl::setTransformWorld(const Matrix& matrix)
{
    m_wMatData.World = matrix;
    return true;
}

//-------------------------------------------
// カメラ位置を設定
//-------------------------------------------
bool RendererImpl::setCameraPosition(const Vector3& cameraPos)
{
    m_lightData.CameraPos = Vector4(cameraPos, 1.0f);
    return true;
}

//-------------------------------------------
// マテリアルデータを設定
//-------------------------------------------
bool RendererImpl::setMaterial(const Material& material)
{
    m_mtlData.Diffuse = material.Diffuse;
    m_mtlData.Specular = material.Specular;
    m_mtlData.Emissive = material.Emissive;
    m_mtlData.Power = material.Power;
    m_mtlData.AlphaCutoff = material.AlphaCutoff;
    m_mtlData.PixelShaderType = int(material.pixelShaderType);
    return true;
}

//-------------------------------------------
// ライトを設定
//-------------------------------------------
bool RendererImpl::setLight(std::span<const LightData> lights, const Color& ambient)
{
    m_lightData.GlobalAmbient = ambient;
    m_lightData.LightCount = int(std::min(lights.size(), MAX_LIGHT));
    for (size_t cnt = 0; cnt < m_lightData.LightCount; cnt++)
    {
        m_lightData.Lights[cnt] = lights[cnt];
    }
    return true;
}

//-------------------------------------------
// フォグを設定
//-------------------------------------------
bool RendererImpl::setFog(const FogData& fog)
{
    m_fogData.color = fog.color;
    m_fogData.start = fog.start;
    m_fogData.end = fog.end;
    m_fogData.horizonHeight = fog.horizonHeight;
    m_fogData.skyFogHeight = fog.skyFogHeight;
    m_fogData.fogPower = fog.fogPower;
    m_fogData.skyFogPower = fog.skyFogPower;
    return true;
}

//-------------------------------------------
// ボーンを設定
//-------------------------------------------
bool RendererImpl::setBoneTransforms(std::span<const Matrix> boneTransforms)
{
    size_t count = boneTransforms.size();
    if (count > MAX_BONES)
    {
        count = MAX_BONES; // 最大ボーンまで
    }
    for (size_t i = 0; i < count; ++i)
    {
        m_boneData.BoneTransforms[i] = boneTransforms[i];
    }
    return true;
}

//-------------------------------------------
// アウトラインを設定
//-------------------------------------------
void RendererImpl::setOutlineData(OutlineData data)
{
    m_outlineData.OutlineColor = data.color;
    m_outlineData.OutlineWidth = data.Thickness;
}

//-------------------------------------------
// メッシュ描画
//-------------------------------------------
bool RendererImpl::drawMesh( const MeshHandle& handle)
{
    if (handle.isValid())
    {
        if (m_meshs.size() > handle.id)
        {
            setMesh(handle);

            // 描画
            drawIndexedPrimitive(m_meshs[handle.id].vertexhaderType, unsigned int(m_meshs[handle.id].indicesCount), 0, 0);
            return true;
        }
    }
    return false;
}

//-------------------------------------------
// 頂点描画
//-------------------------------------------
bool RendererImpl::drawIndexedPrimitive(VertexShaderType vertexShaderType, unsigned int indexCount, unsigned int startIndexLocation, unsigned int baseVertexLocation)
{
    ID3D11Buffer* buffer = nullptr; // 借用

    // 定数バッファ更新
    m_pContext->UpdateSubresource(m_pWMatBuffer.Get(), 0, nullptr, &m_wMatData, 0, 0);   // ワールド行列
    m_pContext->UpdateSubresource(m_pMtlBuffer.Get(), 0, nullptr, &m_mtlData, 0, 0);     // Material

    // 頂点シェーダーに送る
    buffer = m_pWMatBuffer.Get();
    m_pContext->VSSetConstantBuffers(1, 1, &buffer);  // スロット1にセット

    // ピクセルシェーダーに送る
    buffer = m_pMtlBuffer.Get();
    m_pContext->PSSetConstantBuffers(2, 1, &buffer);   // スロット2にセット

    // シェーダータイプごとの設定
    switch (vertexShaderType)
    {
    case VertexShaderType::Vertex2D:
        m_pContext->IASetInputLayout(m_pInputLayout2D.Get());         // 入力レイアウト設定
        m_pContext->VSSetShader(m_pVertexShader2D.Get(), nullptr, 0); // 頂点シェーダー設定
        break;
    case VertexShaderType::Vertex3D:
        m_pContext->IASetInputLayout(m_pInputLayout3D.Get());         // 入力レイアウト設定
        if (m_currentPass == RenderPass::Forward && m_currentForwardSubPass == ForwardSubPass::Outline)
        {// アウトライン描画
            m_pContext->UpdateSubresource(m_pOutlineBuffer.Get(), 0, nullptr, &m_outlineData, 0, 0);   // アウトライン
            buffer = m_pOutlineBuffer.Get();                                                           // 借りる
            m_pContext->VSSetConstantBuffers(3, 1, &buffer);                                           // スロット3にセット
            m_pContext->VSSetShader(m_pOutline3DVS.Get(), nullptr, 0);                                 // アウトライン3D頂点シェーダー設定
        }
        else
        {
            m_pContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0); // 頂点シェーダー設定
        }
        break;
    case VertexShaderType::VertexModel:
        m_pContext->IASetInputLayout(m_pInputLayoutModel.Get());                             // 入力レイアウト設定
        m_pContext->UpdateSubresource(m_pBoneBuffer.Get(), 0, nullptr, &m_boneData, 0, 0);   // ボーン
        buffer = m_pBoneBuffer.Get();                                                        // 借りる
        m_pContext->VSSetConstantBuffers(3, 1, &buffer);                                     // スロット3にセット
        if (m_currentPass == RenderPass::Forward && m_currentForwardSubPass == ForwardSubPass::Outline)
        {// アウトライン描画
            m_pContext->UpdateSubresource(m_pOutlineBuffer.Get(), 0, nullptr, &m_outlineData, 0, 0);   // アウトライン
            buffer = m_pOutlineBuffer.Get();                                                           // 借りる
            m_pContext->VSSetConstantBuffers(4, 1, &buffer);                                           // スロット4にセット
            m_pContext->VSSetShader(m_pOutlineModelVS.Get(), nullptr, 0);                              // アウトラインModel頂点シェーダー設定
        }
        else
        {
            m_pContext->VSSetShader(m_pVertexShaderModel.Get(), nullptr, 0);                     // Model頂点シェーダー設定
        }
        break;
    }

    // ピクセルシェーダー設定
    switch (m_currentPass)
    {
        // 影描画
    case RenderPass::Shadow:
        m_pContext->PSSetShader(m_pShadowPS.Get(), nullptr, 0);        // シャドウ用ピクセルシェーダ
        break;
        // ジオメトリ描画
    case RenderPass::Geometry:
        m_pContext->PSSetShader(m_pGeometryPS.Get(), nullptr, 0); // Geometryシェーダー
        break;
        // フォワード描画
    case RenderPass::Forward:
        // サブパスごとの設定
        switch (m_currentForwardSubPass)
        {
            // Sky描画
        case ForwardSubPass::Sky:
            m_pContext->UpdateSubresource(m_pFogBuffer.Get(), 0, nullptr, &m_fogData, 0, 0);     // Fog
            buffer = m_pFogBuffer.Get();                                                         // 借りる
            m_pContext->PSSetConstantBuffers(5, 1, &buffer);                                     // スロット5にセット
            m_pContext->PSSetShader(m_pSkyPS.Get(), nullptr, 0);                                 // Sky用ピクセルシェーダ
            break;
            // アウトライン描画
        case ForwardSubPass::Outline:
            m_pContext->PSSetShader(m_pOutlinePS.Get(), nullptr, 0);   // アウトライン用ピクセルシェーダ
            break;
            // 半透明描画
        case ForwardSubPass::Transparent:
            // シャドウマップをシェーダーにセット
            ID3D11ShaderResourceView* srv = m_pShadowSRV.Get();
            m_pContext->PSSetShaderResources(5, 1, &srv);

            m_pContext->PSSetShader(m_pTransparentPS.Get(), nullptr, 0);   // 半透明用ピクセルシェーダ
            break;
        }
        break;
        // UI描画
    case RenderPass::UI:
        m_pContext->PSSetShader(m_pUIPS.Get(), nullptr, 0);   // UI用ピクセルシェーダ
        break;
    }

    // 描画
    m_pContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);

    // シャドウマップ
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_pContext->PSSetShaderResources(5, 1, &nullSRV);
    return true;
}

//-------------------------------------------
// シャドウマップ描画モードに設定
//-------------------------------------------
void RendererImpl::setShadowMode()
{
    setRasMode(RasMode::None);
    setBlendMode(BlendMode::None);
    setDepthMode(DepthMode::Default);
    setSampMode(SampMode::Border);
    setPass(RenderPass::Shadow);
}

//-------------------------------------------
// ジオメトリ描画モードに設定
//-------------------------------------------
void RendererImpl::setGeometryMode()
{
    setRasMode(RasMode::Back);
    setBlendMode(BlendMode::Opaque);
    setDepthMode(DepthMode::Default);
    setSampMode(SampMode::Wrap);
    setPass(RenderPass::Geometry);
}

//-------------------------------------------
// デカール描画モードに設定
//-------------------------------------------
void RendererImpl::setDecalMode()
{
    setRasMode(RasMode::Front);
    setBlendMode(BlendMode::Decal);
    setDepthMode(DepthMode::None);
    setSampMode(SampMode::Clamp);
    setPass(RenderPass::Decal);
}

//-------------------------------------------
// 半透明描画モードに設定
//-------------------------------------------
void RendererImpl::setForwardMode()
{
    setRasMode(RasMode::Back);
    setBlendMode(BlendMode::Default);
    setDepthMode(DepthMode::Default);
    setSampMode(SampMode::Wrap);
    setPass(RenderPass::Forward);
    setForwardPass(ForwardSubPass::Transparent);
}

//-------------------------------------------
// UI描画モードに設定
//-------------------------------------------
void RendererImpl::setUIMode()
{
    setRasMode(RasMode::Back);
    setBlendMode(BlendMode::None);
    setDepthMode(DepthMode::None);
    setSampMode(SampMode::Wrap);
    setPass(RenderPass::UI);
}

//-------------------------------------------
// アウトライン描画モードに設定
//-------------------------------------------
void RendererImpl::setOutlineMode()
{
    setRasMode(RasMode::Front);
    setBlendMode(BlendMode::Opaque);
    setDepthMode(DepthMode::TestOnly);
    setSampMode(SampMode::Wrap);
    setPass(RenderPass::Forward);
    setForwardPass(ForwardSubPass::Outline);
}

//-------------------------------------------
// Sky描画モードに設定
//-------------------------------------------
void RendererImpl::setSkyMode()
{
    setRasMode(RasMode::Back);
    setBlendMode(BlendMode::Default);
    setDepthMode(DepthMode::TestOnly);
    setSampMode(SampMode::Wrap);
    setPass(RenderPass::Forward);
    setForwardPass(ForwardSubPass::Sky);
}

//-------------------------------------------
// ライティングパス描画モードに設定
//-------------------------------------------
void RendererImpl::setLightingPassMode()
{
    setRasMode(RasMode::None);
    setBlendMode(BlendMode::Opaque);
    setDepthMode(DepthMode::None);
    setSampMode(SampMode::Border);
}

//-------------------------------------------
// ポストプロセス描画モードに設定
//-------------------------------------------
void RendererImpl::setPostProcessMode()
{
    setRasMode(RasMode::Back);
    setBlendMode(BlendMode::Opaque);
    setDepthMode(DepthMode::None);
    setSampMode(SampMode::Clamp);
}

//-------------------------------------------
// 合成設定
//-------------------------------------------
void RendererImpl::setBlendMode(BlendMode blendMode)
{
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pContext->OMSetBlendState(m_blendStates[size_t(blendMode)].Get(), blendFactor, 0xffffffff);
}

//-------------------------------------------
// ラスタライザー設定
//-------------------------------------------
void RendererImpl::setRasMode(RasMode rasMode)
{
    m_pContext->RSSetState(m_rasStates[size_t(rasMode)].Get());
}

//-------------------------------------------
// Zバッファ設定
//-------------------------------------------
void RendererImpl::setDepthMode(DepthMode depthMode)
{
    m_pContext->OMSetDepthStencilState(m_depthStates[size_t(depthMode)].Get(), 1);
}

//-------------------------------------------
// サンプラー設定
//-------------------------------------------
void RendererImpl::setSampMode(SampMode sampMode)
{
    ID3D11SamplerState* samp = m_samplerStates[size_t(sampMode)].Get();
    m_pContext->PSSetSamplers(0, 1, &samp);
}

//-------------------------------------------
// アウトライン設定
//-------------------------------------------
void setOutlineData(Color color, float width)
{

}

//-------------------------------------------
// シザー設定
//-------------------------------------------
void RendererImpl::setScissorRect(int left, int top, int right, int bottom)
{
    D3D11_RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = right;
    rect.bottom = bottom;
    m_pContext->RSSetScissorRects(1, &rect);
}

//-------------------------------------------
// リサイズ処理 (WM_SIZE)
//-------------------------------------------
void RendererImpl::onResize(int width, int height)
{
    if (width <= 0 || height <= 0) return;

    // サイズと倍率を保存する
    m_screenSize.x = static_cast<float>(width);
    m_screenSize.y = static_cast<float>(height);
    m_screenMagnification.x = static_cast<float>(width) / DEFAULT_SCREEN_SIZE.x;
    m_screenMagnification.y = static_cast<float>(height) / DEFAULT_SCREEN_SIZE.y;

    // コンテキストからターゲットを外す
    m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_pContext->ClearState();
    m_pContext->Flush();

    // 破棄
    m_pRenderTargetView.Reset();
    m_pDepthStencilTexture.Reset();
    m_pDepthStencilView.Reset();

    // スワップチェーンのバッファサイズ変更
    if (FAILED(m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
        throw;

    // 現在のMSAA設定を取得する
    DXGI_SWAP_CHAIN_DESC sd = {};
    m_pSwapChain->GetDesc(&sd); // スワップチェーンの設定を取得

    // 新しいレンダーターゲットビューの作成
    ComPtr<ID3D11Texture2D> pBackBuffer{};
    if (SUCCEEDED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)pBackBuffer.GetAddressOf())) && pBackBuffer != nullptr)
    {
        m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_pRenderTargetView.ReleaseAndGetAddressOf());
    }

    // 新しい深度バッファ(Zバッファ)の作成
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = sd.SampleDesc.Count;
    depthDesc.SampleDesc.Quality = sd.SampleDesc.Quality;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (SUCCEEDED(m_pDevice->CreateTexture2D(&depthDesc, nullptr, m_pDepthStencilTexture.ReleaseAndGetAddressOf())) && m_pDepthStencilTexture != nullptr)
    {
        m_pDevice->CreateDepthStencilView(m_pDepthStencilTexture.Get(), nullptr, m_pDepthStencilView.ReleaseAndGetAddressOf());
    }

    // シーンバッファ再生成
    setupSceneBuffer();

    // シャドウマップ再生成
    setupShadowMap();

    // MRT再生成
    setupGBuffer();

    // ターゲットをセットし直す
    ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
    m_pContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());

    // ビューポート設定の更新
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    m_viewportSize = Vector2(vp.Width, vp.Height);
}

//-----------------------------------
// デバイスの取得 (限定的)
//-----------------------------------
ID3D11Device* RendererImpl::getDevice() const
{
    return m_pDevice.Get();
}

//-----------------------------------
// コンテキストの取得 (限定的)
//-----------------------------------
ID3D11DeviceContext* RendererImpl::getContext() const
{
    return m_pContext.Get();
}

//-----------------------------------
// 登録されているHWNDの取得 (限定的)
//-----------------------------------
HWND RendererImpl::getRegisteredHWND() const
{
    return m_hWnd;
}

//-----------------------------------
// レンダラーの核の部分を生成する
//-----------------------------------
void RendererImpl::setupDevice(HWND handle)
{
    // Deviceの作成
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;
    D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevels, 1, D3D11_SDK_VERSION,
        m_pDevice.ReleaseAndGetAddressOf(), &featureLevel, m_pContext.ReleaseAndGetAddressOf()
    );

    // DeferredなのでMSAAはしない
    UINT qualityLevels = 0;
    UINT sampleCount = 1;

    // DXGIデバイスを取得
    ComPtr<IDXGIDevice1> pDXGIDevice = nullptr;
    m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)pDXGIDevice.GetAddressOf());

    // アダプターを取得
    ComPtr<IDXGIAdapter> pAdapter = nullptr;
    pDXGIDevice->GetAdapter(pAdapter.GetAddressOf());

    // ファクトリーを取得
    ComPtr<IDXGIFactory> pFactory = nullptr;
    pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)pFactory.GetAddressOf());

    // 設定
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = UINT(m_screenSize.x);
    sd.BufferDesc.Height = UINT(m_screenSize.y);
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = handle;

    sd.SampleDesc.Count = sampleCount;
    sd.SampleDesc.Quality = qualityLevels;

    sd.Windowed = TRUE;

    // スワップチェーン生成
    if (m_pDevice != nullptr)
    {
        pFactory->CreateSwapChain(m_pDevice.Get(), &sd, m_pSwapChain.ReleaseAndGetAddressOf());
    }

    // スワップチェーンに使用したWindowを保存
    m_hWnd = handle;

    // レンダーターゲットビューの作成
    ComPtr<ID3D11Texture2D> pBackBuffer = nullptr;
    if (SUCCEEDED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)pBackBuffer.GetAddressOf())) && pBackBuffer != nullptr)
    {
        m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_pRenderTargetView.ReleaseAndGetAddressOf());
    }

    // 深度バッファ(Zバッファ)の作成
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = UINT(m_screenSize.x);
    depthDesc.Height = UINT(m_screenSize.y);
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = sampleCount;
    depthDesc.SampleDesc.Quality = qualityLevels;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (SUCCEEDED(m_pDevice->CreateTexture2D(&depthDesc, nullptr, m_pDepthStencilTexture.ReleaseAndGetAddressOf())) && m_pDepthStencilTexture != nullptr)
    {
        m_pDevice->CreateDepthStencilView(m_pDepthStencilTexture.Get(), nullptr, m_pDepthStencilView.ReleaseAndGetAddressOf());
    }

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = m_screenSize.x;
    vp.Height = m_screenSize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    m_pContext->RSSetViewports(1, &vp);

    m_viewportSize = m_screenSize;
}

//-----------------------------------
// シェーダーを生成する
//-----------------------------------
void RendererImpl::setupShader()
{
    // シェーダーのコンパイルと作成
    ComPtr<ID3DBlob> pBlob = nullptr;

    // 2D頂点シェーダー
    std::filesystem::path path = std::filesystem::path(SHADER_DIRECTORY) / L"2DPolygonVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pVertexShader2D.ReleaseAndGetAddressOf());

    // 2D入力レイアウトの作成 (Vertex2D構造体とHLSLの紐づけ)
    D3D11_INPUT_ELEMENT_DESC layout2D[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_pDevice->CreateInputLayout(layout2D, ARRAYSIZE(layout2D), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), m_pInputLayout2D.ReleaseAndGetAddressOf());

    // 3D頂点シェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"3DPolygonVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pVertexShader3D.ReleaseAndGetAddressOf());

    // 3D入力レイアウトの作成 (Vertex3D構造体とHLSLの紐づけ)
    D3D11_INPUT_ELEMENT_DESC layout3D[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_pDevice->CreateInputLayout(layout3D, ARRAYSIZE(layout3D), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), m_pInputLayout3D.ReleaseAndGetAddressOf());

    // Model頂点シェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"ModelVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pVertexShaderModel.ReleaseAndGetAddressOf());

    // 入力レイアウトの作成 (VertexModel構造体とHLSLの紐づけ)
    D3D11_INPUT_ELEMENT_DESC layoutModel[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONES",    0, DXGI_FORMAT_R8G8B8A8_UINT,      0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_pDevice->CreateInputLayout(layoutModel, ARRAYSIZE(layoutModel), pBlob->GetBufferPointer(), pBlob->GetBufferSize(), m_pInputLayoutModel.ReleaseAndGetAddressOf());

    // シャドウ用ピクセルシェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"ShadowPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pShadowPS.ReleaseAndGetAddressOf());

    // ジオメトリピクセルシェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"GeometryPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pGeometryPS.ReleaseAndGetAddressOf());

    // デカール頂点シェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"DecalVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pDecalVS.ReleaseAndGetAddressOf());

    // デカールピクセルシェーダー
    path = std::filesystem::path(SHADER_DIRECTORY) / L"DecalPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pDecalPS.ReleaseAndGetAddressOf());

    // DeferredLighting用
    // VS
    path = std::filesystem::path(SHADER_DIRECTORY) / L"ScreenVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pScreenVS.ReleaseAndGetAddressOf());

    // UnifiedLightingPS
    path = std::filesystem::path(SHADER_DIRECTORY) / L"UnifiedLighting_DL.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pUnifiedLighting_DL_PS.ReleaseAndGetAddressOf());

    // Forward用

    // アウトライン用3D頂点シェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"Outline3DVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pOutline3DVS.ReleaseAndGetAddressOf());

    // アウトライン用Model頂点シェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"OutlineModelVS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pOutlineModelVS.ReleaseAndGetAddressOf());

    // ピクセルシェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"SkyPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pSkyPS.ReleaseAndGetAddressOf());

    // アウトライン用ピクセルシェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"OutlinePS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pOutlinePS.ReleaseAndGetAddressOf());

    // 半透明シェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"TransparentPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pTransparentPS.ReleaseAndGetAddressOf());

    // ポストプロセス用シェーダー
    for (size_t cnt = 0; cnt < m_pPostProcessShaders.size(); ++cnt)
    {
        path = std::filesystem::path(SHADER_DIRECTORY) / POST_PROCESS_SHADER_FILE_NAMES[cnt];
        D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
        m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pPostProcessShaders[cnt].ReleaseAndGetAddressOf());
    }

    // UIシェーダ
    path = std::filesystem::path(SHADER_DIRECTORY) / L"UIPS.hlsl";
    D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, pBlob.ReleaseAndGetAddressOf(), nullptr);
    m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, m_pUIPS.ReleaseAndGetAddressOf());

    //--------------------
    // 定数バッファの作成
    //--------------------

    // VP行列
    D3D11_BUFFER_DESC vpMatd = {};
    vpMatd.ByteWidth = sizeof(VPMatBufferData);
    vpMatd.Usage = D3D11_USAGE_DEFAULT;
    vpMatd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&vpMatd, nullptr, m_pVPMatBuffer.ReleaseAndGetAddressOf());

    // ライト
    D3D11_BUFFER_DESC lgtd = {};
    lgtd.ByteWidth = sizeof(LightBufferData);
    lgtd.Usage = D3D11_USAGE_DEFAULT;
    lgtd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&lgtd, nullptr, m_pLightBuffer.ReleaseAndGetAddressOf());

    // W行列
    D3D11_BUFFER_DESC wMatd = {};
    wMatd.ByteWidth = sizeof(WorldMatBufferData);
    wMatd.Usage = D3D11_USAGE_DEFAULT;
    wMatd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&wMatd, nullptr, m_pWMatBuffer.ReleaseAndGetAddressOf());

    // ライトのVP行列
    D3D11_BUFFER_DESC sbd = {};
    sbd.ByteWidth = sizeof(ShadowBufferData);
    sbd.Usage = D3D11_USAGE_DEFAULT;
    sbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&sbd, nullptr, m_pShadowConstantBuffer.ReleaseAndGetAddressOf());

    // マテリアル
    D3D11_BUFFER_DESC mtld = {};
    mtld.ByteWidth = sizeof(MaterialBufferData);
    mtld.Usage = D3D11_USAGE_DEFAULT;
    mtld.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&mtld, nullptr, m_pMtlBuffer.ReleaseAndGetAddressOf());

    // ボーン
    D3D11_BUFFER_DESC bbd = {};
    bbd.ByteWidth = sizeof(BoneBufferData);
    bbd.Usage = D3D11_USAGE_DEFAULT;
    bbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&bbd, nullptr, m_pBoneBuffer.ReleaseAndGetAddressOf());

    // アウトライン
    D3D11_BUFFER_DESC obd = {};
    obd.ByteWidth = sizeof(OutlineBufferData);
    obd.Usage = D3D11_USAGE_DEFAULT;
    obd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&obd, nullptr, m_pOutlineBuffer.ReleaseAndGetAddressOf());

    // フォグ
    D3D11_BUFFER_DESC fbd = {};
    fbd.ByteWidth = sizeof(FogBufferData);
    fbd.Usage = D3D11_USAGE_DEFAULT;
    fbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&fbd, nullptr, m_pFogBuffer.ReleaseAndGetAddressOf());

    // デカール
    D3D11_BUFFER_DESC dcd = {};
    dcd.ByteWidth = sizeof(DecalBufferData);
    dcd.Usage = D3D11_USAGE_DEFAULT;
    dcd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&dcd, nullptr, m_pDecalBuffer.ReleaseAndGetAddressOf());

    // ポストプロセス
    D3D11_BUFFER_DESC ppd = {};
    ppd.ByteWidth = sizeof(PostProcessBufferData);
    ppd.Usage = D3D11_USAGE_DEFAULT;
    ppd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&ppd, nullptr, m_pPostProcessBuffer.ReleaseAndGetAddressOf());
}

//-----------------------------------
// システム的なテクスチャを生成する
//-----------------------------------
void RendererImpl::setupDummy()
{
    // ダミーテクスチャ(白 1x1)の作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;

    // 白色のピクセルデータ (R,G,B,A = 255)
    uint32_t pixelColor = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &pixelColor;
    initData.SysMemPitch = 4;

    ComPtr<ID3D11Texture2D> pTex = nullptr;
    if (SUCCEEDED(m_pDevice->CreateTexture2D(&texDesc, &initData, pTex.GetAddressOf())))
    {
        m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, m_pDummyTextureWhite.ReleaseAndGetAddressOf());
    }

    // 黒色のピクセルデータ (R,G,B = 0 : A = 255)
    pixelColor = 0xFF000000;
    initData.pSysMem = &pixelColor;

    if (SUCCEEDED(m_pDevice->CreateTexture2D(&texDesc, &initData, pTex.ReleaseAndGetAddressOf())))
    {
        m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, m_pDummyTextureBlack.ReleaseAndGetAddressOf());
    }
}

//-----------------------------------
// 描画設定
//-----------------------------------
void RendererImpl::setupState()
{
    // サンプラーの作成
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.MaxAnisotropy = 16;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    m_pDevice->CreateSamplerState(&sampDesc, m_samplerStates[size_t(SampMode::Wrap)].ReleaseAndGetAddressOf());

    // サンプラーの作成
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.MaxAnisotropy = 16;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_pDevice->CreateSamplerState(&sampDesc, m_samplerStates[size_t(SampMode::Clamp)].ReleaseAndGetAddressOf());

    // サンプラーの作成
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.MaxAnisotropy = 16;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.BorderColor[0] = 1.0f;
    sampDesc.BorderColor[1] = 1.0f;
    sampDesc.BorderColor[2] = 1.0f;
    sampDesc.BorderColor[3] = 1.0f;
    m_pDevice->CreateSamplerState(&sampDesc, m_samplerStates[size_t(SampMode::Border)].ReleaseAndGetAddressOf());

    ID3D11SamplerState* samp = m_samplerStates[size_t(SampMode::Wrap)].Get(); // Wrap
    m_pContext->PSSetSamplers(0, 1, &samp);

    // Zバッファステートの作成
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // 手前にあるものを描画

    // None
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    m_pDevice->CreateDepthStencilState(&dsDesc, m_depthStates[size_t(DepthMode::None)].ReleaseAndGetAddressOf());

    // Default
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    m_pDevice->CreateDepthStencilState(&dsDesc, m_depthStates[size_t(DepthMode::Default)].ReleaseAndGetAddressOf());

    // TestOnly
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    m_pDevice->CreateDepthStencilState(&dsDesc, m_depthStates[size_t(DepthMode::TestOnly)].ReleaseAndGetAddressOf());

    // --- StencilWrite (キャラ本体描画用: 常に成功し、1を書き込む) ---
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsDesc.StencilEnable = TRUE;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;
    // 表面設定 (常にパスし、REF値を書き換える)
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; // REF値に置き換え
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    // 裏面も同じ
    dsDesc.BackFace = dsDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&dsDesc, m_depthStates[size_t(DepthMode::StencilWrite)].ReleaseAndGetAddressOf());

    // --- StencilMask (アウトライン用: ステンシルがREF値と違う場所だけ描く) ---
    dsDesc.DepthEnable = FALSE; // アウトラインは深度無視（最前面）等の場合
    dsDesc.StencilEnable = TRUE;
    // 比較関数: NOT_EQUAL (REF値と同じ場所=キャラ本体 には描かない)
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP; // 書き換えない
    dsDesc.BackFace = dsDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&dsDesc, m_depthStates[size_t(DepthMode::StencilMask)].ReleaseAndGetAddressOf());

    m_pContext->OMSetDepthStencilState(m_depthStates[size_t(DepthMode::Default)].Get(), 1);

    // ラスタライザーステートの作成
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FrontCounterClockwise = FALSE; // 時計回りが表
    rsDesc.DepthClipEnable = TRUE;

    // None
    rsDesc.FillMode = D3D11_FILL_SOLID; // ソリッド描画
    rsDesc.CullMode = D3D11_CULL_NONE;  // カリングなし
    m_pDevice->CreateRasterizerState(&rsDesc, m_rasStates[size_t(RasMode::None)].ReleaseAndGetAddressOf());

    // Front
    rsDesc.FillMode = D3D11_FILL_SOLID; // ソリッド描画
    rsDesc.CullMode = D3D11_CULL_FRONT; // 表面カリング
    m_pDevice->CreateRasterizerState(&rsDesc, m_rasStates[size_t(RasMode::Front)].ReleaseAndGetAddressOf());

    // Back
    rsDesc.FillMode = D3D11_FILL_SOLID; // ソリッド描画
    rsDesc.CullMode = D3D11_CULL_BACK;  // 裏面カリング
    m_pDevice->CreateRasterizerState(&rsDesc, m_rasStates[size_t(RasMode::Back)].ReleaseAndGetAddressOf());

    // Wireframe
    rsDesc.FillMode = D3D11_FILL_WIREFRAME; // ワイヤーフレーム描画
    rsDesc.CullMode = D3D11_CULL_NONE;      // カリングなし
    m_pDevice->CreateRasterizerState(&rsDesc, m_rasStates[size_t(RasMode::Wireframe)].ReleaseAndGetAddressOf());

    // Scissor
    rsDesc.FillMode = D3D11_FILL_SOLID; // ソリッド描画
    rsDesc.CullMode = D3D11_CULL_BACK;  // 裏面カリング
    rsDesc.ScissorEnable = TRUE;        // シザー矩形
    m_pDevice->CreateRasterizerState(&rsDesc, m_rasStates[size_t(RasMode::Scissor)].ReleaseAndGetAddressOf());

    m_pContext->RSSetState(m_rasStates[size_t(RasMode::Back)].Get());

    // ブレンドステートの作成
    D3D11_BLEND_DESC blendDesc{};

    // None
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::None)].ReleaseAndGetAddressOf());

    // Default
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::Default)].ReleaseAndGetAddressOf());

    // Add
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::Add)].ReleaseAndGetAddressOf());

    // Subtract
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::Subtract)].ReleaseAndGetAddressOf());

    // Opaque
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::Opaque)].ReleaseAndGetAddressOf());

    // Decal
    // スペキュラー強度(alpha)は維持
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE; // RGBのみ書き込み許可
    m_pDevice->CreateBlendState(&blendDesc, m_blendStates[size_t(BlendMode::Decal)].ReleaseAndGetAddressOf());

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pContext->OMSetBlendState(m_blendStates[size_t(BlendMode::Default)].Get(), blendFactor, 0xffffffff);
}

//----------------------------------------------------
// シーンバッファ生成
//----------------------------------------------------
void RendererImpl::setupSceneBuffer()
{
    // テクスチャ作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = UINT(m_screenSize.x);
    texDesc.Height = UINT(m_screenSize.y);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // シーン用テクスチャ
    m_pDevice->CreateTexture2D(&texDesc, nullptr, m_pSceneTexture.ReleaseAndGetAddressOf());
    if (m_pSceneTexture == nullptr) return;

    // RTV作成
    m_pDevice->CreateRenderTargetView(m_pSceneTexture.Get(), nullptr, m_pSceneRTV.ReleaseAndGetAddressOf());

    // SRV作成
    m_pDevice->CreateShaderResourceView(m_pSceneTexture.Get(), nullptr, m_pSceneSRV.ReleaseAndGetAddressOf());

    // ワーク用テクスチャ
    m_pDevice->CreateTexture2D(&texDesc, nullptr, m_pWorkTexture.ReleaseAndGetAddressOf());
    if (m_pWorkTexture == nullptr) return;

    // RTV作成
    m_pDevice->CreateRenderTargetView(m_pWorkTexture.Get(), nullptr, m_pWorkRTV.ReleaseAndGetAddressOf());

    // SRV作成
    m_pDevice->CreateShaderResourceView(m_pWorkTexture.Get(), nullptr, m_pWorkSRV.ReleaseAndGetAddressOf());

    // ブルーム用 (1/4サイズ)
    D3D11_TEXTURE2D_DESC bloomDesc = texDesc; // 設定をコピー
    bloomDesc.Width /= 4;                     // 幅 1/4
    bloomDesc.Height /= 4;                    // 高さ 1/4

    // ブルームのRTVとSRVを生成
    for (size_t cnt = 0; cnt < std::min(m_pBloomRTVs.size(), m_pBloomSRVs.size()); ++cnt)
    {
        ComPtr<ID3D11Texture2D> pBloomTex = nullptr;
        m_pDevice->CreateTexture2D(&bloomDesc, nullptr, pBloomTex.GetAddressOf());
        if (pBloomTex != nullptr)
        {
            m_pDevice->CreateRenderTargetView(pBloomTex.Get(), nullptr, m_pBloomRTVs[cnt].ReleaseAndGetAddressOf());
            m_pDevice->CreateShaderResourceView(pBloomTex.Get(), nullptr, m_pBloomSRVs[cnt].ReleaseAndGetAddressOf());
        }
    }
}

//----------------------------------------------------
// MRT用のバッファを生成する
//----------------------------------------------------
void RendererImpl::setupGBuffer()
{
    // 各バッファのフォーマット
    DXGI_FORMAT formats[GBUFFER_COUNT]
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   // 0: Albedo (色) sRGBで保存
        DXGI_FORMAT_R16G16B16A16_FLOAT,    // 1: Normal (法線) - 精度が必要
        DXGI_FORMAT_R32G32B32A32_FLOAT,    // 2: Position (座標) - かなり精度が必要
        DXGI_FORMAT_R16G16B16A16_FLOAT     // 3: Emissive (発光)
    };

    for (int cnt = 0; cnt < GBUFFER_COUNT; ++cnt)
    {
        // テクスチャ作成
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = UINT(m_screenSize.x);
        texDesc.Height = UINT(m_screenSize.y);
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = formats[cnt];
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // 書く＆読む
        texDesc.CPUAccessFlags = 0;
        m_pDevice->CreateTexture2D(&texDesc, nullptr, m_pGBufferTextures[cnt].ReleaseAndGetAddressOf());

        // RenderTargetView (書き込み用) 作成
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = formats[cnt];
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        m_pDevice->CreateRenderTargetView(m_pGBufferTextures[cnt].Get(), &rtvDesc, m_pGBufferRTVs[cnt].ReleaseAndGetAddressOf());

        // ShaderResourceView (読み込み用) 作成
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = formats[cnt];
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        m_pDevice->CreateShaderResourceView(m_pGBufferTextures[cnt].Get(), &srvDesc, m_pGBufferSRVs[cnt].ReleaseAndGetAddressOf());
    }
}

//----------------------------------------------------
// シャドウマップ生成
//----------------------------------------------------
void RendererImpl::setupShadowMap()
{
    // テクスチャの作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_shadowMapResolution;
    texDesc.Height = m_shadowMapResolution;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // 32bit深度
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // DSVかつSRV
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    if (FAILED(m_pDevice->CreateTexture2D(&texDesc, nullptr, m_pShadowTexture.ReleaseAndGetAddressOf()))) return;

    // 書き込み用ビューの作成
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    m_pDevice->CreateDepthStencilView(m_pShadowTexture.Get(), &dsvDesc, m_pShadowDSV.ReleaseAndGetAddressOf());

    // 読み込み用ビューの作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 赤色として深度を読む
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    m_pDevice->CreateShaderResourceView(m_pShadowTexture.Get(), &srvDesc, m_pShadowSRV.ReleaseAndGetAddressOf());
}

//----------------------------------------------------
// フォントの設定
//----------------------------------------------------
void RendererImpl::setupFont()
{
    try
    {
        // SpriteBatchの初期化
        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_pContext.Get());

        // フォントファイルの読み込み
        m_spriteFont = std::make_unique<DirectX::SpriteFont>(m_pDevice.Get(), L"data/FONT/test.spritefont");
    }
    catch (const std::exception& e)
    {
        std::wstringstream st{};
        st << "ERROR::Font::" << e.what() << std::endl;
        std::wstring es = st.str();
        OutputDebugString(es.c_str());
    }
}

//----------------------------------------------------
// シーンバッファ破棄
//----------------------------------------------------
void RendererImpl::releaseSceneBuffer()
{
    // ブルーム
    m_pBloomRTVs.fill(nullptr);
    m_pBloomSRVs.fill(nullptr);

    // リソース
    m_pWorkSRV.Reset();
    m_pSceneSRV.Reset();

    // ターゲット
    m_pWorkRTV.Reset();
    m_pSceneRTV.Reset();

    // テクスチャ
    m_pWorkTexture.Reset();
    m_pSceneTexture.Reset();
}

//---------------------------------
// MRT破棄
//---------------------------------
void RendererImpl::releaseGBuffer()
{
    // リソース
    m_pGBufferSRVs.fill(nullptr);

    // ターゲット
    m_pGBufferRTVs.fill(nullptr);

    // テクスチャ
    m_pGBufferTextures.fill(nullptr);
}

//----------------------------------------------------
// シャドウマップ破棄
//----------------------------------------------------
void RendererImpl::releaseShadowMap()
{
    // リソース
    m_pShadowSRV.Reset();

    // ターゲット
    m_pShadowDSV.Reset();

    // テクスチャ
    m_pShadowTexture.Reset();
}

//---------------------------------
// デカールの描画
//---------------------------------
void RendererImpl::drawDecal(Matrix transform, const MeshHandle& handle, Color color)
{
    setTransformWorld(transform);
    setMesh(handle);

    // 逆行列
    DecalBufferData cb;
    Matrix::Inverse(transform, cb.InverseWorld);
    cb.DecalColor = color;

    // 定数バッファ更新
    m_pContext->UpdateSubresource(m_pWMatBuffer.Get(), 0, nullptr, &m_wMatData, 0, 0);   // ワールド行列
    m_pContext->UpdateSubresource(m_pDecalBuffer.Get(), 0, nullptr, &cb, 0, 0);          // デカール情報

    // 頂点シェーダーに送る
    ID3D11Buffer* buffer = m_pWMatBuffer.Get();
    m_pContext->VSSetConstantBuffers(1, 1, &buffer);  // スロット1にセット

    // ピクセルシェーダーに送る
    buffer = m_pMtlBuffer.Get();
    m_pContext->PSSetConstantBuffers(2, 1, &buffer); // スロット2にセット
    buffer = m_pDecalBuffer.Get();
    m_pContext->PSSetConstantBuffers(4, 1, &buffer); // スロット4にセット

    // シェーダー設定
    m_pContext->IASetInputLayout(m_pInputLayout3D.Get());  // 入力レイアウト設定
    m_pContext->VSSetShader(m_pDecalVS.Get(), nullptr, 0); // 頂点シェーダー設定

    // ピクセルシェーダー設定
    m_pContext->PSSetShader(m_pDecalPS.Get(), nullptr, 0);

    // 描画
    if (handle.isValid())
    m_pContext->DrawIndexed(UINT(m_meshs[handle.id].indicesCount), 0, 0);
}

//---------------------------------
// MRTの描画 (実際に画面に出す)
//---------------------------------
void RendererImpl::drawLightingPass()
{
    // シーンをセット
    ID3D11RenderTargetView* rtv = m_pSceneRTV.Get();
    m_pContext->OMSetRenderTargets(1, &rtv, nullptr);

    // シーンをクリアする
    clearRenderTarget(m_pSceneRTV.Get());

    // リソースをシェーダーにセット
    m_pContext->PSSetShaderResources(1, GBUFFER_COUNT, MakeRawArray(m_pGBufferSRVs).data());

    // シャドウマップをシェーダーにセット
    ID3D11ShaderResourceView* srv = m_pShadowSRV.Get();
    m_pContext->PSSetShaderResources(5, 1, &srv);

    // シェーダー切り替え
    m_pContext->IASetInputLayout(nullptr);
    m_pContext->VSSetShader(m_pScreenVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pUnifiedLighting_DL_PS.Get(), nullptr, 0);

    ID3D11Buffer* buffer = nullptr;

    // ライトバッファの更新
    m_pContext->UpdateSubresource(m_pLightBuffer.Get(), 0, nullptr, &m_lightData, 0, 0);
    buffer = m_pLightBuffer.Get();
    m_pContext->PSSetConstantBuffers(3, 1, &buffer);

    // VP行列を定数バッファに送る
    ShadowBufferData shadowData;
    shadowData.LightViewProj = m_lightVPMatrix;
    m_pContext->UpdateSubresource(m_pShadowConstantBuffer.Get(), 0, nullptr, &shadowData, 0, 0);
    buffer = m_pShadowConstantBuffer.Get();
    m_pContext->PSSetConstantBuffers(4, 1, &buffer);

    // ライトバッファの更新
    m_pContext->UpdateSubresource(m_pFogBuffer.Get(), 0, nullptr, &m_fogData, 0, 0);
    buffer = m_pFogBuffer.Get();
    m_pContext->PSSetConstantBuffers(5, 1, &buffer);

    // 描画設定
    setLightingPassMode();

    // 描画
    m_pContext->Draw(3, 0);

    // 戻す
    setForwardMode();

    // リソース解除
    
    // G-Buffer
    ID3D11ShaderResourceView* nullSRVs[GBUFFER_COUNT] = { nullptr, nullptr, nullptr,nullptr };
    m_pContext->PSSetShaderResources(1, GBUFFER_COUNT, nullSRVs);

    // シャドウマップ
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_pContext->PSSetShaderResources(5, 1, &nullSRV);
}

//---------------------------------
// ポストプロセスの描画
//---------------------------------
void RendererImpl::drawPostProcessPass(PostProcessShaderMask mask, ToneMappingType type)
{
    ID3D11Buffer* buffer = nullptr;

    // 定数バッファ更新
    PostProcessBufferData cb;
    float w = (float)m_screenSize.x;
    float h = (float)m_screenSize.y;
    cb.ScreenSize = Vector4(w, h, 1.0f / w, 1.0f / h); // z,w には逆数を入れる
    cb.BlurDir = Vector2::Zero();                      // ブラー方向をいったん初期化
    cb.bloomThreshold = 1.2f;                          // ブルーム閾値
    cb.bloomIntensity = 2.0f;                          // ブルーム係数
    cb.toneMappingType = int(type);                    // トーンマッピングの種類
    m_pContext->UpdateSubresource(m_pPostProcessBuffer.Get(), 0, nullptr, &cb, 0, 0);
    buffer = m_pPostProcessBuffer.Get();
    m_pContext->PSSetConstantBuffers(0, 1, &buffer);

    // 全画面用 (ScreenVS)
    m_pContext->VSSetShader(m_pScreenVS.Get(), nullptr, 0);

    // ポストプロセス用設定
    setPostProcessMode();

    // 今、絵が入っている場所 (最初SceneTexture)
    ID3D11ShaderResourceView* currentSRV = m_pSceneSRV.Get();

    // 次に書き込む場所 (最初WorkTexture)
    ID3D11RenderTargetView* nextRTV = m_pWorkRTV.Get();

    // 次のRTVに対応するSRV (スワップ用)
    ID3D11ShaderResourceView* nextSRV = m_pWorkSRV.Get();

    // Maskチェック
    bool isPP = Any(mask), isBloom = HasFlag(mask, PostProcessShaderMask::Bloom), isGray = HasFlag(mask, PostProcessShaderMask::Gray), isFXAA = HasFlag(mask, PostProcessShaderMask::FXAA);

    //---------------
    // ポストプロセス
    //---------------
    if (isPP)
    {
        if (isBloom)
        {
            // ビューポートを1/4に変更
            D3D11_VIEWPORT vp = { 0, 0, w / 4.0f, h / 4.0f, 0.0f, 1.0f };
            m_pContext->RSSetViewports(1, &vp);

            // 抽出
            cb.bloomThreshold = 1.0f; // これ以上の彩度光がブラー対象です
            m_pContext->UpdateSubresource(m_pPostProcessBuffer.Get(), 0, nullptr, &cb, 0, 0);

            // 書き込み
            ID3D11RenderTargetView* rtv = m_pBloomRTVs[0].Get();
            m_pContext->OMSetRenderTargets(1, &rtv, nullptr);
            clearRenderTarget(rtv);

            // 読み込み
            m_pContext->PSSetShaderResources(0, 1, &currentSRV); // 今のシーン

            // シェーダー
            m_pContext->PSSetShader(m_pPostProcessShaders[size_t(PostProcessShaderType::BloomExtract)].Get(), nullptr, 0); // 光を抽出するシェーダ
            m_pContext->Draw(3, 0);

            // SRVの解除
            ID3D11ShaderResourceView* nullSRV = nullptr;
            m_pContext->PSSetShaderResources(0, 1, &nullSRV);

            // 横ぼかし
            cb.BlurDir = Vector2(1.0f, 0.0f); // 横方向
            m_pContext->UpdateSubresource(m_pPostProcessBuffer.Get(), 0, nullptr, &cb, 0, 0);

            // 書き込み
            rtv = m_pBloomRTVs[1].Get();
            m_pContext->OMSetRenderTargets(1, &rtv, nullptr);
            clearRenderTarget(rtv);

            // 読み込み
            ID3D11ShaderResourceView* srv = m_pBloomSRVs[0].Get();
            m_pContext->PSSetShaderResources(0, 1, &srv);

            // シェーダー
            m_pContext->PSSetShader(m_pPostProcessShaders[size_t(PostProcessShaderType::GaussianBlur)].Get(), nullptr, 0);
            m_pContext->Draw(3, 0);

            // SRVの解除
            m_pContext->PSSetShaderResources(0, 1, &nullSRV);

            // 縦ぼかし
            cb.BlurDir = Vector2(0.0f, 1.0f); // 縦方向
            m_pContext->UpdateSubresource(m_pPostProcessBuffer.Get(), 0, nullptr, &cb, 0, 0);

            // 書き込み
            rtv = m_pBloomRTVs[0].Get();
            m_pContext->OMSetRenderTargets(1, &rtv, nullptr);
            clearRenderTarget(rtv);

            // 読み込み
            srv = m_pBloomSRVs[1].Get();
            m_pContext->PSSetShaderResources(0, 1, &srv);

            // シェーダー
            m_pContext->PSSetShader(m_pPostProcessShaders[size_t(PostProcessShaderType::GaussianBlur)].Get(), nullptr, 0);
            m_pContext->Draw(3, 0);

            // SRVの解除
            m_pContext->PSSetShaderResources(0, 1, &nullSRV);

            // ビューポートを戻す
            D3D11_VIEWPORT fullVP = { 0, 0, w, h, 0.0f, 1.0f };
            m_pContext->RSSetViewports(1, &fullVP);
        }
        if (isGray)
        {
            // 書き込み
            m_pContext->OMSetRenderTargets(1, &nextRTV, nullptr);
            clearRenderTarget(nextRTV);

            // 読み込み
            m_pContext->PSSetShaderResources(0, 1, &currentSRV);

            // シェーダー
            m_pContext->PSSetShader(m_pPostProcessShaders[size_t(PostProcessShaderType::Gray)].Get(), nullptr, 0);
            m_pContext->Draw(3, 0);

            // SRVの解除
            ID3D11ShaderResourceView* nullSRV = nullptr;
            m_pContext->PSSetShaderResources(0, 1, &nullSRV);

            // SRVを今描画した結果にする
            std::swap(currentSRV, nextSRV);
            nextRTV = (currentSRV == m_pSceneSRV.Get()) ? m_pWorkRTV.Get() : m_pSceneRTV.Get();
        }
    }

    // ブラー合成,色調調整をしてLDRして最終的なシーンテクスチャを完成させる
    {
        // 書き込み
        m_pContext->OMSetRenderTargets(1, &nextRTV, nullptr);
        clearRenderTarget(nextRTV);

        // 読み込み
        m_pContext->PSSetShaderResources(0, 1, &currentSRV);

        // ブルームが有効な場合ブルームの結果テクスチャをセット 無効な時は黒ダミーテクスチャで加算をさせないようにする
        ID3D11ShaderResourceView* bloomTex = isBloom ? m_pBloomSRVs[0].Get() : m_pDummyTextureBlack.Get();
        m_pContext->PSSetShaderResources(1, 1, &bloomTex);

        // シェーダー
        m_pContext->PSSetShader(m_pPostProcessShaders[size_t(PostProcessShaderType::Compossite)].Get(), nullptr, 0);
        m_pContext->Draw(3, 0);

        // SRVの解除
        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_pContext->PSSetShaderResources(0, 1, &nullSRV);

        // SRVを今描画した結果にする
        std::swap(currentSRV, nextSRV);
    }

    // シーンテクスチャをバックバッファに出力(コピー)する(FXAAが有効な場合FXAAを行いつつ出力)
    {
        // バックバッファに書き込み
        ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
        m_pContext->OMSetRenderTargets(1, &rtv, nullptr);
        clearRenderTarget(rtv);

        // 最終結果
        m_pContext->PSSetShaderResources(0, 1, &currentSRV);

        // バックバッファにコピー
        ID3D11PixelShader* pBackBufferPS = isFXAA ? m_pPostProcessShaders[size_t(PostProcessShaderType::FXAA)].Get() : m_pPostProcessShaders[size_t(PostProcessShaderType::None)].Get();
        m_pContext->PSSetShader(pBackBufferPS, nullptr, 0);

        // 描画
        m_pContext->Draw(3, 0);
    }

    // 解除
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_pContext->PSSetShaderResources(0, 1, &nullSRV);
}

//---------------------------------
// 文字の描画
//---------------------------------
void RendererImpl::drawString(std::string_view string, Vector2 pos, Color color, float angle, Vector2 scale)
{
    // 描画開始
    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        DirectX::XMMatrixIdentity()
    );

    // テキスト描画
    m_spriteFont->DrawString(
        m_spriteBatch.get(),
        string.data(),
        DirectX::XMVECTOR{ pos.x, pos.y },
        DirectX::XMVECTOR{ color.r, color.g,color.b,color.a },
        angle,
        DirectX::g_XMZero,
        DirectX::XMVECTOR{ scale.x, scale.y }
    );

    // 描画終了
    m_spriteBatch->End();
}

//---------------------------------
// テクスチャを作成する
//---------------------------------
void RendererImpl::createTexture(std::shared_ptr<TextureData> spTextureData, uint32_t id)
{
    ComPtr<ID3D11ShaderResourceView> pSRV = nullptr;

    HRESULT hr = S_OK;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return;
    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratchImage;

    // 画像データを ScratchImage に読み込む
    switch (spTextureData->type)
    {
    case TextureType::Raw:
    {
        // Imageに変換
        DirectX::Image img = {};
        img.width = spTextureData->width;
        img.height = spTextureData->height;
        img.format = DXGI_FORMAT_R8G8B8A8_UNORM; // 生データはUNORM
        img.rowPitch = spTextureData->width * 4; // 4バイト(RGBA)
        img.slicePitch = img.rowPitch * spTextureData->height;
        img.pixels = spTextureData->rawPixels.get();

        // ScratchImageを作成
        hr = scratchImage.InitializeFromImage(img);

        // メタデータを取得
        if (SUCCEEDED(hr)) metadata = scratchImage.GetMetadata();
        break;
    }
    case TextureType::Wic:
        hr = DirectX::LoadFromWICMemory(spTextureData->buffer.data(), spTextureData->buffer.size(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
        break;
    case TextureType::Dds:
        hr = DirectX::LoadFromDDSMemory(spTextureData->buffer.data(), spTextureData->buffer.size(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
        break;
    case TextureType::Tga:
        hr = DirectX::LoadFromTGAMemory(spTextureData->buffer.data(), spTextureData->buffer.size(), DirectX::TGA_FLAGS_NONE, &metadata, scratchImage);
        break;
    }

    if (SUCCEEDED(hr)) { spTextureData->width = static_cast<int>(metadata.width); spTextureData->height = static_cast<int>(metadata.height); }
    else return;

    // ミップマップ生成 (CPU側処理)
    // ミップマップが1枚しかなく、圧縮フォーマットでない場合は生成する
    if (metadata.mipLevels == 1 && !DirectX::IsCompressed(metadata.format))
    {
        DirectX::ScratchImage mipChain;
        // フィルタリングを行いながらミップマップ生成
        hr = DirectX::GenerateMipMaps(
            scratchImage.GetImages(), scratchImage.GetImageCount(), metadata,
            DirectX::TEX_FILTER_DEFAULT, 0,
            mipChain
        );

        if (SUCCEEDED(hr))
        {
            // 生成したものに差し替え
            scratchImage = std::move(mipChain);
            metadata = scratchImage.GetMetadata();
        }
    }

    // フォーマット調整 (UNORM -> TYPELESS & SRGB)
    DXGI_FORMAT resourceFormat = metadata.format;
    DXGI_FORMAT viewFormat = metadata.format;

    if (metadata.format == DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        resourceFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        viewFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }
    else if (metadata.format == DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        resourceFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        viewFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    }
    else if (metadata.format == DXGI_FORMAT_BC1_UNORM)
    {
        resourceFormat = DXGI_FORMAT_BC1_TYPELESS;
        viewFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
    }
    else if (metadata.format == DXGI_FORMAT_BC2_UNORM)
    {
        resourceFormat = DXGI_FORMAT_BC2_TYPELESS;
        viewFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
    }
    else if (metadata.format == DXGI_FORMAT_BC3_UNORM)
    {
        resourceFormat = DXGI_FORMAT_BC3_TYPELESS;
        viewFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
    }

    // テクスチャリソース作成 (TYPELESSで作成)
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)metadata.width;
    desc.Height = (UINT)metadata.height;
    desc.MipLevels = (UINT)metadata.mipLevels;
    desc.ArraySize = (UINT)metadata.arraySize;
    desc.Format = resourceFormat; // TYPELESS
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // サブリソースデータの準備
    std::vector<D3D11_SUBRESOURCE_DATA> subResources;
    const DirectX::Image* images = scratchImage.GetImages();
    size_t nImages = scratchImage.GetImageCount();
    subResources.resize(nImages);
    for (size_t i = 0; i < nImages; ++i)
    {
        subResources[i].pSysMem = images[i].pixels;
        subResources[i].SysMemPitch = (UINT)images[i].rowPitch;
        subResources[i].SysMemSlicePitch = (UINT)images[i].slicePitch;
    }

    ComPtr<ID3D11Texture2D> pTexResource = nullptr;
    hr = m_pDevice->CreateTexture2D(&desc, subResources.data(), pTexResource.GetAddressOf());
    if (FAILED(hr)) return;

    // SRV作成 (SRGBで作成)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = viewFormat; // SRGB
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    hr = m_pDevice->CreateShaderResourceView(pTexResource.Get(), &srvDesc, pSRV.GetAddressOf());

    if (FAILED(hr)) return;

    {// m_texturesに追加
        std::lock_guard<std::mutex> lock(m_texMutex);
        m_textures[id] = pSRV;
    }
}

//---------------------------------
// 行列のセット
//---------------------------------
void RendererImpl::setVPMatrix(Matrix view, Matrix proj)
{
    // プロジェクションマトリックス
    m_vpMatData.Proj = proj;

    // ビューマトリクス
    m_vpMatData.View = view;

    // VPバッファの更新とセット
    m_pContext->UpdateSubresource(m_pVPMatBuffer.Get(), 0, nullptr, &m_vpMatData, 0, 0);
    ID3D11Buffer* buffer = m_pVPMatBuffer.Get();
    m_pContext->VSSetConstantBuffers(0, 1, &buffer);
}

//---------------------------------
// 正射影行列のセット
//---------------------------------
void RendererImpl::setOrthographic()
{
    // 正射影行列の計算
    m_vpMatData.Proj = Matrix::OrthographicOffCenter(0.0f, m_screenSize.x, m_screenSize.y, 0.0f, 0.0f, 1.0f);

    // ビューマトリクスは単位行列
    m_vpMatData.View.identity();

    // VPバッファの更新とセット
    m_pContext->UpdateSubresource(m_pVPMatBuffer.Get(), 0, nullptr, &m_vpMatData, 0, 0);
    ID3D11Buffer* buffer = m_pVPMatBuffer.Get();
    m_pContext->VSSetConstantBuffers(0, 1, &buffer);
}

//----------------------------
// 
// レンダラー (外部インターフェース)
//
// implの同名の関数に渡すだけ
// 
//----------------------------

Renderer::Renderer() : m_pImpl{ std::make_unique<RendererImpl>() } {}
Renderer::~Renderer() = default;

void Renderer::init(HWND handle, long width, long height)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->init(handle, width, height);
    }
}

void Renderer::uninit()
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->uninit();
    }
}

bool Renderer::render(const Scene& scene, std::function<void()> guiRender)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->render(scene, guiRender, *this);
    }
    return false;
}

bool Renderer::uploadTextures(const TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->uploadTextures(textureManager, maxThread, progressCallback);
    }
    return false;
}

MeshHandle Renderer::createMesh(VertexShaderType type, const void* vertices, size_t verticesCount, const void* indices, size_t indicesCount)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->createMesh(type, vertices, verticesCount, indices, indicesCount);
    }
    return MeshHandle();
}

bool Renderer::setMesh(const MeshHandle& handle)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setMesh(handle);
    }
    return false;
}

bool Renderer::setTexture(const TextureHandle& handle)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setTexture(handle);
    }
    return false;
}

bool Renderer::setTransformWorld(const Matrix& matrix)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setTransformWorld(matrix);
    }
    return false;
}

bool Renderer::setMaterial(const Material& material)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setMaterial(material);
    }
    return false;
}

bool Renderer::setFog(const FogData& fog)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setFog(fog);
    }
    return false;
}

bool Renderer::drawMesh(const MeshHandle& handle)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->drawMesh(handle);
    }
    return false;
}

bool Renderer::drawIndexedPrimitive(VertexShaderType vertexShaderType, int indexCount, unsigned int startIndexLocation, unsigned int baseVertexLocation)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->drawIndexedPrimitive(vertexShaderType, indexCount, startIndexLocation, baseVertexLocation);
    }
    return false;
}

void Renderer::drawDecal(Matrix transform, const MeshHandle& handle, Color color)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->drawDecal(transform,handle, color);
    }
}

void Renderer::drawString(std::string_view string, Vector2 pos, Color color, float angle, Vector2 scale)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->drawString(string, pos, color, angle, scale);
    }
}

bool Renderer::setBoneTransforms(std::span<const Matrix> boneTransforms)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setBoneTransforms(boneTransforms);
    }
    return false;
}

void Renderer::setOutlineData(OutlineData data)
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->setOutlineData(data);
    }
}

void Renderer::setPostProcessShaderMask(PostProcessShaderMask mask)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->setPostProcessShaderMask(mask);
    }
}
void Renderer::setToneMappingType(ToneMappingType type)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->setToneMappingType(type);
    }
}
void Renderer::setShadowMapResolution(int resolution)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->setShadowMapResolution(resolution);
    }
}
void Renderer::setShadowMapArea(float width, float height, float near, float fur)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->setShadowMapArea(width, height, near, fur);
    }
}
void Renderer::setAmbient(const Color& ambient)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->setAmbient(ambient);
    }
}

void Renderer::onResize(int width, int height)
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->onResize(width, height);
    }
}

void Renderer::getScreenSizeMagnification(Vector2& magnification) const
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->getScreenSizeMagnification(magnification);
    }
}

void Renderer::getViewportSize(Vector2& size) const
{
    if (m_pImpl != nullptr)
    {
        m_pImpl->getViewportSize(size);
    }
}

ID3D11Device* Renderer::getDevice() const
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->getDevice();
    }
    return nullptr;
}

ID3D11DeviceContext* Renderer::getContext() const
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->getContext();
    }
    return nullptr;
}

HWND Renderer::getRegisteredHWND() const
{
    if (m_pImpl != nullptr)
    {
        return m_pImpl->getRegisteredHWND();
    }
    return nullptr;
}
