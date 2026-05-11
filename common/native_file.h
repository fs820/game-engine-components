//--------------------------------------------
//
// ファイル根底 [native_file.h]
// Author: Fuma Sato
//
//--------------------------------------------
#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

// ファイルオープンのモード
enum class FileMode : unsigned char
{
    Read,       // 読み込み (バイナリ)
    Write,      // 書き込み (新規・上書き)
    Append,     // 追記
    ReadText,   // テキスト読み込み
    WriteText,  // テキスト書き込み
    Max
};

//--------------------------
// ネイティブファイルクラス
//--------------------------
class NativeFile
{
public:
    NativeFile() = default;
    NativeFile(const std::filesystem::path& path, FileMode mode);
    ~NativeFile();

    bool open(const std::filesystem::path& path, FileMode mode);
    void close();

    size_t read(void* data, size_t size);
    bool write(const void* data, size_t size);

    bool isOpen() const;
    bool isEnd() const;

    void seek(std::streamoff offset, std::ios_base::seekdir origin = std::ios_base::beg);

    bool exists() { return Exists(m_filePath); }

    std::filesystem::path getFilePath() const { return m_filePath; }

    static bool Exists(const std::filesystem::path& path);
    static bool CreateDir(const std::filesystem::path& path);
    static size_t GetSize(const std::filesystem::path& path);

private:
    std::filesystem::path m_filePath;
    std::fstream m_stream;
    FileMode m_mode;
};
