//--------------------------------------------
//
// サウンド [sound.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "sound.h"

#include <fmod.hpp>
#include <fmod_errors.h>

namespace
{
    // エラーチェック用関数
    void CheckError(FMOD_RESULT result)
    {
        if (result != FMOD_OK)
        {
            std::cerr << "FMOD Error! (" << result << ") " << FMOD_ErrorString(result) << std::endl;
        }
    }
}

// カスタムデリータ
void SystemDeleter::operator()(FMOD::System* p) const
{
    if (p != nullptr)
    {
        p->close();
        p->release();
    }
}

//----------------------------
// サウンドリソース
//----------------------------
class SoundResource
{
public:
    SoundResource(const std::filesystem::path& path);
    ~SoundResource();

    bool load(std::weak_ptr<FMOD::System> system, SoundType type, SoundTypeEX exType);
    void unload();

    std::weak_ptr<FMOD::Sound> getSoundData() const { return m_sound; }

private:
    // ファイルパス
    const std::filesystem::path m_path;

    // サウンドデータ
    std::shared_ptr<FMOD::Sound> m_sound;
};

SoundResource::SoundResource(const std::filesystem::path& path) : m_path(path), m_sound{} {}
SoundResource::~SoundResource() { unload(); }

//--------------
// サウンドを読み込む関数
//--------------
bool SoundResource::load(std::weak_ptr<FMOD::System> system, SoundType type, SoundTypeEX exType)
{
    FMOD_RESULT result{};

    // 音声ファイルの読み込み
    if (const auto& stSystem = system.lock())
    {
        FMOD::Sound* sound{};

        FMOD_CREATESOUNDEXINFO info{};
        switch (type)
        {
        case SoundType::BGM:
            switch (exType)
            {
            case SoundTypeEX::Sound_2D:
                result = stSystem->createStream(ToUtf8String(m_path).c_str(), FMOD_2D, nullptr, &sound);
                break;
            case SoundTypeEX::Sound_3D:
                result = stSystem->createStream(ToUtf8String(m_path).c_str(), FMOD_3D, nullptr, &sound);
                break;
            }
            break;
        case SoundType::SE:
            switch (exType)
            {
            case SoundTypeEX::Sound_2D:
                result = stSystem->createSound(ToUtf8String(m_path).c_str(), FMOD_2D, nullptr, &sound);
                break;
            case SoundTypeEX::Sound_3D:
                result = stSystem->createSound(ToUtf8String(m_path).c_str(), FMOD_3D, nullptr, &sound);
                break;
            }
            break;
        }
        CheckError(result);

        // カスタムデリータ
        m_sound.reset(sound, [](FMOD::Sound* p) { if (p) p->release(); });
    }

    return true;
}

//--------------
// サウンドを解放する関数
//--------------
void SoundResource::unload()
{
    m_sound.reset();
}

//----------------------------
// サウンドクラス
//----------------------------
Sound::Sound(SoundManager& soundManager, const SoundHandle& handle) : m_pSoundManager(&soundManager), m_handle(handle), m_pChannel{}, m_isPlaying{} {}

//---------------------------------------------------------
// ムーブコンストラクタ
//---------------------------------------------------------
Sound::Sound(Sound&& other) noexcept
    : m_pSoundManager(other.m_pSoundManager)
    , m_handle(other.m_handle)
    , m_pChannel(other.m_pChannel)
    , m_isPlaying(other.m_isPlaying)
{
    // ポインタ無効化
    other.m_pChannel = nullptr;
    other.m_isPlaying = false;
}

//---------------------------------------------------------
// ムーブ代入演算子
//---------------------------------------------------------
Sound& Sound::operator=(Sound&& other) noexcept
{
    // 自分自身への代入でない
    if (this != &other)
    {
        // 自分が今再生している音があれば
        stop();

        // 相手からデータをコピー
        m_pSoundManager = other.m_pSoundManager;
        m_handle = other.m_handle;
        m_pChannel = other.m_pChannel;
        m_isPlaying = other.m_isPlaying;

        // ポインタ無効化
        other.m_pChannel = nullptr;
        other.m_isPlaying = false;
    }
    return *this;
}

