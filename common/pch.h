#pragma once

// C
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <strsafe.h>
#include <comdef.h>

// C++(std)
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <array>
#include <algorithm>
#include <wrl/client.h> // Com

// DirectX11
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#define STR(var) #var

// string -> wstring
inline std::wstring Utf8ToWide(std::string_view text)
{
    if (text.empty()) return std::wstring();

    // 必バッファサイズを計算
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);

    // 変換
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &wstrTo[0], size_needed);

    return wstrTo;
}

// wstring -> string
inline std::string WideToUtf8(std::wstring_view wstr)
{
    if (wstr.empty()) return std::string();

    // バッファサイズを計算
    int size_needed = WideCharToMultiByte(
        CP_UTF8, // 変換先は UTF-8
        0,
        &wstr[0], (int)wstr.size(),
        NULL, 0, NULL, NULL
    );

    // 変換
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8,
        0,
        &wstr[0], (int)wstr.size(),
        &strTo[0], size_needed,
        NULL, NULL
    );
    return strTo;
}

inline std::u8string toU8String(std::string_view s)
{
    return std::u8string(s.begin(), s.end());
}

inline std::string ToUtf8String(std::u8string_view s)
{
    return std::string(s.begin(), s.end());
}

inline std::string ToUtf8String(const std::filesystem::path& p)
{
    auto u8 = p.u8string();
    return std::string(u8.begin(), u8.end());
}

constexpr uint64_t Hash(const char* str)
{
    uint64_t h = 2166136261u; // FNV-1a
    while (*str)
    {
        h ^= static_cast<uint8_t>(*str++);
        h *= 16777619u;
    }
    return h;
}

constexpr uint64_t Hash(const char8_t* str)
{
    uint64_t h = 2166136261u; // FNV-1a
    while (*str)
    {
        h ^= static_cast<uint8_t>(*str++);
        h *= 16777619u;
    }
    return h;
}

//------------------------------
// swapを使ったvectorの要素削除
//------------------------------
template<typename T>
void swapRemove(std::vector<T>& vec, size_t index)
{
    if (index < vec.size())
    {
        std::swap(vec[index], vec.back());
        vec.pop_back();
    }
}

//------------------------------
// swapを使ったvectorの要素削除
//------------------------------
template<typename T>
void swapRemove(std::vector<T>& vec, T obj)
{
    if (auto it = std::find(vec.begin(), vec.end(), obj))
    {
        std::swap(it, vec.back());
        vec.pop_back();
    }
}
