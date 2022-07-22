#include "Channel.hpp"

#include "../Server/Server.hpp"
#include "../Utils/Utils.hpp"

decltype(Channel::handlers) Channel::handlers = {
"/kick", {
"Closes connection to specified user",
[](const string& raw, socket_t fd) -> void {}
}
};

auto Channel::setup() -> decltype(setup()) {
    events = reinterpret_cast<epoll_event*>(calloc(Conn::maxEvts, sizeof(decltype(event))));
    active.store(true);
    futureHandler = async(std::launch::async, [&]() -> auto { return channelListen(); });
    return 0;
}

Channel::~Channel() {
    if (events) free(events);
}

auto Channel::channelListen() -> decltype(channelListen()) {
    while (active) {
        eventCount = epoll_wait(epollFd, events, Conn::maxEvts, -1);
        for (decltype(eventCount) i{0}; i < eventCount; ++i) {
            auto curFd{events[i].data.fd};

            if (
                (events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (events[i].events & EPOLLERR)
            ) throw std::runtime_error("Faulty descriptor notified!");
            else {

                auto nBytes{read(events[i].data.fd, msgTmp, Conn::maxMsgSize)};
                if (nBytes <= 0) {
                    Utils::msgPrint(
                        server->outputMutex,
                        ": Peer ",
                        connections[curFd].nick,
                        " disconnected"
                    );
                    close(curFd);
                    connections.extract(curFd);
                    continue;
                }

                Utils::debugPrint("Received ", nBytes, " bytes...");
                msgTmp[nBytes] = '\0';
                if (msgTmp[nBytes - 1] == '\n')
                    msgTmp[nBytes - 1] = '\0';
                Utils::debugPrint("Msg: ", msgTmp);
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
    return 0;
}
