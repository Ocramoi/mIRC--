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

        unordered_map<socket_t, Conn::USER_CONNECTION_t> connections{};
        mutex connectionsLocker;

        unordered_map<string, Channel> channels;

        mutex &outputMutex;
        string port;

        void setup();
        status_t setupListener();
        status_t asyncListener();

        char msgTmp[Conn::maxMsgSize + 1];
        status_t sendMsg(const string &msg, socket_t fd);
        status_t handleJoin(const string &raw, socket_t fd);
        status_t handleNick(const string &raw, socket_t fd);
    public:
        static addrinfo serverHints;
        future<status_t> listenerHandler;
        Server(
            string &_port,
            mutex &_outputMutex
        ) : outputMutex(_outputMutex),
            port(_port) {
            setup();
        };
        ~Server();
        void kill();
        serverHandlers_t handlers{{
            {
            "/join", {
            "/join [channel], Joins channel based on argument",
            [&](const string& raw, socket_t fd) -> void {
                handleJoin(raw, fd);
            }
        }
        }, {
            "/nickname", {
            "/nickname [nick], Client is regonized by name on argument",
            [&](const string& raw, socket_t fd) -> void {
                handleNick(raw, fd);
            }
        }}, {
            "/ping", {
            "/ping, Pong! (pings the server)",
            [&](const string&, socket_t fd) -> auto {
                sendMsg("PONG", fd);
            }
        }}, {
            "/commands", {
            "/commands, Prints all available commands",
            [&](const string&, socket_t fd) -> auto {
                string tmp{""};
                for (const auto& command : handlers)
                    tmp += command.first + "-> " + command.second.first + "; ";
                sendMsg(tmp, fd);
            }
        }}
        }};

        // status_t removePeer(const socket_t& conn);
        // size_t nConnectedPeers();
};

#endif // SERVER_H_
