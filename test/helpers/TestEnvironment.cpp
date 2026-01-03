#include "TestEnvironment.h"

#ifdef NATIVE_TEST
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#endif

bool TestEnvironment::checkConnection(const std::string& host, int port) {
#ifdef NATIVE_TEST
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    bool result = connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    close(sock);
    return result;
#else
    // For non-native tests, assume services are available
    return true;
#endif
}