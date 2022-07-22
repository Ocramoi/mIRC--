#include "Server.hpp"

#include "../Utils/Utils.hpp"
#include "../Channel/Channel.hpp"


decltype(Server::serverHints) Server::serverHints = {
    .ai_flags = AI_PASSIVE,
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    .ai_addr = NULL,
    .ai_canonname = NULL,
    .ai_next = NULL
};

Server::~Server() {
    if (!listening.load()) return;
    kill();
}

void Server::kill() {
    channels.clear();
    lock_guard<mutex> lockConnections{connectionsLocker};
    for (const auto& peer : connections)
        shutdown(peer.first, SHUT_RDWR);
    connections.clear();
    listening.store(false);

    shutdown(serverSocket, SHUT_RDWR);
    close(serverSocket);

    if (events) {
        free(events);
        events = nullptr;
    }
}

void Server::setup() {
    if (port == "0") port = Conn::PORT;

    events = reinterpret_cast<epoll_event*>(calloc(Conn::maxEvts, sizeof(decltype(event))));
    if (setupListener()) {
        cerr << "Error while setting up listener..." << endl;
        exit(EXIT_FAILURE);
    }

    listenerHandler = async(std::launch::async, [&]() -> auto { return asyncListener(); });
}

auto Server::setupListener() -> decltype(Server::setupListener()) {
    Utils::debugPrint("Setting up server address...");
    if (
        getaddrinfo(
            NULL,
            port.c_str(),
            &serverHints, &serverAddrTmp
        ) != 0
    ) {
        cerr << "Couldn't fetch address info!" << endl;
        return 1;
    }

    for (serverAddr = serverAddrTmp; serverAddr != NULL; serverAddr = serverAddr->ai_next) {
        Utils::debugPrint("Setting up server socket...");
        if ((serverSocket = socket(serverAddrTmp->ai_family, serverAddrTmp->ai_socktype, serverAddrTmp->ai_protocol)) < 0) {
            cerr << "Couldn't create listener socket!" << endl;
            continue;
        }

        int yes{1};
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));

        Utils::debugPrint("Binding server socket...");
        if (bind(serverSocket, serverAddrTmp->ai_addr, serverAddrTmp->ai_addrlen)) {
            cerr << "Couldn't bind server socket to port " << port << endl;
            close(serverSocket);
            continue;
        }

        break;
    }

    freeaddrinfo(serverAddrTmp);

    if (!serverAddr) {
        cerr << "Couldn't resolve host" << endl;
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, Conn::maxConns) < 0) {
        cerr << "Couldn't listen on host" << endl;
        exit(EXIT_FAILURE);
    }

    switch (Utils::setNonBlocking(serverSocket)) {
        case Utils::NONBLOCK:
            cerr << "Error setting non blocking" << endl;
            exit(EXIT_FAILURE);
            break;
        case Utils::INVALID:
            cerr << "Invalid descriptor for operation" << endl;
            exit(EXIT_FAILURE);
            break;
        default:
            break;
    }

    {
        lock_guard<mutex> coutLock{outputMutex};
        Utils::debugPrint("Listening on port :", port);
    }

    return 0;
}

