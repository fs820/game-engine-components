//--------------------------------------------
//
// Model描画用コンポーネント [render_mesh.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "render.h"

class Model;

//-------------------------------------
// Model描画用コンポーネントクラス
//-------------------------------------
class ModelRenderComponent : public RenderComponent
{
public:
    ModelRenderComponent(const RenderQueueMask& renderQueueMask, RasMode mode, Model* pModel, OutlineData outline = {})
        : RenderComponent(renderQueueMask, mode), m_pModel(pModel),m_outline(outline) {}
    ~ModelRenderComponent() = default;

    void render(Renderer& renderer) override;

    void setModel(Model* pModel) { m_pModel = pModel; }
    void setOutline(const OutlineData& outline) { m_outline = outline; }

private:
    Model* m_pModel;       // 描画するモデルのポインタ
    OutlineData m_outline; // アウトライン
};