//--------------
// 再生
//--------------
void Sound::play()
{
    FMOD_RESULT result;
    const auto& system = m_pSoundManager->getSystem();
    if (const auto& stSystem = system.lock())
    {
        const auto& resource = m_pSoundManager->getSoundData(m_handle);
        if (const auto& stResource = resource.lock())
        {
            // 再生
            FMOD::Channel* channel{};
            const auto& data = stResource->getSoundData();
            if (const auto& stData = data.lock())
            {
                result = stSystem->playSound(stData.get(), nullptr, false, &channel);
                CheckError(result);

                m_pChannel = channel;
                m_isPlaying = true;
                std::cout << "Playing sound..." << std::endl;
            }
        }
    }
}

//--------------
// 停止
//--------------
void Sound::stop()
{
    if (m_pChannel != nullptr)
    {
        // チャンネルが有効か確認して止める
        bool isPlaying = false;
        m_pChannel->isPlaying(&isPlaying);

        if (isPlaying)
        {
            m_pChannel->stop();
        }

        m_pChannel = nullptr;
        m_isPlaying = false;
    }
}

//--------------
// 一時停止
//--------------
void Sound::pause(bool isPaused)
{
    if (m_pChannel != nullptr)
    {
        m_pChannel->setPaused(isPaused);
    }
}

//--------------
// 再生確認
//--------------
bool Sound::isPlaying() const
{
    if (m_pChannel)
    {
        bool playing = false;
        m_pChannel->isPlaying(&playing);
        return playing;
    }
    return false;
}

