//--------------------------------------------
//
// Decal描画用コンポーネント [render_decal.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "render.h"

//-------------------------------------
// Decal描画用コンポーネントクラス
//-------------------------------------
class DecalRenderComponent : public RenderComponent
{
public:
    DecalRenderComponent(MeshHandle mesh, TextureHandle texture, Color color = Color::White())
        : RenderComponent(RenderQueueMask::Decal, RasMode::Front), m_mesh(mesh), m_texture(texture), m_color(color) {}
    ~DecalRenderComponent() = default;

    void render(Renderer& renderer) override;

    void setMeshHandle(const MeshHandle& mesh) { m_mesh = mesh; }
    void setTextureHandle(const TextureHandle& texture) { m_texture = texture; }
    void setColor(const Color& color) { m_color = color; }

private:
    MeshHandle m_mesh;       // 描画範囲を決めるメッシュのハンドル
    TextureHandle m_texture; // 描画に使用するテクスチャ
    Color m_color;           // 色
};
