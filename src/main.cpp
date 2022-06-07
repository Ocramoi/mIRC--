#include <bits/stdc++.h>
#include <mutex>
#include <future>
#include "./Client/Client.hpp"

using namespace std;

int main(int argc, char **argv) {
    shared_ptr<Client> c;
    mutex outputMutex;

    {
        string nick, ip;
        if (argc <= 1) {
            cout << "Nick: "; cin >> nick;
            cout << "Address ['0' for passive mode]: "; cin >> ip;
        } else if (argc == 2) {
            cout << "Nick: "; cin >> nick;
            ip = string(argv[1]);
        } else {
            nick = string(argv[1]);
            ip = string(argv[2]);
        }
        cout << endl;
        c = make_shared<Client>(nick, ip, outputMutex);
    }

    return 0;
}
