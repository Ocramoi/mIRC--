#ifndef CLIENT_H_
#define CLIENT_H_

#include "../Utils/Conn.hpp"
#include "../Channel/Channel.hpp"

#include <bits/stdc++.h>
#include <errno.h>
#include <future>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>

using std::string;
using std::unordered_map;
using std::cout;
using std::cerr;
using std::atomic_bool;
using std::async;
using std::mutex;
using std::lock_guard;
using std::future;
using std::pair;
using std::vector;

class Client {
    private:
        int clientSocket{INT_MAX};
        atomic_bool listening{true};

        unordered_map<indexing_t, pair<Conn::USER_CONNECTION_t, future<status_t>>> connections;
        mutex connectionsLocker;

        mutex &outputMutex;

        void setup();
        status_t setupListener();
        status_t setupSender();
        status_t receiveFromConnection(string clientIp);

        future<status_t> listenerHandler;
    public:
        static addrinfo peerHints;
        Client(
            mutex &_outputMutex
        ) : outputMutex(_outputMutex) {
            setup();
        };
        ~Client();

        status_t connectToPeer(const string& addr, const string& port);
        status_t sendMsg(const string& msg);

        bool connected();
};

#endif // CLIENT_H_
