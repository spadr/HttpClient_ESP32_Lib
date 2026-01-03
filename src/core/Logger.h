#pragma once

#include <string>

// ログレベル定義
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

// デフォルトのログレベル（未定義の場合はINFO）
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Arduino環境とNative環境での出力方法の切り替え
#if defined(ARDUINO)
#include <Arduino.h>
#define LOG_PRINT(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#define LOG_PRINTLN(str) Serial.println(str)
#elif defined(NATIVE_TEST)
#include <iostream>
#include <cstdio>
#define LOG_PRINT(fmt, ...) std::printf(fmt, ##__VA_ARGS__)
#define LOG_PRINTLN(str) std::cout << str << std::endl
#else
// その他の環境（ビルドエラー回避のため空定義または標準出力）
#include <cstdio>
#define LOG_PRINT(fmt, ...) std::printf(fmt, ##__VA_ARGS__)
#define LOG_PRINTLN(str) std::printf("%s\n", str)
#endif

// ログマクロ定義

// DEBUGログ
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) LOG_PRINT("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG_S(str) LOG_PRINT("[DEBUG] %s\n", str)
#else
#define LOG_DEBUG(fmt, ...)
#define LOG_DEBUG_S(str)
#endif

// INFOログ
#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG_PRINT("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO_S(str) LOG_PRINT("[INFO] %s\n", str)
#else
#define LOG_INFO(fmt, ...)
#define LOG_INFO_S(str)
#endif

// ERRORログ
#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG_PRINT("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR_S(str) LOG_PRINT("[ERROR] %s\n", str)
#else
#define LOG_ERROR(fmt, ...)
#define LOG_ERROR_S(str)
#endif
