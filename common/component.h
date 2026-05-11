//-------------------------------------
//
//　コンポーネントの基底 [component.h]
// Author: Fuma Sato
//
//-------------------------------------
#pragma once
#include <atomic>
#include <cstdint>

class Renderer;
class GameObject;

//---------------------------------
// コンポーネントの基底クラス
//---------------------------------
class Component
{
public:
    Component() : m_owner{}, m_id{} { m_id = ++s_nextId; }
    virtual ~Component() = default;

    virtual bool awake() { return true; }
    virtual bool start() { return true; }
    virtual void update(float deltaTime) {}
    virtual void physicsSync() {}
    virtual void lateUpdate(float deltaTime) {}
    virtual void render(Renderer& renderer) {}
    virtual void destroy() {}

    void setOwner(GameObject* owner) { m_owner = owner; }

protected:
        GameObject& getOwner() { return *m_owner; }
        const GameObject& getOwner() const { return *m_owner; }
        uint64_t getID() const { return m_id; }

private:
    static std::atomic<uint64_t> s_nextId;

     GameObject* m_owner;
     uint64_t m_id;
};
