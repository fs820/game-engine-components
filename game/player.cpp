//--------------------------------------------
//
// プレイヤー [player.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "player.h"
#include "object.h"
#include "trans_comp.h"
#include "model_comp.h"
#include "render_model.h"
#include "model.h"
#include "texture.h"

namespace factory
{
    std::unique_ptr<GameObject> createPlayer(ModelManager& modelManager, Renderer& renderer, ModelHandle model, Transform transform, float offsetModelScaale)
    {
        std::unique_ptr<GameObject> player = std::make_unique<GameObject>(transform);

        // モデル
        Model* pModel = new Model(modelManager, renderer, model);
        pModel->init();
        pModel->setScale(offsetModelScaale);
        pModel->setPixelShaderType(PixelShaderType::Toon);
        pModel->setAnimation();
        auto pModelComp = player->add<ModelComponent>(pModel);

        OutlineData outline = OutlineData{ Color::Black(),0.001f };
        player->add<ModelRenderComponent>(RenderQueueMask::Shadow | RenderQueueMask::Geometry | RenderQueueMask::Outline, RasMode::None, pModelComp->get(), outline);
        return player;
    }
}
