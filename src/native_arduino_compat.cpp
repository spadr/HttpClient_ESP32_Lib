#include "native_arduino_compat.h"

#ifdef ARDUINO_ARCH_NATIVE

// Global instances for native testing
SerialClass Serial;
WiFiClass WiFi;

#endif // ARDUINO_ARCH_NATIVE