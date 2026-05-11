//--------------------------------------------
//
// エントリーポイント [entry.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#define SDL_MAIN_USE_CALLBACKS 1 // エントリーポイントをSDLに任せる
#include <SDL3/SDL_main.h>       // SLDのエントリーポイントなどのコールバックを定義するためのh
#include "application.h"         // アプリケーションクラス

// 必ず実装しなければならない関数
// アプリケーションクラスを継承したゲームのmainクラスのインスタンスを生成する
extern Application* CreateApplication();

// SDLのエントリーポイント定義

//-------------------------------------------------------------------
// 初期化
//-------------------------------------------------------------------
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    // アプリを生成
    Application* app = CreateApplication();
    *appstate = app;

    // 初期化
    if (app == nullptr || !app->init(argc, argv))
    {
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

//-------------------------------------------------------------------
// 破棄
//-------------------------------------------------------------------
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if (appstate != nullptr)
    {
        // アプリを破棄
        Application* app = static_cast<Application*>(appstate);
        app->uninit();
        delete app;
        app = nullptr;
    }
}

//-------------------------------------------------------------------
// 更新
//-------------------------------------------------------------------
SDL_AppResult SDL_AppIterate(void* appstate)
{
    Application* app = static_cast<Application*>(appstate);

    // 更新
    if (!app->update())
    {
        return SDL_APP_SUCCESS;
    }

    // 描画
    return app->render() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

//-------------------------------------------------------------------
// イベント
//-------------------------------------------------------------------
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    // イベント処理
    Application* app = static_cast<Application*>(appstate);
    return app->handleEvent(event) ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}
