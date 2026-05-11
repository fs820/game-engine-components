//--------------------------------------------
//
// テクスチャ [texture.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <filesystem>
#include <mutex>
#include <functional>
#include <span>
#include "pch.h"
#include "graphics_types.h"

// stbカスタムデリータ
struct StbDeleter
{
    void operator()(unsigned char* p) const;
};

// テクスチャデータ保持用構造体
struct TextureData
{
    TextureType type;                                      // 種類
    std::vector<uint8_t> buffer;                           // ファイルの中身そのもの (png, jpgヘッダ含む)
    int width;                                             // 幅
    int height;                                            // 高さ
    std::unique_ptr<unsigned char, StbDeleter> rawPixels;  // RGBAポインタ (type==Raw時のみ)
    bool isValid;                                          // データが有効かどうか

    TextureData() : type{}, buffer{}, isValid(false), rawPixels{}, width{}, height{} {}
    ~TextureData();
};

// テクスチャスロット構造体
struct TextureSlot
{
    std::filesystem::path path;        // パス
    TextureType type;                  // 種類
    std::shared_ptr<TextureData> data; // データ

    TextureSlot() : type{}, data{}, path{} {}
    ~TextureSlot() = default;
};

//----------------------------
// テクスチャマネージャー
//----------------------------
class TextureManager
{
public:
    TextureManager() : m_slotsMutex{}, m_slots{}, m_idToHandle{} {}
    ~TextureManager() = default;

    bool load(unsigned int maxThread, std::function<bool(std::string_view, int, int)> progressCallback = {}, uint64_t id = Hash(""));

    bool registerPath(uint64_t id, const std::filesystem::path& path, std::string_view typeName = std::string_view());
    bool registerByteData(uint64_t id, const std::filesystem::path& path, std::span<const uint8_t> data, std::string_view typeName = std::string_view());
    bool registerRawData(uint64_t id, const std::filesystem::path& path, unsigned char* data, int width, int height);

    TextureHandle getTextureHandle(uint64_t id);

    std::weak_ptr<TextureData> getTextureData(const TextureHandle& handle) const;
    void getTextureSize(const TextureHandle& handle, int& width, int& height) const;
    size_t getTextureCount() const { return m_slots.size(); }

    std::weak_ptr<TextureData> getTextureData(size_t index) const;

    void releaseCpuResources();
    void releaseCpuResource(uint64_t id);

private:
    void setData(size_t index);

    std::mutex m_slotsMutex;                                   // ↓のmutex
    std::vector<TextureSlot> m_slots;                          // テクスチャスロットリスト
    std::unordered_map<uint64_t, TextureHandle> m_idToHandle;  // ID -> ハンドルのマップ
};
