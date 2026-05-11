//--------------------------------------------
//
// Decal描画用コンポーネント [render_decal.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "render_decal.h"
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
void DecalRenderComponent::render(Renderer& renderer)
{
    // ワールド変換の設定
    Transform transform{};
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    transform = trans->get();

    // テクスチャの設定
    renderer.setTexture(m_texture);

    // 描画
    renderer.drawDecal(transform.toMatrix(), m_mesh, m_color);
}
