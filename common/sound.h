//--------------------------------------------
//
// サウンド [sound.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include"math_types.h"

// 前方宣言
class SoundManager;
class SoundResource;
namespace FMOD
{
    class System;
    class Sound;
    class Channel;
}

constexpr size_t INVALID_SOUND_ID = ~0u;     // 無効値

enum class SoundType { BGM, SE };
enum class SoundTypeEX { Sound_2D, Sound_3D };

// stbカスタムデリータ
struct SystemDeleter
{
    void operator()(FMOD::System* p) const;
};

struct SoundHandle
{
    uint32_t id;

    bool isValid() const { return id != INVALID_SOUND_ID; }

    SoundHandle() : id{ INVALID_SOUND_ID } {}
    explicit SoundHandle(uint32_t handleId) : id{ handleId } {}
    ~SoundHandle() = default;
};

// サウンドスロット構造体
struct SoundSlot
{
    std::filesystem::path path;          // パス
    std::shared_ptr<SoundResource> data; // リソース
    SoundType type;     // Se.Bgm
    SoundTypeEX exType; // 2D,3D

    SoundSlot() : path{}, data{}, type{}, exType{} {}
    ~SoundSlot() = default;
};

//----------------------------
// サウンドの動的インスタンス
//----------------------------
class Sound
{
public:
    Sound(SoundManager& soundManager, const SoundHandle& handle);
    ~Sound() { stop(); }

    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
    Sound(Sound&&) noexcept;
    Sound& operator=(Sound&&) noexcept;

    void play();
    void stop();
    void pause(bool isPaused);
    bool isPlaying() const;

    void setLoop(bool isLoop, int loopCount = -1);
    void setVolume(float volume); // 0.0f (無音) ～ 1.0f (最大)
    void setPitch(float pitch);   // 1.0f が通常。0.5fで低い、2.0fで高い
    void setPan(float pan);       // -1.0f (左) ～ 1.0f (右)

    void set3DPosition(Vector3 pos, Vector3 vel = { 0,0,0 });

private:
    SoundManager* m_pSoundManager; // サウンドマネージャー参照

    SoundHandle m_handle;   // 静的なサウンドリソースのハンドル

    FMOD::Channel* m_pChannel; // チャンネル

    bool m_isPlaying; // 再生中
};

//----------------------------
// モデルマネージャー
//----------------------------
class SoundManager
{
public:
    SoundManager() : m_pSystem{}, m_slotsMutex{}, m_slots{}, m_idToHandle{} {}
    ~SoundManager() = default;

    bool load(unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback = {}, uint64_t id = Hash(""));
    bool registerPath(uint64_t id, const std::filesystem::path& path, SoundType type = SoundType::BGM, SoundTypeEX exType = SoundTypeEX::Sound_2D);

    void update();

    void set3DListener(Vector3 pos, Vector3 forward, Vector3 vel = { 0,0,0 }, Vector3 up = { 0,1,0 }, int id = 0);

    void releaseResources();
    void releaseResource(const SoundHandle& handle);

    SoundHandle getSoundHandle(uint64_t id);
    std::weak_ptr<FMOD::System> getSystem() const { return m_pSystem; }
    std::weak_ptr<SoundResource> getSoundData(const SoundHandle& handle) const;

private:
    std::shared_ptr<FMOD::System> m_pSystem; // FMODの核

    std::mutex m_slotsMutex;                                // ↓のmutex
    std::vector<SoundSlot> m_slots;                         // モデルスロット
    std::unordered_map<uint64_t, SoundHandle> m_idToHandle; // ID -> ハンドルのマップ
};