//--------------
// ループ
//--------------
void Sound::setLoop(bool isLoop, int loopCount)
{
    if (m_pChannel != nullptr)
    {
        m_pChannel->setMode(isLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        if (isLoop)
        {
            m_pChannel->setLoopCount(loopCount);
        }
    }
}

//--------------
// 音量
//--------------
void Sound::setVolume(float volume)
{
    if (m_pChannel != nullptr)
    {
        m_pChannel->setVolume(volume);
    }
}

//--------------
// 速さ
//--------------
void Sound::setPitch(float pitch)
{
    if (m_pChannel != nullptr)
    {
        m_pChannel->setPitch(pitch);
    }
}

//--------------
// 左右
//--------------
void Sound::setPan(float pan)
{
    if (m_pChannel != nullptr)
    {
        m_pChannel->setPan(pan);
    }
}

//--------------
// 発生位置
//--------------
void Sound::set3DPosition(Vector3 pos, Vector3 vel)
{
    if (m_pChannel)
    {
        FMOD_VECTOR fPos = { pos.x, pos.y, pos.z };
        FMOD_VECTOR fVel = { vel.x, vel.y, vel.z };
        m_pChannel->set3DAttributes(&fPos, &fVel);
    }
}

//----------------------------
// サウンドマネージャークラス
//----------------------------

//--------------
// サウンドを読み込む関数
//--------------
bool SoundManager::load(unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback, uint64_t id)
{
    // Systemの生成と初期化
    if (m_pSystem == nullptr)
    {
        FMOD::System* sys = nullptr;
        // 生成
        FMOD_RESULT result = FMOD::System_Create(&sys);
        CheckError(result);

        // 初期化
        // 512チャンネル, 通常モード
        result = sys->init(512, FMOD_INIT_NORMAL, nullptr);
        CheckError(result);

        // 格納
        m_pSystem.reset(sys, SystemDeleter{});
    }

    if (id != Hash(""))
    {// テクスチャが指定されている
        if (m_idToHandle.contains(id))
        {
            SoundHandle handle = m_idToHandle[id];
            if (handle.id < m_slots.size() && m_slots[handle.id].data == nullptr)
            {// 登録されておりまだデータが読み込まれていないテクスチャ
                // 読み込む
                std::shared_ptr<SoundResource> data = std::make_shared<SoundResource>(m_slots[handle.id].path);
                data->load(m_pSystem, m_slots[handle.id].type, m_slots[handle.id].exType);
                m_slots[handle.id].data = data;
            }
        }
        return true;
    }
    else
    {// 指定なし (登録テクスチャを全て読み込む)

        // 読み込み対象の数をカウント
        int totalCount = 0;
        for (const auto& slot : m_slots)
        {
            if (slot.data == nullptr) totalCount++;
        }
        if (totalCount == 0) return true; // 読み込むものがなければ終了

        std::vector<std::future<void>> futures{};
        std::atomic<int> finishedCount{ 0 }; // 完了した数
        std::atomic<int> activeThreads{ 0 };

        // メモリ確保
        futures.reserve(totalCount);

        for (size_t cnt = 0; cnt < m_slots.size(); cnt++)
        {
            if (m_slots[cnt].data == nullptr)
            {// 読み込まれていない
                // 空きが出るまで待つ
                while (activeThreads.load() >= (int)maxThread)
                {
                    // コールバックがあれば呼び出す
                    if (progressCallback != nullptr)
                    {
                        if (!progressCallback("Model", finishedCount.load(), totalCount))
                        {// キャンセルされた
                            for (auto& future : futures)
                            {
                                if (future.valid()) future.get();
                            }
                            return false;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                // 実行数
                ++activeThreads;

                // 読み込む
                std::filesystem::path path = m_slots[cnt].path;
                SoundType type = m_slots[cnt].type;
                SoundTypeEX exType = m_slots[cnt].exType;
                futures.push_back(std::async(std::launch::async, [this, cnt, path, type, exType, &activeThreads, &finishedCount]()
                    {
                        // 読み込む
                        std::shared_ptr<SoundResource> data = std::make_shared<SoundResource>(path);
                        data->load(m_pSystem, type, exType);

                        {// m_slotsは同時に触らない
                            std::lock_guard<std::mutex> lock(m_slotsMutex);
                            m_slots[cnt].data = data;
                        }

                        --activeThreads;
                        ++finishedCount;
                    }));
            }
        }

        // 残りのタスクが終わるのを待つ
        while (finishedCount.load() < totalCount)
        {
            if (progressCallback != nullptr)
            {
                if (!progressCallback("Model", finishedCount.load(), totalCount))
                {
                    // キャンセルされた場合
                    for (auto& future : futures)
                    {
                        if (future.valid()) future.get();
                    }
                    return false;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
        }

        // この関数はすべて読み込み終わってから終わります
        for (auto& future : futures)
        {
            if (future.valid()) future.get();
        }
        if (progressCallback != nullptr)
        {
            return progressCallback("Model", finishedCount.load(), totalCount);
        }
        return true;
    }
}

//----------------------------------
// サウンドを登録する
//----------------------------------
bool SoundManager::registerPath(uint64_t id, const std::filesystem::path& path, SoundType type, SoundTypeEX exType)
{
    // キャッシュチェック
    if (m_idToHandle.contains(id))
    {
        // 既に読み込まれている
        return false;
    }

    // スロットに登録
    SoundSlot slot{};
    slot.path = path;
    slot.type = type;
    slot.exType = exType;
    m_slots.push_back(slot);

    // ハンドルを登録
    m_idToHandle.try_emplace(id);
    m_idToHandle[id] = SoundHandle{ (unsigned int)(m_slots.size() - 1) };

    return true;
}

//----------------------------------
// 更新
//----------------------------------
void SoundManager::update()
{
    if (m_pSystem != nullptr)
    {
        m_pSystem->update();
    }
}

//----------------------------------
// 耳の位置 (基本カメラ位置)
//----------------------------------
void SoundManager::set3DListener(Vector3 pos, Vector3 forward, Vector3 vel, Vector3 up, int id)
{
    if (m_pSystem != nullptr)
    {
        // リスナーの位置を設定
        FMOD_VECTOR fPos = { pos.x, pos.y, pos.z };
        FMOD_VECTOR fVel = { 0.0f, 0.0f, 0.0f };
        FMOD_VECTOR fForward = { forward.x, forward.y, forward.z };
        FMOD_VECTOR fUp = { up.x, up.y, up.z };

        m_pSystem->set3DListenerAttributes(id, &fPos, &fVel, &fForward, &fUp);
    }
}

//--------------
// リソースを解放する関数
//--------------
void SoundManager::releaseResource(const SoundHandle& handle)
{
    if (m_slots.size() > handle.id)
    {
        auto data = m_slots[handle.id];
        data.data->unload();
        data.data.reset();
    }
}

//--------------
// リソースを解放する関数
//--------------
void SoundManager::releaseResources()
{
    for (auto& slot : m_slots)
    {
        if (slot.data != nullptr)
        {
            slot.data->unload();
            slot.data.reset();
        }
    }
}

//--------------
// モデルハンドルを取得する関数
//--------------
SoundHandle SoundManager::getSoundHandle(uint64_t id)
{
    if (m_idToHandle.contains(id))
    {
        return m_idToHandle[id];
    }
    return SoundHandle();
}

//--------------
// モデルデータを取得する関数
//--------------
std::weak_ptr<SoundResource> SoundManager::getSoundData(const SoundHandle& handle) const
{
    if (m_slots.size() > handle.id)
    {
        auto data = m_slots[handle.id];
        return data.data;
    }
    else
    {
        return std::weak_ptr<SoundResource>{};
    }
}
