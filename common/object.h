//-------------------------------------
//
//　ゲームオブジェクト [object.h]
// Author: Fuma Sato
//
//-------------------------------------
#pragma once
#include <stdexcept>
#include <memory>
#include <vector>
#include "trans_comp.h"

//---------------------------------
// ゲームオブジェクトクラス
//---------------------------------
class GameObject
{
public:
    GameObject(const Transform& transform = Transform::Zero())
        : m_isMarkedForDestroy{} { m_transform = std::make_unique<TransformComponent>(transform); }
    ~GameObject();

    GameObject(GameObject&&) noexcept = default;
    GameObject& operator=(GameObject&&) noexcept = default;
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    bool start();
    void update(float deltaTime);
    void physicsSync();
    void lateUpdate(float deltaTime);
    void destroy();
    void markForDestroy() { m_isMarkedForDestroy = true; }
    bool isMarkedForDestroy() const { return m_isMarkedForDestroy; }

    TransformComponent* getTransform() { return m_transform.get(); }

    //-----------------------
    // コンポーネントの追加
    //-----------------------
    template<typename T, typename... Args>
        requires std::derived_from<T, Component>
    T* add(Args&&... args)
    {
        // TransformComponentは追加できない(Defaultの1つのみ)
        static_assert(!std::is_same_v<T, TransformComponent>, "TransformComponentは追加できません!");

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->setOwner(this);
        component->awake();

        T* ref = component.get();
        m_components.push_back(std::move(component));
        return ref;
    }

    //-----------------------
    // コンポーネントの取得
    //-----------------------
    template<typename T>
        requires std::derived_from<T, Component>
    bool has() const
    {
        for (auto& component : m_components)
        {
            if (dynamic_cast<T*>(component.get()) != nullptr)
                return true;
        }
        return false;
    }

    //-----------------------
    // コンポーネントの取得
    //-----------------------
    template<typename T>
        requires std::derived_from<T, Component>
    std::vector<T*> get()
    {
        // TransformComponentは専用の関数を使う
        static_assert(!std::is_same_v<T, TransformComponent>, "TransformComponentは専用の関数を使って取得してください!");

        std::vector<T*> components;
        for (auto& component : m_components)
        {
            auto pComp = dynamic_cast<T*>(component.get());
            if (pComp != nullptr)
                components.push_back(pComp);
        }
        return components;
    }

private:
    std::unique_ptr<TransformComponent> m_transform;      // 座標変換
    std::vector<std::unique_ptr<Component>> m_components; // コンポーネント
    bool m_isMarkedForDestroy;                            // 破棄予定フラグ
};
