#pragma once

// テスト用設定の例
// 実際の値はCI環境変数またはローカル設定で上書きされます
namespace Config
{
    const char *ssid = "dummy_ssid";
    const char *password = "dummy_password";

    // NTP設定
    const long gmt_offset_sec = 3600 * 9; // JST (UTC+9)
    const int daylight_offset_sec = 0;

    // E2Eテスト用認証トークン (Cloudflare Workersの環境変数と一致させること)
    // CIでは置換されます
    const char *e2e_token = "dummy_e2e_token";

} // namespace Config

