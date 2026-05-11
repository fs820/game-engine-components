//--------------------------------------------
//
// Modelコンポーネント [model_comp.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "model_comp.h"
#include "model.h"
#include "object.h"
#include "trans_comp.h"
#include "log.h"

//--------------------------------------------
//
// Modelコンポーネントクラス
//
//--------------------------------------------

//---------------------------
// 更新
//---------------------------
void ModelComponent::lateUpdate(float deltaTime)
{
    // ワールド変換の設定
    Transform transform{};
    auto& owner = getOwner();
    auto trans = owner.getTransform();
    transform = trans->get();

    m_pModel->update(deltaTime, transform.toMatrix());
}

void ModelComponent::set(Model* pModel) { m_pModel.reset(pModel); }
Model* ModelComponent::get() const { return m_pModel.get(); }
