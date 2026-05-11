//-------------------------------------
//
//　ゲームオブジェクト [object.cpp]
// Author: Fuma Sato
//
//-------------------------------------
#include "object.h"
#include "component.h"

//---------------------------------
// ゲームオブジェクトクラス
//---------------------------------
GameObject::~GameObject() { destroy(); }

//-----------------------
// コンポーネントの開始
//-----------------------
bool GameObject::start()
{
    for (auto& component : m_components)
    {
        if (!component->start())
        {
            return false;
        }
    }
    return true;
}

//-----------------------
// コンポーネントの更新
//-----------------------
void GameObject::update(float deltaTime)
{
    for (auto& component : m_components)
    {
        component->update(deltaTime);
    }
}

//-----------------------
// コンポーネントの更新
//-----------------------
void GameObject::physicsSync()
{
    for (auto& component : m_components)
    {
        component->physicsSync();
    }
}

//-----------------------
// コンポーネントの更新
//-----------------------
void GameObject::lateUpdate(float deltaTime)
{
    for (auto& component : m_components)
    {
        component->lateUpdate(deltaTime);
    }
}

//-----------------------
// コンポーネントの破棄
//-----------------------
void GameObject::destroy()
{
    for (auto& component : m_components)
    {
        component->destroy();
    }
    m_components.clear();
}
