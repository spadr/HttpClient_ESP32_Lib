#pragma once

namespace app {
    /**
     * @brief アプリケーション初期化
     * WiFi接続、NTP同期など共通初期化処理を実行
     */
    void init();
    
    /**
     * @brief HttpClientデモ実行
     * HTTPS/HTTP接続テストやモックテストを実行
     */
    void runDemo();
    
    /**
     * @brief ループ処理（必要時）
     * 継続的な処理が必要な場合に使用
     */
    void tick();
}