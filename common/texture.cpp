//--------------------------------------------
//
// テクスチャ [texture.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace
{
    // 拡張子の先頭のドットを取り除く
    std::string RemoveDot(const std::string& ext)
    {
        if (!ext.empty() && ext[0] == '.')
            return ext.substr(1);
        return ext;
    }

    // テクスチャの種類を調べる
    TextureType findTextureType(const std::filesystem::path& path, std::string_view typeName)
    {
        std::string name{};
        name.resize(typeName.size());
        std::transform(typeName.begin(), typeName.end(), name.begin(), ::tolower);
        if (name.empty())
        {
            name = path.has_extension() ? path.extension().string() : "";
        }
        if (name.empty())
        {
            return TextureType::None;
        }
        name = RemoveDot(name);

        if (name == "png" || name == "jpg" || name == "jpeg" || name == "bmp")
        {
            return TextureType::Wic;
        }
        else if (name == "dds")
        {
            return TextureType::Dds;
        }
        else if (name == "tga")
        {
            return TextureType::Tga;
        }
        else if (name == "raw")
        {
            return TextureType::Raw;
        }
        else
        {
            return TextureType::None;
        }
    }
}

// stbカスタムデリータ
void StbDeleter::operator()(unsigned char* p) const
{
    if (p)
    {
        stbi_image_free(p);
    }
}
TextureData::~TextureData() = default;

