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
#include <set>

class Server;

using std::string;
using std::unordered_map;
using std::future;
using std::async;
using std::atomic_bool;
using std::pair;
using std::function;
using std::set;

using epoll_event = struct epoll_event;
using channelHandlers_t = unordered_map<
    string,
    pair<string, function<void(const string&, socket_t fd)>>
    >;

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
        char msgTmp[Conn::maxMsgSize + 1]; int msgCount;
        Conn::USER_CONNECTION_t admin;
        set<socket_t> blacklist;
        status_t channelListen();
        status_t setup();
        void forgetConnection(socket_t fd);
        void kickConnection(socket_t fd);
        status_t sendMsg(const string &msg, socket_t fd);
        status_t handleKick(const string& raw);
        status_t handleMute(const string& raw);
        status_t handleUnmute(const string& raw);
        status_t handleWhoIs(const string& raw);
    public:
        Channel(Server *_server, string _name) : server(_server), name(_name) {
            setup();
        };
        ~Channel();
        status_t joinChannel(Conn::USER_CONNECTION_t& con);
        status_t setAdmin(Conn::USER_CONNECTION_t& con);
        channelHandlers_t adminHandlers{{{
                "/kick", {
                "/kick [user], Closes connection to specified user",
                [&](const string& raw, socket_t) -> auto {
                    if (!handleKick(raw))
                        sendMsg(
                            "(ADMIN) User kicked",
                            admin.socket
                        );
                    else
                        sendMsg(
                            "(ADMIN) User not found",
                            admin.socket
                        );
                }
            }}, {
                "/mute", {
                "/mute [user], Mutes messages coming from specified user",
                [&](const string& raw, socket_t) -> auto {
                    if (!handleMute(raw))
                        sendMsg(
                            "(ADMIN) User muted",
                            admin.socket
                        );
                    else
                        sendMsg(
                            "(ADMIN) User not found",
                            admin.socket
                        );
                }
            }
            }, {
                "/unmute", {
                "/mute [user], Unmutes previously muted user",
                [&](const string& raw, socket_t) -> auto {
                    if (!handleMute(raw))
                        sendMsg(
                            "(ADMIN) User unmuted",
                            admin.socket
                        );
                    else
                        sendMsg(
                            "(ADMIN) User not found",
                            admin.socket
                        );
                }
            }
            }, {
                "/whois", {
                "/whois [user], Returns specified users' IP address",
                [&](const string& raw, socket_t) -> auto {
                    if (handleWhoIs(raw))
                        sendMsg(
                            "(ADMIN) User not found",
                            admin.socket
                        );
                }
            }
            }
        }},  handlers{{{
                    "/commands", {
                        "/commands, Prints all available commands",
                            [&](const string&, socket_t fd) -> auto {
                                string tmp{""};
                                for (const auto& command : handlers)
                                    tmp += command.first + "-> " + command.second.first + "; ";
                                sendMsg(tmp, fd);
                            }
                            }}, {
                    "/ping", {
                        "/ping, Pong! (pings the server)",
                            [&](const string&, socket_t fd) -> auto {
                                sendMsg("PONG", fd);
                            }
                            }}, {
                    "/quit", {
                        "/quit, Exists channel and server gracefully",
                        [&](const string&, socket_t fd) -> auto {
                            sendMsg(
                                "(@ ^-^)/ Bye bye " + connections.at(fd).nick,
                                fd
                            );
                            forgetConnection(fd);
                }
                    }
                }
            }};
};

#endif // CHANNEL_H_
