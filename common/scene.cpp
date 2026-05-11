//--------------------------------------------
//
// シーン [scene.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "scene.h"
#include "object.h"
#include "camera.h"

//---------------------------------------------
// 
// シーン管理クラス
// 
//---------------------------------------------

//------------------
// シーンを追加
//------------------
void SceneManager::addScene(std::string_view sceneName, Scene* scene)
{
    m_scenes.try_emplace(std::string(sceneName), std::unique_ptr<Scene>(scene)); // シーンを追加
}

//------------------
// シーンを切り替える
//------------------
void SceneManager::changeScene(std::string_view sceneName)
{
    auto it = m_scenes.find(std::string(sceneName));
    if (it != m_scenes.end())
    {
        if (m_activeScene != nullptr)
        {
            m_activeScene->onExit(); // 現在のシーンから出る
        }
        m_activeScene = it->second.get();
        m_activeScene->onEnter(); // 新しいシーンに入る
    }
}

//------------------
// シーンの更新
//------------------
void SceneManager::update(float elapsedTime, float deltaTime)
{
    // シーンの更新
    if (m_activeScene != nullptr)
    {
        m_activeScene->update(elapsedTime, deltaTime);
    }
}

//------------------
// シーンの更新
//------------------
void SceneManager::lateUpdate(float elapsedTime, float deltaTime)
{
    // シーンの更新
    if (m_activeScene != nullptr)
    {
        m_activeScene->lateUpdate(elapsedTime, deltaTime);
    }
}

//------------------
// シーンの整理
//------------------
void SceneManager::cleanup()
{
    // シーンの更新
    if (m_activeScene != nullptr)
    {
        m_activeScene->cleanup();
    }
}

//---------------------------------------------
//
// シーンクラス
//
//---------------------------------------------
Scene::Scene(Application* pApp) : m_pApp(pApp) {}
Scene::~Scene() = default;

//------------------
// 更新
//------------------
void Scene::update(float elapsedTime, float deltaTime)
{
    // StartしていないゲームオブジェクトのStartを呼び出す
    for (auto& gameObject : m_noStartObjects)
    {
        if (!gameObject->start())
        {
            gameObject->markForDestroy(); // Startに失敗したゲームオブジェクトは破棄予定にする
        }
    }
    m_noStartObjects.clear(); // Startしていないゲームオブジェクトのリストをクリア

    onUpdate(elapsedTime, deltaTime);

    // ゲームオブジェクトの更新
    for (auto& gameObject : m_gameObjects)
    {
        gameObject->update(deltaTime);
    }
}

//------------------
// 更新
//------------------
void Scene::lateUpdate(float elapsedTime, float deltaTime)
{
    // 物理シミュレーションの結果を適応する
    for (auto& gameObject : m_gameObjects)
    {
        gameObject->physicsSync();
    }

    // ゲームオブジェクトの遅延更新
    for (auto& gameObject : m_gameObjects)
    {
        gameObject->lateUpdate(deltaTime);
    }
}

//------------------
// 破棄
//------------------
void Scene::cleanup()
{
    // 破棄予定のゲームオブジェクトを削除
    m_gameObjects.erase(
        std::remove_if(
            m_gameObjects.begin(),
            m_gameObjects.end(),
            [](const std::unique_ptr<GameObject>& obj)
            {
                if (obj->isMarkedForDestroy())
                {
                    obj->destroy();
                    return true;
                }
                return false;
            }),
        m_gameObjects.end());
}

//------------------
// ゲームオブジェクトを追加
//------------------
void Scene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_noStartObjects.push_back(gameObject.get());   // Startしていないゲームオブジェクトのリストに追加
    m_gameObjects.push_back(std::move(gameObject)); // ゲームオブジェクトのリストに追加
}
