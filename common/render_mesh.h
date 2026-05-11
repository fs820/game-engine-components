//--------------------------------------------
//
// Mesh描画用コンポーネント [render_mesh.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "render.h"

//-------------------------------------
// Mesh描画用コンポーネントクラス
//-------------------------------------
class MeshRenderComponent : public RenderComponent
{
public:
    MeshRenderComponent(const RenderQueueMask& renderQueueMask, RasMode mode, MeshHandle mesh, Material material, TextureHandle texture, OutlineData outlineData = {})
        : RenderComponent(renderQueueMask, mode), m_mesh(mesh), m_material(material), m_texture(texture), m_outline(outlineData) {}
    ~MeshRenderComponent() = default;

    void render(Renderer& renderer) override;

    void setMeshHandle(const MeshHandle& mesh) { m_mesh = mesh; }
    void setMaterial(const Material& material) { m_material = material; }
    void setTextureHandle(const TextureHandle& texture) { m_texture = texture; }
    void setOutline(const OutlineData& outline) { m_outline = outline; }

private:
    MeshHandle m_mesh;       // 描画するメッシュのハンドル
    Material m_material;     // 描画に使用するマテリアル
    TextureHandle m_texture; // 描画に使用するテクスチャ
    OutlineData m_outline;   // アウトライン
};
