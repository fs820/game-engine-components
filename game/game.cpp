//--------------------------------------------
//
// ゲーム [game.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "game.h"
#include "gui.h"
#include "mesh.h"
#include "camera_comp.h"
#include "light_comp.h"
#include "application.h"
#include "texture.h"
#include "renderer.h"
#include "ground.h"
#include "mymath.h"
#include "binary_stream.h"
#include "player.h"
#include "model.h"
#include "decal.h"
#include "worldend.h"
#include "sky.h"
#include "board.h"
#include "input.h"
#include "physics.h"

//-----------------------------
// 
// タイトルシーン
// 
//-----------------------------

//------------------------
// シーンに入るときの処理
//------------------------
void GameScene::onEnter()
{
    auto pApp = getApp();                      // アプリケーション
    auto pRenderer = pApp->getRenderer();      // レンダラー
    auto pInput = getApp()->getInput();        // インプット
    auto pPhy = getApp()->getPhysicsManager(); // 物理

    // マウスを移動量モードにする (マウスが非表示になりウィンドウに固定される)
    pInput->setRelativeMouseMode(true);

    // アンチエイリアスとブルームを行う
    pRenderer->setPostProcessShaderMask(PostProcessShaderMask::FXAA | PostProcessShaderMask::Bloom);

    // アニメ調の色彩
    pRenderer->setToneMappingType(ToneMappingType::Anime);

    // 影の解像度
    pRenderer->setShadowMapResolution(8192);

    // 影の描画範囲
    pRenderer->setShadowMapArea(40.0f,40.0f, 0.5f, 100.0f);

    // 重力
    pPhy->setGravity({ 0,EARTH_GRAVITY,0 });

    // 環境光
    BinaryReader reader(AMBIENT_FILE);
    if (reader.isValid()) m_ambient = reader.read<Color>();
    pRenderer->setAmbient(m_ambient);

    // Fog
    FogData fog{};
    fog.color = Color::White();
    fog.start = 800.0f;
    fog.end = 1100.0f;
    fog.horizonHeight = 0.0f;
    fog.skyFogHeight = 550.0f;
    fog.fogPower = 1.0f;
    fog.skyFogPower = 3.0f;
    pRenderer->setFog(fog);

    // カメラ
    Camera camera{};
    camera.SetPivot(Camera::Pivot::Target);
    camera.SetPivotPosition({ 0.0f, 1.5f, 0.0f }, true);
    camera.SetMinRadius(1.0f);
    camera.SetMaxRadius(1000.0f);
    camera.SetRadius(5.0f);
    camera.SetTheta(math::degreesToRadians(-90.0f));
    camera.SetPhi(math::degreesToRadians(45.0f));
    camera.SetFovY(math::degreesToRadians(55.0f));
    camera.Move(0.0f, Vector2::Zero(), 0.0f);
    Vector2 screenMag{};
    pRenderer->getScreenSizeMagnification(screenMag);
    camera.SetAspectRatio(DEFAULT_SCREEN_SIZE.x * screenMag.x / DEFAULT_SCREEN_SIZE.y * screenMag.y);
    std::unique_ptr<GameObject> pCamera = std::make_unique<GameObject>();
    pCamera->add<CameraComponent>(camera);
    addGameObject(std::move(pCamera));

    // ライト
    float distance = 60.0f;
    float lightTelta = math::degreesToRadians(135.0f);
    float lightPhi = math::degreesToRadians(30.0f);
    Vector3 targetPos = { 0, 0, 0 };
    Vector3 lightDirVec = targetPos - Vector3::FromSpherical(distance, lightTelta, lightPhi);
    lightDirVec.normalize();
    LightData light{};
    light.color = Color::White();
    light.direction = { lightDirVec,0 };
    std::unique_ptr<GameObject> pLight = std::make_unique<GameObject>();
    pLight->add<LightComponent>(light, true, distance, lightTelta, lightPhi, targetPos, Vector3{ 0,1,0 });
    addGameObject(std::move(pLight));

    // 地面の生成
    auto pGround = factory::createGround(*getApp()->getMeshManager(), *getApp()->getTextureManager(), getApp()->getTextureManager()->getTextureHandle(Hash("ground")), Transform(Vector3(0, 0, 0), Quaternion::RotationYawPitchRoll(0.0f, math::degreesToRadians(90.0f), 0.0f), Vector3(100.0f, 100.0f, 1.0f)), 1000.0f);
    addGameObject(std::move(pGround));

    // 果ての生成
    auto pWorldEnd = factory::createWorldEnd(*getApp()->getMeshManager(), *getApp()->getTextureManager(), getApp()->getTextureManager()->getTextureHandle(Hash("worldend")), Transform(Vector3(0, 5, 0), Quaternion::RotationYawPitchRoll(0.0f, 0.0f, 0.0f), Vector3(100.0f, 10.0f, 100.0f)));
    addGameObject(std::move(pWorldEnd));

    // 空の生成
    auto pSky = factory::createSky(*getApp()->getMeshManager(), *getApp()->getTextureManager(), getApp()->getTextureManager()->getTextureHandle(Hash("sky")), Transform(Vector3(0, 0, 0), Quaternion::RotationYawPitchRoll(0.0f, 0.0f, 0.0f), Vector3(1000.0f, 1000.0f, 1000.0f)));
    addGameObject(std::move(pSky));

    // プレイヤーの生成
    auto pPlayer = factory::createPlayer(*getApp()->getModelManager(), *getApp()->getRenderer(), getApp()->getModelManager()->getModelHandle(Hash("player")), Transform(Vector3(0, 0, 0), Quaternion::RotationYawPitchRoll(0.0f, 0.0f, 0.0f), Vector3(1, 1, 1)), 100.0f);
    addGameObject(std::move(pPlayer));

    // デカールの生成
    auto pDecal = factory::createDecal(*getApp()->getMeshManager(), getApp()->getTextureManager()->getTextureHandle(Hash("decal")), Transform(Vector3(2, 0, 0), Quaternion::RotationYawPitchRoll(0.0f, 0.0f, 0.0f), Vector3(0.8f, 2.0f, 0.8f)));
    addGameObject(std::move(pDecal));

    // ボードの生成
    auto cameraComp = getGameObjectsOfType<CameraComponent>();
    auto pBoard = factory::createBoard(*getApp()->getMeshManager(), cameraComp[0]->get(), getApp()->getTextureManager()->getTextureHandle(Hash("board")), Transform(Vector3(0, 0.4f * 5.0f, 2), Quaternion::RotationYawPitchRoll(0.0f, 0.0f, 0.0f), Vector3(0.8f * 5.0f, 0.8f * 5.0f, 1.0f)));
    addGameObject(std::move(pBoard));
}

