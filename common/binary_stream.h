//--------------------------------------------
//
// バイナリストリームクラス [binary_stream.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include "native_file.h"
#include <concepts>
#include <bit>

//---------------------------------------------------------
// 連続読み込み用リーダー
//---------------------------------------------------------
class BinaryReader
{
public:
    explicit BinaryReader(const std::filesystem::path& path) : m_file(path, FileMode::Read) {}
    ~BinaryReader() = default;

    // 型Tを一つ読み込んで返す（位置は自動で進む）
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    T read()
    {
        T data{};
        if (m_file.read(&data, sizeof(T)) == sizeof(T))
        {
            return data;
        }
        throw std::runtime_error("Failed to read data");
    }

    // 配列などをまとめて読み込む
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    void readArray(std::vector<T>& outVector, size_t count)
    {
        outVector.resize(count);
        if (m_file.read(outVector.data(), sizeof(T) * count) != sizeof(T) * count)
        {
            throw std::runtime_error("Failed to read array data");
        }
    }

    // 生データ読み込み
    void readBytes(void* dest, size_t size)
    {
        if (m_file.read(dest, size) != size)
        {
            throw std::runtime_error("Failed to read bytes");
        }
    }


    bool isValid() const { return m_file.isOpen(); }

private:
    NativeFile m_file;
};

//---------------------------------------------------------
// 連続書き込み用ライター
//---------------------------------------------------------
class BinaryWriter
{
public:
    explicit BinaryWriter(const std::filesystem::path& path, bool append = false) : m_file(path, append ? FileMode::Append : FileMode::Write) {}
    ~BinaryWriter() = default;

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    void write(const T& data)
    {
        m_file.write(&data, sizeof(T));
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    void writeArray(const std::vector<T>& data)
    {
        m_file.write(data.data(), sizeof(T) * data.size());
    }

    bool isValid() const { return m_file.isOpen(); }

private:
    NativeFile m_file;
};
