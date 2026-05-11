//--------------------------------------------
//
// モデル [model.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once

#include "pch.h"
#include "graphics_types.h" // VertexModel, Color

// 前方宣言
class Renderer;           // レンダラー
class ModelManager;       // モデルマネージャー
class TextureManager;     // テクスチャマネージャー
class ModelResource;      // リソース本体
struct Node;              // ノード構造体
struct Animation;         // アニメーション構造体
struct NodeAnimation;     // ノードアニメーション構造体

constexpr size_t INVALID_ANIM_ID = ~0u;     // 無効値

// ノードのインスタンス情報
struct NodeInstance
{
    const Node* node;         // ノードへのポインタ
    Transform localTransform; // ローカル変換
    Matrix globalTransform;   // ワールド変換行列

    NodeInstance() : node{ nullptr }, localTransform{} {}
    ~NodeInstance() = default;
};

// アニメーションのインスタンス情報
struct AnimationInstance
{
    size_t animationIndex; // アニメーションへのインデックス
    double currentTime;    // 現在の再生時間 (Tick)
    bool isPlaying;        // 再生中フラグ
    bool isLoop;           // ループ再生フラグ

    AnimationInstance() : animationIndex{ INVALID_ANIM_ID }, currentTime{ 0.0 }, isPlaying{ false }, isLoop{ false } {}
    ~AnimationInstance() = default;
};

// モデルスロット構造体
struct ModelSlot
{
    std::filesystem::path path;             // モデルパス
    std::vector<ModelHandle> motionHandles; // 読み込んだモーションハンドル
    std::shared_ptr<ModelResource> data;    // モデルリソース
    bool isAnimationOnly;                   // アニメーションのみのデータ

    ModelSlot() : path{}, motionHandles{}, data{}, isAnimationOnly{} {}
    ~ModelSlot() = default;
};

//----------------------------
// モデルの動的インスタンス
//----------------------------
class Model
{
public:
    Model(ModelManager& modelManager, Renderer& renderer, const ModelHandle& handle);
    ~Model() { uninit(); }

    void init();
    void uninit();
    void update(float deltaTime, const Matrix& worldMatrix);
    void draw();
    void setAnimation(size_t animationIndex = 0u, double blendDuration = 0.0, bool isSync = false, bool isLoop = false, bool forceReset = false);
    bool isAnimationPlaying() const { return m_currentAnimation.isPlaying || m_nextAnimation.isPlaying; }
    void setScale(float scale);
    void setPixelShaderType(PixelShaderType pixelShaderType) { m_pixelShaderType = pixelShaderType; }

private:
    void setupNodeInstances(Node* node, const Matrix& parentTransform);
    void updateAnimation(double deltaTime);
    void updateNodeTransforms(NodeInstance* node, const Matrix& parentTransform);
    void updateBoneTransforms();
    void drawNode(NodeInstance* node);
    void updateNodeAnimTransforms(NodeInstance* node, Animation* currentAnim, Animation* nextAnim, double currentTime, double nextTime, bool isCurrentLoop, bool isNextLoop);
    Transform getAnimatedTransform(NodeInstance* node, const Animation* anim, const Transform& defaultTransform, double currentTime, bool isLoop);
    void setupBlendStartPose();

    ModelManager& m_modelManager; // モデルマネージャー参照
    Renderer& m_renderer;         // レンダラー参照

    const ModelHandle m_handle;                // 静的なモデルリソースのハンドル

    std::unordered_map<std::string, NodeInstance> m_nodeInstanceMaps; // ノードインスタンスマップ
    std::vector<Matrix> m_boneTransforms;                             // 最終的なボーン変換行列リスト
    AnimationInstance m_currentAnimation;                             // 現在のアニメーション情報
    AnimationInstance m_nextAnimation;                                // 次のアニメーション情報
    Animation* m_pBlendStartPose;                                     // ブレンド開始ポーズ (ブレンド中に新しいアニメーションが来た場合用)
    double m_blendDuration;                                           // ブレンドにかける時間
    double m_blendTime;                                               // ブレンド経過時間
    bool m_isSync;                                                    // 同期ブレンドフラグ

    PixelShaderType m_pixelShaderType;                                // ピクセルシェーダ

    Transform m_transform;                                            // モデル全体変換値
};

//----------------------------
// モデルマネージャー
//----------------------------
class ModelManager
{
public:
    ModelManager() : m_slotsMutex{}, m_slots{}, m_idToHandle{} {}
    ~ModelManager() = default;

    bool load(Renderer& renderer, TextureManager& textureManager, unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback = {}, uint64_t id = Hash(""));

    bool registerPath(uint64_t id, const std::filesystem::path& path, bool isAnimationOnly = false);
    bool setAnimation(uint64_t destModel, uint64_t srcAnim);

    void releaseCpuResources();
    void releaseCpuResource(const ModelHandle& handle);

    ModelHandle getModelHandle(uint64_t id);
    std::weak_ptr<ModelResource> getModelData(const ModelHandle& handle) const;

private:
    std::mutex m_slotsMutex;                                // ↓のmutex
    std::vector<ModelSlot> m_slots;                         // モデルスロット
    std::unordered_map<uint64_t, ModelHandle> m_idToHandle; // ID -> ハンドルのマップ
};
