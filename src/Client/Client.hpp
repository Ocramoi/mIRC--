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

/*
** TODO Use numerical IP address instead of string
** NOTE Maybe use a lock-free alternative for connection handling?
** NOTE Maybe poll()ing over the socket and a pipe for async connection handling on close?
*/

class Client {
    public:
        static addrinfo serverHints,
            peerHints;
    private:
        addrinfo *serverAddrTmp{},
            *serverAddr{};
        int serverSocket{INT_MAX}, clientSocket{INT_MAX};
        atomic_bool listening{true};

        // TODO reimplement future cacheing
        unordered_map<indexing_t, pair<Conn::USER_CONNECTION_t, future<status_t>>> connections;
        mutex connectionsLocker;

        mutex &outputMutex;
        string nick, port;

        void setup();
        status_t setupListener();
        status_t setupSender();
        status_t asyncListener();
        status_t receiveFromConnection(string clientIp);

        future<status_t> listenerHandler;
    public:
        Client(
            string &_nick,
            string &_port,
            mutex &_outputMutex
        ) : nick(_nick),
            outputMutex(_outputMutex),
            port(_port) {
            memset(serverAddrTmp, 0, sizeof(*serverAddrTmp)); memset(serverAddr, 0, sizeof(*serverAddr));
            setup();
        };
        ~Client();

        status_t connectToPeer(const string& addr, const string& port);
        status_t sendMsg(const string& msg);
        size_t nConnectedPeers();
};

#endif // CLIENT_H_
