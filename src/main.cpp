#include "./Client/Client.hpp"
#include "./Server/Server.hpp"
#include "./Utils/Utils.hpp"

#include <bits/stdc++.h>
#include <mutex>
#include <future>
#include <csignal>

using namespace std;

shared_ptr<Client> client;
shared_ptr<Server> server;

void userSignalHandler(decltype(SIGSEGV)) {
    cout << "\n(￣ω￣;) To exit, please use [/quit]'!" << endl;
}

void serverSignalHandler(decltype(SIGSEGV)) {
    cout << "\n(￣ω￣;) Thank you admin-san!" << endl;
    server->kill();
}

int main(int argc, char **argv) {
    mutex outputMutex;
    if (argc == 1 || (string(argv[1]) == "server")) {
        signal(SIGINT, serverSignalHandler);
        string port;
        cout << "Port for binding ['0' for default (" << Conn::PORT << ")]: "; cin >> port;
        server = make_shared<Server>(port, outputMutex);
        server->listenerHandler.wait();
    } else if (argc == 2 && (string(argv[1]) == "client")) {
        signal(SIGINT, userSignalHandler);
        client = make_shared<Client>(outputMutex);
        Utils::showHandlerUsage(outputMutex);
        bool loop{true}; string cmmd;
        while (loop) {
            if (!getline(cin, cmmd)) break;

            Utils::debugPrint("Input: '", cmmd, "'");

            if (!client->connected() && cmmd[0] == '/') {
                loop = Utils::handleCommand(cmmd, outputMutex, client);
            } else {
                client->sendMsg(cmmd);
            }
        }
    } else {
        cout << "Usage: `make run [server/client = server]`" << endl;
        return 0;
    }

    cout << "Bye! See you soon (@ ^-^)/" << endl;

    return 0;
}
