//-------------------------------
//
// イベントの連絡 [event.h]
// Author: fuma sato
//
//-------------------------------
#pragma once
#include <functional>
#include <any>
#include <typeindex>
#include <unordered_map>

//-------------------------------
// イベントディスパッチャー
//-------------------------------
class EventDispatcher final
{
// 公開
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&& other) noexcept
        : m_subscribers(std::move(other.m_subscribers))
    {
        // ポインタ無効化
        other.m_subscribers.clear();
    }
    EventDispatcher& operator=(EventDispatcher&& other) noexcept
    {
        // 自分自身への代入でない
        if (this != &other)
        {
            // 既存の購読者をクリア
            m_subscribers.clear();

            // 相手からデータをコピー
            m_subscribers = std::move(other.m_subscribers);
            other.m_subscribers.clear();
        }
        return *this;
    }

    // イベントTに対するコールバックを登録する
    // コールバックの型は void(const T&)
    template<typename T>
    void Subscribe(std::function<void(const T&)> callback)
    {
        // イベントの型情報(type_index)をキーにする
        auto typeIndex = std::type_index(typeid(T));

        // std::anyを使って、型が異なるコールバックリストを保持する
        auto& subscribers = m_subscribers[typeIndex];

        // もしこの型のイベントが初めてなら、リストを新規作成
        if (!subscribers.has_value())
        {
            subscribers = std::vector<std::function<void(const T&)>>();
        }

        // any_castで具体的な型のリストにキャストし、コールバックを追加
        // ※キャストに失敗すると例外が飛ぶので、try-catchも検討の価値あり
        auto& typedSubscribers = std::any_cast<std::vector<std::function<void(const T&)>>&>(subscribers);
        typedSubscribers.push_back(callback);
    }

    // イベントTを発行する
    template<typename T>
    void Publish(const T& event) const
    {
        auto typeIndex = std::type_index(typeid(T));

        // この型のイベントに購読者がいるかチェック
        auto it = m_subscribers.find(typeIndex);
        if (it == m_subscribers.end())
        {
            return; // 誰も聞いていないなら何もしない
        }

        // any_castで具体的な型のリストにキャスト
        auto& subscribers = std::any_cast<const std::vector<std::function<void(const T&)>>&>(it->second);

        // 登録されているすべてのコールバックを呼び出す
        for (const auto& callback : subscribers)
        {
            callback(event);
        }
    }

// 非公開
private:
    // 型情報から、型消去されたコールバックのリストへのマップ
    std::unordered_map<std::type_index, std::any> m_subscribers;
};
