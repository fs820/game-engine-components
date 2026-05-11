//--------------------------------------------
//
// シーン [scene.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "object.h"

class Scene;
class Renderer;
class Camera;
class Application;

//---------------------------------------------
// シーン管理クラス
//---------------------------------------------
class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

    void addScene(std::string_view sceneName, Scene* scene);
    void changeScene(std::string_view sceneName);
    void update(float elapsedTime, float deltaTime);
    void lateUpdate(float elapsedTime, float deltaTime);
    void cleanup();

    Scene* getActiveScene() const { return m_activeScene; }

private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene = nullptr;
};

//---------------------------------------------
// シーンクラス
//---------------------------------------------
class Scene
{
public:
    Scene(Application* pApp);
    virtual ~Scene();

    virtual void onEnter() {}                                  // シーンに入るときの処理
    virtual void onExit() {}                                   // シーンから出るときの処理

    void update(float elapsedTime, float deltaTime);     // 更新処理
    void lateUpdate(float elapsedTime, float deltaTime); // 更新処理
    void cleanup();

    void addGameObject(std::unique_ptr<GameObject> gameObject);

    //-------------------------------------------------------------------
    // ゲームオブジェクトのうち、指定したコンポーネントを持つものを取得
    //-------------------------------------------------------------------
    template<typename T>
    std::vector<T*> getGameObjectsOfType() const
    {
        std::vector<T*> result;
        for (const auto& gameObject : m_gameObjects)
        {
            if (gameObject->has<T>())
            {
                auto comps = gameObject->get<T>();
                result.insert(result.end(), comps.begin(), comps.end());
            }
        }
        return result;
    }

protected:
    virtual void onUpdate(float, float) {}
    Application* getApp() { return m_pApp; }

private:
    Application* m_pApp;

    std::vector<std::unique_ptr<GameObject>> m_gameObjects;  // ゲームオブジェクトのリスト
    std::vector<GameObject*> m_noStartObjects;               // Startしていないゲームオブジェクトのリスト
};
