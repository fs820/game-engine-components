//--------------------------------------------
//
// タイトル [title.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "scene.h"

//---------------------------------------------
// タイトルシーンクラス
//---------------------------------------------
class TitleScene : public Scene
{
public:
    TitleScene(Application* pApp) : Scene(pApp) {}
    virtual ~TitleScene() override = default;

    void onEnter() override;
    void onExit() override;
    void onUpdate(float elapsedTime, float deltaTime) override;

private:
};
