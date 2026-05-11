//--------------------------------------------
//
// ファイル根底 [native_file.cpp]
// Author: Fuma Sato
//
//--------------------------------------------
#include "native_file.h"


//--------------------------
// 
// ネイティブファイルクラス
// 
//--------------------------
NativeFile::NativeFile(const std::filesystem::path& path, FileMode mode)
{
    open(path, mode);
}

NativeFile::~NativeFile()
{
    close();
}

//--------------------------
// ファイルを開く
//--------------------------
bool NativeFile::open(const std::filesystem::path& path, FileMode mode)
{
    if (m_stream.is_open()) close();

    m_filePath = path;
    m_mode = mode;
    std::ios_base::openmode openMode = std::ios_base::binary; // バイナリ

    switch (mode)
    {
    case FileMode::Read:
        openMode |= std::ios_base::in;
        break;
    case FileMode::Write:
        // ディレクトリ作成
        if (path.has_parent_path())
        {
            create_directories(path.parent_path());
        }

        openMode |= std::ios_base::out | std::ios_base::trunc;
        break;
    case FileMode::Append:
        // ディレクトリ作成
        if (path.has_parent_path())
        {
            create_directories(path.parent_path());
        }

        openMode |= std::ios_base::out | std::ios_base::app;
        break;
    case FileMode::ReadText:
        openMode = std::ios_base::in; // バイナリフラグを外す
        break;
    case FileMode::WriteText:
        // ディレクトリ作成
        if (path.has_parent_path())
        {
            create_directories(path.parent_path());
        }

        openMode = std::ios_base::out | std::ios_base::trunc;
        break;
    }

    m_stream.open(path, openMode);
    return m_stream.is_open();
}

//--------------------------
// ファイルを閉じる
//--------------------------
void NativeFile::close()
{
    if (m_stream.is_open())
    {
        m_stream.close();
    }
}

//--------------------------
// ファイルからデータを読み込む
//--------------------------
size_t NativeFile::read(void* data, size_t size)
{
    if (!m_stream.is_open()) return 0; // ファイルが開いていない

    // 読み込み
    m_stream.read(static_cast<char*>(data), size);

    // 読み込んだバイト数を返す
    return static_cast<size_t>(m_stream.gcount());
}

//--------------------------
// ファイルにデータを書き込む
//--------------------------
bool NativeFile::write(const void* data, size_t size)
{
    if (!m_stream.is_open()) return false; // ファイルが開いていない

    // 書き込み
    m_stream.write(static_cast<const char*>(data), size);

    // 書き込み成功かどうかを返す
    return m_stream.good();
}

//--------------------------
// ファイルが開いているか、終端に達しているか
//--------------------------
bool NativeFile::isOpen() const { return m_stream.is_open(); }
bool NativeFile::isEnd() const { return m_stream.eof(); }

//--------------------------
// ファイル内の位置を移動する
//--------------------------
void NativeFile::seek(std::streamoff offset, std::ios_base::seekdir origin)
{
    if (m_mode == FileMode::Read || m_mode == FileMode::ReadText)
        m_stream.seekg(offset, origin);
    else
        m_stream.seekp(offset, origin);
}

//--------------------------
// ファイルが存在するか
//--------------------------
bool NativeFile::Exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

//--------------------------
// ディレクトリを作成する
//--------------------------
bool NativeFile::CreateDir(const std::filesystem::path& path)
{
    return std::filesystem::create_directories(path);
}

//--------------------------
// ファイルのサイズを取得する
//--------------------------
size_t NativeFile::GetSize(const std::filesystem::path& path)
{
    try
    {
        // ファイルが存在し、通常のファイルであればサイズを返す
        if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
        {
            return static_cast<size_t>(std::filesystem::file_size(path));
        }
    }
    catch (...) {}

    return 0;
}
