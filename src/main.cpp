#include "./Client/Client.hpp"
#include "./Utils/Utils.hpp"

#include <bits/stdc++.h>
#include <mutex>
#include <future>

using namespace std;

int main(int argc, char **argv) {
    shared_ptr<Client> client;
    mutex outputMutex;

    {
        string nick, port;
        if (argc <= 1) {
            cout << "Nick: "; cin >> nick;
            cout << "Port for binding ['0' for default (" << Conn::PORT << ")]: "; cin >> port;
        } else if (argc == 2) {
            cout << "Nick: "; cin >> nick;
            port = string(argv[1]);
        } else {
            nick = string(argv[1]);
            port = string(argv[2]);
        }
        cout << endl;
        client = make_shared<Client>(nick, port, outputMutex);
    }

    Utils::showHandlerUsage(outputMutex);

    bool loop{true}; string cmmd;
    while (loop) {
        if (!getline(cin, cmmd)) break;

        Utils::debugPrint("Input: '", cmmd, "'");

        if (cmmd[0] == '/') {
            loop = Utils::handleCommand(cmmd, outputMutex, client);
        } else {
            client->sendMsg(cmmd);
            if (!client->nConnectedPeers())
                cout << "No current connections!" << endl;
        }
    }

    cout << "Bye! See you soon (@ ^-^)/" << endl;

    return 0;
}