auto Server::asyncListener() -> decltype(Server::asyncListener()) {
    event.data.fd = serverSocket;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) return 1;

    while (listening.load()) {
        eventCount = epoll_wait(epollFd, events, Conn::maxEvts, 200);

        for (decltype(eventCount) i{0}; i < eventCount && listening.load(); ++i) {
            auto curFd{events[i].data.fd};

            if (
                (events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))
            ) {
                lock_guard<mutex> lockConnections(connectionsLocker);
                if (connections.find(curFd) != connections.end()) {
                    Utils::msgPrint(
                        outputMutex,
                        "Peer ",
                        connections[curFd].nick,
                        " disconnected"
                    );
                    shutdown(curFd, SHUT_RDWR);
                    connections.extract(curFd);
                }
                continue;
            } else if (curFd == serverSocket) {
                sockaddr_storage curServerAddr{};
                socklen_t clientLen{sizeof(decltype(curServerAddr))};

                auto newServerFd{accept(serverSocket, reinterpret_cast<sockaddr*>(&curServerAddr), &clientLen)};
                if (newServerFd < 0) {
                    if (listening.load()) cerr << "Error accepting new client..." << endl;
                    continue;
                }

                Utils::setNonBlocking(newServerFd);
                event.data.fd = newServerFd;
                event.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, newServerFd, &event) == -1) {
                    cerr << "Error polling new connection" << endl;
                    close(newServerFd);
                    continue;
                }

                auto clientIp{Utils::ipToString(reinterpret_cast<sockaddr*>(&curServerAddr), clientLen)};

                {
                    lock_guard<mutex> conLock{connectionsLocker};
                    connections.insert({newServerFd, { .nick = clientIp, .ipString = clientIp, .socket = newServerFd, .active = true }});
                }

                Utils::msgPrint(outputMutex, "New connection from ", clientIp);
            } else {
                auto count{read(curFd, msgTmp, Conn::maxMsgSize)};
                if (count <= 0) {
                    lock_guard<mutex> lockConnections(connectionsLocker);
                    if (connections.find(curFd) != connections.end()) {
                        Utils::msgPrint(
                            outputMutex,
                            "Peer ",
                            connections[curFd].nick,
                            " disconnected"
                        );
                        shutdown(curFd, SHUT_RDWR);
                        connections.extract(curFd);
                    }
                    continue;
                }

                string received{""};
                do {
                    msgTmp[count] = '\0';
                    if (msgTmp[count - 1] == '\n')
                        msgTmp[count - 1] = '\0';
                    Utils::debugPrint("Received ", count, " bytes...");
                    received += string(msgTmp);
                } while ((count = read(curFd, msgTmp, Conn::maxMsgSize)) > 0);
                Utils::debugPrint("MSG: ", received);

                auto sliced{Utils::split(received, ' ')};
                if (sliced.at(0).at(0) == '/') {
                    if (handlers.find(sliced.at(0)) != handlers.end())
                        handlers.at(sliced.at(0)).second(received, curFd);
                    else
                        handlers.at("/commands").second(received, curFd);
                } else {
                    sendMsg("Try using a command! ('/...')", curFd);
                }
            }
        }
    }

    return 0;
}

auto Server::handleNick(
    const string &raw,
    socket_t fd
) -> decltype(Server::handleNick(raw, fd)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return 1;
    Utils::debugPrint(
        "Renaming '" +
        connections.at(fd).nick +
        "' to '" +
        sliced.at(1)
    );
    connections.at(fd).nick = sliced.at(1);
    sendMsg("Succesfully renamed to '" + sliced.at(1) + "'!", fd);
    return 0;
}

auto Server::handleJoin(
    const string &raw,
    socket_t fd
) -> decltype(handleJoin(raw, fd)) {
    auto sliced{Utils::split(raw, ' ')};
    if (sliced.size() < 2) return 1;

    auto name{sliced.at(1)};
    bool newChannel{false};
    if (channels.find(name) == channels.end()) {
        newChannel = true;
        channels.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(this, name)
        );
    }

    auto peer{connections.at(fd)};
    {
        channels.at(name).joinChannel(peer);
        lock_guard<mutex> lockConnections(connectionsLocker);
        connections.extract(fd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    }

    if (newChannel) channels.at(name).setAdmin(peer);

    return 0;
}

auto Server::sendMsg(const string &msg, socket_t fd) -> decltype(sendMsg(msg, fd)) {
    auto peer{connections.at(fd)};
    if (send(fd, msg.c_str(), msg.length(), 0) <= 0) {
        cerr << "Error while sending to " << peer.nick << endl;
        lock_guard<mutex> lockConnections(connectionsLocker);
        if (connections.find(fd) != connections.end()) {
            Utils::msgPrint(
                outputMutex,
                "Peer ",
                connections.at(fd).nick,
                " disconnected"
            );
            shutdown(fd, SHUT_RDWR);
            connections.extract(fd);
        }
        return -1;
    }
    return 0;
}
