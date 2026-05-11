#include "log.h"
#include <spdlog/sinks/basic_file_sink.h> // ファイル出力用
#include <spdlog/sinks/msvc_sink.h>      // Visual Studio のデバッグ出力用

//---------------------
// ロガーの初期化関数
//---------------------
void InitLogger()
{
    // デバッグ出力とファイル
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app_log.txt", true); // logsフォルダにapp_log.txtを作成

    // 2つの出力先持つロガーを作成
    std::initializer_list<spdlog::sink_ptr> sinks = { msvc_sink, file_sink };
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks);

    // デフォルトのロガーとして設定
    spdlog::set_default_logger(logger);

    // ログレベル
    // DEBUG ->trace(最も詳細)
    // RELEASE -> info(簡略)
#ifdef _DEBUG
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    // ログの出力フォーマット
    // [時刻] [ロガー名] [レベル] メッセージ
    spdlog::set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");

    // 初期化ログ
    spdlog::info("Logger initialized.");
}
