// === noobWarrior ===
// File: HttpServer.cpp
// Started by: Hattozo
// Started on: 3/10/2025
// Description: Simple implementation of HTTP/1.1 standard using BSD socket library
#include <NoobWarrior/HttpServer/HttpServer.h>
#include <NoobWarrior/Log.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

using namespace NoobWarrior::HttpServer;

int HttpServer::Open(int port, bool useipv6) {
    struct sockaddr_in address;

    mSocketFd = socket(useipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (mSocketFd < 0) {
        Out("HttpServer", "Failed to open socket");
        return mSocketFd;
    }

    if (useipv6) {
        int optNoV6Only;
#if defined(_WIN32)
        setsockopt(mSocketFd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optNoV6Only, sizeof(optNoV6Only));
#else
        setsockopt(mSocketFd, IPPROTO_IPV6, IPV6_V6ONLY, &optNoV6Only, sizeof(optNoV6Only));
#endif
    }

    address.sin_family = useipv6 ? AF_INET6 : AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    int bindRet = bind(mSocketFd, (struct sockaddr*)&address, sizeof(address));
    if (bindRet < 0) {
        Out("HttpServer", "Failed to bind socket to port {}", port);
        return bindRet;
    }

    int listenRet = listen(mSocketFd, 128);
    if (listenRet < 0) {
        Out("HttpServer", "Server listen failed");
        return listenRet;
    }

    Out("HttpServer", "Server listening on port {}", port);

    return 0;
}