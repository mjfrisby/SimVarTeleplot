#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "Teleplot.h"

static Teleplot* tp = nullptr;

extern "C"
{

__declspec(dllexport)
void TP_Init(const char* address, int port)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    if (tp)
        delete tp;

    tp = new Teleplot(std::string(address), port);
}

__declspec(dllexport)
void TP_Update(const char* key, double value)
{
    if (!tp) return;
    tp->update(std::string(key), value);
}

__declspec(dllexport)
void TP_Update2D(const char* key, double x, double y)
{
    if (!tp) return;
    tp->update2D(std::string(key), x, y);
}

__declspec(dllexport)
void TP_Log(const char* msg)
{
    if (!tp) return;
    tp->log(std::string(msg));
}

__declspec(dllexport)
void TP_Shutdown()
{
    if (tp)
    {
        delete tp;
        tp = nullptr;
    }

    WSACleanup();
}

}
