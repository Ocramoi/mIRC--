#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "../Utils/Conn.hpp"

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <future>
#include <atomic>
#include <algorithm>

class Server;

using std::string;
using std::unordered_map;
using std::future;
using std::async;
using std::atomic_bool;
using std::pair;

using epoll_event=struct epoll_event;

class Channel {
    friend class Server;
    private:
        int eventCount;
        decltype(epoll_create1(0)) epollFd{epoll_create1(0)};
        epoll_event event{},
            *events;
        string name;
        future<status_t> futureHandler;
        atomic_bool active{false};
        Server *server;
        unordered_map<socket_t, Conn::USER_CONNECTION_t> connections;
        status_t channelListen();
        status_t setup();
        char msgTmp[Conn::maxMsgSize + 1]; int msgCount;
    public:
        Channel(Server *_server, string _name) : server(_server), name(_name) {
            setup();
        };
        ~Channel();
        status_t joinChannel(Conn::USER_CONNECTION_t &con);
};

#endif // CHANNEL_H_