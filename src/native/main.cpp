#if defined(NATIVE_TEST) && !defined(PIO_UNIT_TESTING)

#include <iostream>
#include <chrono>
#include <thread>
#include "../app/App.hpp"

// Native環境用のmain関数
// テスト実行時はUnityフレームワークがmain()を提供するため、
// このmain()は「pio run」でsrc/だけをビルドした場合のみ使用される
int main() {
    std::cout << "HttpClient ESP32 Library - Native Build Environment" << std::endl;
    
    try {
        app::init();
        app::runDemo();
        
        std::cout << "Demo completed. Starting main loop..." << std::endl;
        
        // メインループ（必要に応じて）
        for (int i = 0; i < 5; ++i) {
            app::tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        std::cout << "Application finished successfully." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }
}

#endif // NATIVE_TEST && !PIO_UNIT_TESTING