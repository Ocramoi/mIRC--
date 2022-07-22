#include "Channel.hpp"

#include "../Server/Server.hpp"
#include "../Utils/Utils.hpp"

auto Channel::setup() -> decltype(setup()) {
    events = reinterpret_cast<epoll_event*>(calloc(Conn::maxEvts, sizeof(decltype(event))));
    active.store(true);
    futureHandler = async(std::launch::async, [&]() -> auto { return channelListen(); });
    return 0;
}

Channel::~Channel() {
    for (const auto& peer : connections)
        shutdown(peer.first, SHUT_RDWR);
    active.store(false);
    futureHandler.wait();

    if (events) free(events);
}

auto Channel::channelListen() -> decltype(channelListen()) {
    while (active) {
        eventCount = epoll_wait(epollFd, events, Conn::maxEvts, 200);
        for (decltype(eventCount) i{0}; i < eventCount && active.load(); ++i) {
            auto curFd{events[i].data.fd};

            if (
                (events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))
            ) {
                forgetConnection(curFd);
                continue;
            } else {
                auto nBytes{read(curFd, msgTmp, Conn::maxMsgSize)};
                if (nBytes <= 0) {
                    forgetConnection(curFd);
                    continue;
                }

                Utils::debugPrint("Received ", nBytes, " bytes...");
                msgTmp[nBytes] = '\0';
                if (msgTmp[nBytes - 1] == '\n')
                    msgTmp[nBytes - 1] = '\0';
                Utils::debugPrint("Msg: ", msgTmp);

                string received{""};
                do {
                    msgTmp[nBytes] = '\0';
                    if (msgTmp[nBytes - 1] == '\n')
                        msgTmp[nBytes - 1] = '\0';
                    Utils::debugPrint("Received ", nBytes, " bytes...");
                    received += string(msgTmp);
                } while ((nBytes = read(curFd, msgTmp, Conn::maxMsgSize)) > 0);
                Utils::debugPrint("MSG: ", received);

                auto sliced{Utils::split(received, ' ')};
                if (sliced.at(0).at(0) == '/') {
                    cout << curFd << " " << admin.socket << sliced.at(0) << endl;
                    if (
                        curFd == admin.socket &&
                        adminHandlers.find(sliced.at(0)) != adminHandlers.end()
                    )
                        adminHandlers.at(sliced.at(0)).second(received, curFd);
                    else if (handlers.find(sliced.at(0)) != handlers.end())
                        handlers.at(sliced.at(0)).second(received, curFd);
                    else
                        handlers.at("/commands").second(received, curFd);
                } else {
                    if (blacklist.find(curFd) != blacklist.end()) continue;
                    for (const auto& con : connections) {
                        if (con.first == curFd) continue;
                        sendMsg(
                            "@" + connections.at(curFd).nick + ": " + received,
                            con.first
                        );
                    }
                }
            }
        }
    }
    return 0;
}

auto Channel::joinChannel(
    Conn::USER_CONNECTION_t &con
) -> decltype(joinChannel(con)) {
    connections[con.socket] = con;
    event.data.fd = con.socket;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, con.socket, &event) == -1) return 1;
    sendMsg("Welcome to channel " + name, con.socket);
    return 0;
}

auto Channel::forgetConnection(socket_t fd) -> decltype(Channel::forgetConnection(fd)) {
    if (connections.find(fd) == connections.end()) return;
    shutdown(fd, SHUT_RDWR);
    close(fd);
    Utils::msgPrint(
        server->outputMutex,
        "Peer ",
        connections.at(fd).nick,
        " disconnected"
    );
    connections.extract(fd);
}

auto Channel::kickConnection(socket_t fd) -> decltype(Channel::forgetConnection(fd)) {
    Utils::msgPrint(
        server->outputMutex,
        ": Kicking ",
        connections.at(fd).nick
    );
    forgetConnection(fd);
}

auto Channel::sendMsg(const string &msg, socket_t fd) -> decltype(sendMsg(msg, fd)) {
    auto peer{connections.at(fd)};
    string tmp{"#" + name + ": " + msg};
    if (send(fd, tmp.c_str(), tmp.length(), 0) <= 0) {
        cerr << "Error while sending to " << peer.nick << endl;
        forgetConnection(fd);
        return -1;
    }
    return 0;
}

auto Channel::setAdmin(Conn::USER_CONNECTION_t &con) -> decltype(setAdmin(con)) {
    admin = con;
    sendMsg(con.nick + ", you're now a channel admin!", con.socket);
    return 0;
}

auto Channel::handleKick(const string& raw) -> decltype(handleKick(raw)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return -1;
    for (const auto& peer : connections) {
        if (peer.second.nick == sliced.at(1)) {
            kickConnection(peer.first);
            return 0;;
        }
    }
    return 1;
}

auto Channel::handleMute(const string& raw) -> decltype(handleKick(raw)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return -1;
    for (const auto& peer : connections) {
        if (peer.second.nick == sliced.at(1)) {
            blacklist.insert(peer.first);
            return 0;
        }
    }
    return 1;
}

auto Channel::handleUnmute(const string& raw) -> decltype(handleKick(raw)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return -1;
    for (const auto& peer : connections) {
        if (peer.second.nick == sliced.at(1)) {
            blacklist.extract(peer.first);
            return 0;
        }
    }
    return 1;
}

auto Channel::handleWhoIs(const string& raw) -> decltype(handleKick(raw)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return -1;
    for (const auto& peer : connections) {
        if (peer.second.nick == sliced.at(1)) {
            sendMsg("(ADMIN) " + peer.second.ipString, admin.socket);
            return 0;
        }
    }
    return 1;
}
