#pragma once

#include <spdlog/spdlog.h>

// HRESULTチェックマクロ
#define HR_CHECK(hr, message) \
    do { \
        if (FAILED(hr)) { \
            _com_error err(hr); \
            LPCTSTR errMsg = err.ErrorMessage(); \
            spdlog::error("HRESULT Failed: {0:#x} ({1}) - {2} (File: {3}, Line: {4})", \
                static_cast<unsigned int>(hr), \
                WideToUtf8(errMsg), \
                message, \
                __FILE__, \
                __LINE__); \
        } \
    } while(0)

void InitLogger();
