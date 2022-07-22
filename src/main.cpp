#include "./Client/Client.hpp"
#include "./Server/Server.hpp"
#include "./Utils/Utils.hpp"

#include <bits/stdc++.h>
#include <mutex>
#include <future>
#include <csignal>

using namespace std;

void signalHandler(decltype(SIGSEGV) sigNum) {
    switch (sigNum) {
        case SIGINT: {
            cout << "\n(￣ω￣;) Para sair, use o comando [/quit]'!" << endl;
            break;
        }
        default:
            break;
    }
}

int main(int argc, char **argv) {
    if (argc > 2) {
        cout << "Usage `make run [server/client = server]`" << endl;
        return 0;
    }

    mutex outputMutex;
    if (argc == 1 || (string(argv[1]) == "server")) {
        string port;
        cout << "Port for binding ['0' for default (" << Conn::PORT << ")]: "; cin >> port;
        Server{port, outputMutex};
    }

    // shared_ptr<Client> client;


    // {
    //     string nick, port;
    //     if (argc <= 1) {
    //         cout << "Nick: "; cin >> nick;
    //     } else if (argc == 2) {
    //         cout << "Nick: "; cin >> nick;
    //         port = string(argv[1]);
    //     } else {
    //         nick = string(argv[1]);
    //         port = string(argv[2]);
    //     }
    //     cout << endl;
    //     client = make_shared<Client>(nick, port, outputMutex);
    // }

    // signal(SIGINT, signalHandler);

    // Utils::showHandlerUsage(outputMutex);

    // bool loop{true}; string cmmd;
    // while (loop) {
    //     if (!getline(cin, cmmd)) break;

    //     Utils::debugPrint("Input: '", cmmd, "'");

    //     if (cmmd[0] == '/') {
    //         loop = Utils::handleCommand(cmmd, outputMutex, client);
    //     } else {
    //         client->sendMsg(cmmd);
    //         if (!client->nConnectedPeers())
    //             cout << "No current connections!" << endl;
    //     }
    // }

    cout << "Bye! See you soon (@ ^-^)/" << endl;

    return 0;
}
