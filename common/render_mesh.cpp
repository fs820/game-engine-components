//--------------------------------------------
//
// Mesh描画用コンポーネント [render_mesh.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "render_mesh.h"
#include "renderer.h"
#include "object.h"
#include "trans_comp.h"
#include "log.h"

//-------------------------------------
// 
// Mesh描画用コンポーネントクラス
// 
//-------------------------------------

//----------------------------
// 描画
//----------------------------
void MeshRenderComponent::render(Renderer& renderer)
{
    // ワールド変換の設定
    Transform transform{};
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    transform = trans->get();
    renderer.setTransformWorld(transform.toMatrix());

    // マテリアルとテクスチャの設定
    renderer.setMaterial(m_material);
    renderer.setTexture(m_texture);

    // アウトライン
    renderer.setOutlineData(m_outline);

    // 描画
    renderer.drawMesh(m_mesh);
}
