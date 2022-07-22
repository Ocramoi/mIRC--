#ifndef SERVER_H_
#define SERVER_H_

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
#include <sys/epoll.h>

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
using std::endl;
using std::function;

using epoll_event = struct epoll_event;

using serverHandlers_t = unordered_map<
    string,
    pair<string, function<void(const string&, socket_t fd)>>
    >;

class Server {
    friend class Channel;
    private:
        int eventCount;
        decltype(epoll_create1(0)) epollFd{epoll_create1(0)};
        epoll_event event{},
            *events;
        addrinfo *serverAddrTmp{},
            *serverAddr{};
        int serverSocket{INT_MAX}, clientSocket{INT_MAX};
        atomic_bool listening{true};

        // TODO reimplement future cacheing
        // unordered_map<indexing_t, pair<, future<status_t>>> connections;
        unordered_map<socket_t, Conn::USER_CONNECTION_t> connections;
        mutex connectionsLocker;

        unordered_map<string, Channel> channels;

        mutex &outputMutex;
        string port;

        void setup();
        status_t setupListener();
        // status_t setupSender();
        status_t asyncListener();
        // status_t receiveFromConnection(socket_t clientSocket);

        future<status_t> listenerHandler;

        char msgTmp[Conn::maxMsgSize + 1];

    public:
        static serverHandlers_t handlers;
        static addrinfo serverHints,
            peerHints;
        Server(
            string &_port,
            mutex &_outputMutex
        ) : outputMutex(_outputMutex),
            port(_port) {
            setup();
        };
        ~Server();

        // status_t removePeer(const socket_t& conn);
        // size_t nConnectedPeers();
};

#endif // SERVER_H_