//----------------------------
// ファイルからテクスチャを読み込む
//----------------------------
bool TextureManager::load(unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback, uint64_t id)
{
    if (id != Hash(""))
    {// テクスチャが指定されている
        if (m_idToHandle.contains(id))
        {
            TextureHandle handle = m_idToHandle[id];
            if (handle.id < m_slots.size() && m_slots[handle.id].type != TextureType::None && m_slots[handle.id].data == nullptr)
            {// 登録されておりまだデータが読み込まれていないテクスチャ
                // 読み込む
                setData(handle.id);
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
                        if(!progressCallback("Texture", finishedCount.load(), totalCount))
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
                futures.push_back(std::async(std::launch::async, [this, cnt, &activeThreads, &finishedCount]()
                    {
                        setData(cnt);
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
                if (!progressCallback("Texture", finishedCount.load(), totalCount))
                {// キャンセルされた場合
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
            return progressCallback("Texture", finishedCount.load(), totalCount);
        }
        return true;
    }
}

//----------------------------
// ファイルからテクスチャを読み込む
//----------------------------
bool TextureManager::registerPath(uint64_t id, const std::filesystem::path& path, std::string_view typeName)
{
    std::lock_guard<std::mutex> lock(m_slotsMutex);

    // キャッシュチェック
    if (m_idToHandle.contains(id))
    {
        return false;
    }

    // スロットを作成
    TextureSlot slot{};
    slot.path = path;
    slot.type = findTextureType(path, typeName);
    if (slot.type == TextureType::None) { return false; }

    // キャッシュ登録
    TextureHandle handle(uint32_t(m_slots.size()));
    m_slots.push_back(std::move(slot));

    // ハンドル登録
    m_idToHandle.try_emplace(id);
    m_idToHandle[id] = handle;
    return true;
}

//----------------------------
// テクスチャデータを登録する
//----------------------------
bool TextureManager::registerByteData(uint64_t id, const std::filesystem::path& path, std::span<const uint8_t> data, std::string_view typeName)
{
    std::lock_guard<std::mutex> lock(m_slotsMutex);

    // キャッシュチェック
    if (m_idToHandle.contains(id))
    {
        return false;
    }

    // スロットを作成
    TextureSlot slot{};
    slot.path = path;
    slot.type = findTextureType(path, typeName);

    // テクスチャ構造体を作成
    auto textureData = std::make_shared<TextureData>();
    textureData->type = slot.type;
    if (textureData->type == TextureType::None || textureData->type == TextureType::Raw) { return false; }
    textureData->buffer.assign(data.begin(), data.end());
    textureData->isValid = true;

    // キャッシュ登録
    slot.data = textureData;
    TextureHandle handle(uint32_t(m_slots.size()));
    m_slots.push_back(std::move(slot));

    // ハンドル登録
    m_idToHandle.try_emplace(id);
    m_idToHandle[id] = handle;
    return true;
}

//----------------------------
// テクスチャデータを登録する
//----------------------------
bool TextureManager::registerRawData(uint64_t id, const std::filesystem::path& path, unsigned char* data, int width, int height)
{
    std::lock_guard<std::mutex> lock(m_slotsMutex);

    // キャッシュチェック
    if (m_idToHandle.contains(id))
    {
        return false;
    }

    // スロットを作成
    TextureSlot slot{};
    slot.path = path;
    slot.type = TextureType::Raw;

    // テクスチャ構造体を作成
    auto textureData = std::make_shared<TextureData>();
    textureData->type = slot.type;
    textureData->rawPixels.reset(data);
    textureData->width = width;
    textureData->height = height;
    textureData->isValid = true;

    // キャッシュ登録
    slot.data = textureData;
    TextureHandle handle(uint32_t(m_slots.size()));
    m_slots.push_back(std::move(slot));

    m_idToHandle.try_emplace(id);
    m_idToHandle[id] = handle;
    return true;
}

//----------------------------
// ID→ハンドル
//----------------------------
TextureHandle TextureManager::getTextureHandle(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_slotsMutex);

    if (m_idToHandle.contains(id))
    {
        return m_idToHandle[id];
    }
    return TextureHandle();
}

//----------------------------
// テクスチャデータを取得する
//----------------------------
std::weak_ptr<TextureData> TextureManager::getTextureData(const TextureHandle& handle) const
{
    if (!handle.isValid() || handle.id >= m_slots.size())
    {
        return std::weak_ptr<TextureData>{};
    }
    return m_slots[handle.id].data;
}

//----------------------------
// テクスチャデータを取得する
//----------------------------
std::weak_ptr<TextureData> TextureManager::getTextureData(size_t index) const
{
    if (index >= m_slots.size())
    {
        return std::weak_ptr<TextureData>{};
    }
    return m_slots[index].data;
}

//----------------------------
// テクスチャのサイズを取得する
//----------------------------
void TextureManager::getTextureSize(const TextureHandle& handle, int& width, int& height) const
{
    width = 0;
    height = 0;
    if (!handle.isValid() || handle.id >= m_slots.size()) return;
    auto& data = m_slots[handle.id].data;
    if (data != nullptr)
    {
        width = data->width;
        height = data->height;
    }
}

//----------------------------
// CPUリソースを解放する
//----------------------------
void TextureManager::releaseCpuResources()
{
    for (auto& slot : m_slots)
    {
        if (slot.data != nullptr)
        {
            if (!slot.data->buffer.empty())
            {
                slot.data->buffer.clear();
                slot.data->buffer.shrink_to_fit();
            }
            if (slot.data->rawPixels != nullptr)
            {
                slot.data->rawPixels.reset();
            }
        }
    }
}

//----------------------------
// 指定したテクスチャのCPUリソースを解放する
//----------------------------
void TextureManager::releaseCpuResource(uint64_t id)
{
    if (m_idToHandle.contains(id))
    {
        TextureHandle handle = m_idToHandle[id];
        if (!handle.isValid() || handle.id >= m_slots.size()) return;
        auto& slot = m_slots[handle.id];
        if (slot.data != nullptr)
        {
            if (!slot.data->buffer.empty())
            {
                slot.data->buffer.clear();
                slot.data->buffer.shrink_to_fit();
            }
            if (slot.data->rawPixels != nullptr)
            {
                slot.data->rawPixels.reset();
            }
        }
    }
}

//----------------------------
// slotのデータをセットする
//----------------------------
void TextureManager::setData(size_t index)
{
    // テクスチャ構造体を作成
   std::shared_ptr<TextureData> data = std::make_shared<TextureData>();
    std::filesystem::path filePath{};

    {// m_slotsは同時に触らない
        std::lock_guard<std::mutex> lock(m_slotsMutex);
        data->type = m_slots[index].type;
        filePath = m_slots[index].path;
    }

    switch (data->type)
    {
    case TextureType::Raw:
    {
        // 生データ取り出し
        int width, height;
        data->rawPixels.reset((stbi_load(ToUtf8String(filePath).c_str(), &width, &height, nullptr, 4)));
        data->width = width;
        data->height = height;
        data->isValid = true;
        break;
    }
    case TextureType::Wic:
    {
        // テクスチャデータを読み込む
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            data->buffer.resize(size);
            if (file.read((char*)data->buffer.data(), size))
            {
                data->isValid = true;
            }
        }
        break;
    }
    case TextureType::Dds:
    {
        // テクスチャデータを読み込む
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            data->buffer.resize(size);
            if (file.read((char*)data->buffer.data(), size))
            {
                data->isValid = true;
            }
        }
        break;
    }
    case TextureType::Tga:
    {
        // テクスチャデータを読み込む
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            data->buffer.resize(size);
            if (file.read((char*)data->buffer.data(), size))
            {
                data->isValid = true;
            }
        }
        break;
    }
    }

    // slotに入れる
    {// m_slotsは同時に触らない
        std::lock_guard<std::mutex> lock(m_slotsMutex);
        m_slots[index].data = data;
    }
}
