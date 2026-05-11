//--------------------------------------------
//
// タイトル [title.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "title.h"
#include "gui.h"
#include "title_rogo.h"
#include "mesh.h"
#include "application.h"
#include "texture.h"
#include "renderer.h"
#include "render_string.h"
#include "input.h"

//-----------------------------
// 
// タイトルシーン
// 
//-----------------------------

//------------------------
// シーンに入るときの処理
//------------------------
void TitleScene::onEnter()
{
    auto pRederer = getApp()->getRenderer(); // レンダラー
    auto pInput = getApp()->getInput();      // インプット

    // マウスを通常モードにする (マウスが表示される)
    pInput->setRelativeMouseMode(false);

    // アンチエイリアスとブルームを行う
    pRederer->setPostProcessShaderMask(PostProcessShaderMask::FXAA | PostProcessShaderMask::Bloom);

    // アニメ調の色彩
    pRederer->setToneMappingType(ToneMappingType::Anime);

    // 影の描画範囲
    pRederer->setShadowMapArea(40.0f, 40.0f, 0.5f, 100.0f);

    // 環境光
    pRederer->setAmbient(Color(0.2f,0.2f,0.2f,1.0f));

    // Fog
    FogData fog{};
    fog.color = Color::White();
    fog.start = 800.0f;
    fog.end = 1100.0f;
    fog.horizonHeight = 0.0f;
    fog.skyFogHeight = 550.0f;
    fog.fogPower = 1.0f;
    fog.skyFogPower = 3.0f;
    pRederer->setFog(fog);

    // ロゴの生成
    auto pLogo = factory::createTitleLogo(*getApp()->getMeshManager(), getApp()->getTextureManager()->getTextureHandle(Hash("logo")), Transform(Vector3(960, 540, 0), Quaternion::Identity(), Vector3(1344, 1050, 0)));
    addGameObject(std::move(pLogo));

    // 文字列生成
    auto pString = std::make_unique<GameObject>();
    pString->add<StringRenderComponent>("タイトル", Vector2{ 100,100 });
    addGameObject(std::move(pString));
}

//------------------------
// シーンから出るときの処理
//------------------------
void TitleScene::onExit()
{
}

//------------------------
// 更新処理
//------------------------
void TitleScene::onUpdate(float elapsedTime, float deltaTime)
{
#ifdef _DEBUG
    if (ImGui::Begin("Title Scene")) // ウィンドウ開始
    {
        ImGui::Text("This is the Title Scene.");
    }
    ImGui::End(); // ウィンドウ終了
#endif // _DEBUG
}
