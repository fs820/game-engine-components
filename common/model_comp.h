//--------------------------------------------
//
// Modelコンポーネント [model_comp.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "component.h"

class Model;

//-------------------------------------
// Transformコンポーネントクラス
//-------------------------------------
class ModelComponent : public Component
{
public:
    ModelComponent(Model* pModel) : m_pModel(pModel) {}
    virtual ~ModelComponent() = default;

    void lateUpdate(float deltaTime) override;

    void set(Model* pModel);
    Model* get() const;

private:
    std::unique_ptr<Model> m_pModel; // オブジェクトの位置、回転、スケールを表すTransform
};
