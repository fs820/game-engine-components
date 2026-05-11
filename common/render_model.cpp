//--------------------------------------------
//
// Mesh描画用コンポーネント [render_mesh.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "render_model.h"
#include "renderer.h"
#include "object.h"
#include "trans_comp.h"
#include "log.h"
#include "model.h"

//-------------------------------------
// 
// Mesh描画用コンポーネントクラス
// 
//-------------------------------------

//----------------------------
// 描画
//----------------------------
void ModelRenderComponent::render(Renderer& renderer)
{
    // ワールド変換の設定
    Transform transform{};
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    transform = trans->get();
    renderer.setTransformWorld(transform.toMatrix());

    // アウトライン
    renderer.setOutlineData(m_outline);

    // 描画
    m_pModel->draw();
}