//------------------------
// シーンから出るときの処理
//------------------------
void GameScene::onExit()
{
}

//------------------------
// 更新処理
//------------------------
void GameScene::onUpdate(float elapsedTime, float deltaTime)
{
    auto pCamera = getGameObjectsOfType<CameraComponent>();
    auto& camera = pCamera[0]->get();

#ifdef _DEBUG
    auto pApp = getApp();                         // アプリケーション
    auto pRenderer = pApp->getRenderer();         // レンダラー

    auto pLight = getGameObjectsOfType<LightComponent>();
    auto light = pLight[0]->get();

    if (ImGui::Begin("Game Scene")) // ウィンドウ開始
    {
        ImGui::Text("This is the Game Scene.");
    }
    ImGui::End(); // ウィンドウ終了

    // ImGuiで角度を変える
    if (ImGui::Begin("Game Camera"))
    {
        float radius = camera.GetRadius(), minRadius = camera.GetMinRadius(), maxRadius = camera.GetMaxRadius();
        ImGui::SliderFloat("Radius", &radius, minRadius, maxRadius);
        camera.SetRadius(radius);
        float theta = math::radiansToDegrees(camera.GetTheta());
        ImGui::SliderFloat("Theta", &theta, 0.0f, 360.0f);
        camera.SetTheta(math::degreesToRadians(theta));
        float phi = math::radiansToDegrees(camera.GetPhi());
        ImGui::SliderFloat("Phi", &phi, 0.0f, 180.0f);
        camera.SetPhi(math::degreesToRadians(phi));
    }
    ImGui::End();

    // ImGuiで色を変える
    if (ImGui::Begin("Game Light"))
    {
        float radius{}, theta{}, phi{};
        pLight[0]->getShadowInfo(&radius, &theta, &phi);
        float dTheta = math::radiansToDegrees(theta);
        float dPhi = math::radiansToDegrees(phi);

        ImGui::SliderFloat("Radius", &radius, 0.0f, 100.0f);
        ImGui::SliderFloat("Theta", &dTheta, 0.0f, 360.0f);
        ImGui::SliderFloat("Phi", &dPhi, 0.0f, 180.0f);

        theta = math::degreesToRadians(dTheta);
        phi = math::degreesToRadians(dPhi);
        theta = math::normalizeTheta(theta);
        phi = math::normalizePhi(phi);

        pLight[0]->setShadowInfo(true, radius, theta, phi, Vector3::Zero(), Vector3{ 0,1,0 });

        Vector3 lightDirVec = Vector3::Zero() - Vector3::FromSpherical(radius, theta, phi);
        lightDirVec.normalize();
        light.direction = { lightDirVec,0 };
        pLight[0]->set(light);

        ImGui::ColorEdit4("Ambient", &m_ambient.r); // カラーエディット

        // ImGuiの編集が終了した
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            pRenderer->setAmbient(m_ambient);

            // 色を保存する
            BinaryWriter writer(AMBIENT_FILE);
            writer.write(m_ambient);
        }
    }
    ImGui::End();
#endif // _DEBUG

    camera.Move(deltaTime, Vector2::Zero(), 0.0f);
}
