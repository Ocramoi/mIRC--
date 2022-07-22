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
}, Server::peerHints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
};

decltype(Server::handlers) Server::handlers = {};

Server::~Server() {
    listening.store(false);

    for (const auto& peer : connections)
        shutdown(peer.first, SHUT_RDWR);

    shutdown(serverSocket, SHUT_RDWR);

    close(serverSocket);
}

void Server::setup() {
    if (port == "0") port = Conn::PORT;

    events = reinterpret_cast<epoll_event*>(calloc(Conn::maxEvts, sizeof(decltype(event))));
    if (setupListener()) {
        cerr << "Error while setting up listener..." << endl;
        exit(EXIT_FAILURE);
    }

    listenerHandler = async(std::launch::async, [&]() -> auto { return asyncListener(); });
    listenerHandler.wait(); // TODO Better waiting
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
        eventCount = epoll_wait(epollFd, events, Conn::maxEvts, -1);

        for (decltype(eventCount) i{0}; i < eventCount; ++i) {
            auto curFd{events[i].data.fd};

            if (
                (events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (events[i].events & EPOLLERR)
            ) throw std::runtime_error("Faulty descriptor notified!");
            else if (curFd == serverSocket) {
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

                lock_guard<mutex> conLock{connectionsLocker};
                connections[newServerFd] = { .nick = clientIp, .ipString = clientIp, .socket = newServerFd, .active = true };
                channels.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple("main"),
                    std::forward_as_tuple(this, "main")
                );
                channels.at("main").joinChannel(connections[newServerFd]);

                {
                    lock_guard<mutex> coutLock{outputMutex};
                    cout << ": New connection from " << clientIp << endl;
                }
            } else {
                auto count{read(curFd, msgTmp, Conn::maxMsgSize)};
                if (count <= 0) {
                    if (errno != EAGAIN) shutdown(events[i].data.fd, SHUT_RDWR);
                    Utils::msgPrint(
                        outputMutex,
                        ": Peer ",
                        connections[curFd].nick,
                        " disconnected"
                    );
                    close(curFd);
                    connections.extract(curFd);
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

                auto values{Utils::split(received, ' ')};
                if (values.at(0).at(0) != '/') {
                }
            }
        }
    }

    return 0;
}

// auto Server::receiveFromConnection(
//     socket_t clientSocket
// ) -> decltype(Server::receiveFromConnection(clientSocket)) {
//     Conn::USER_CONNECTION_t connection{};
//     try {
//         connection = connections[clientSocket].first;
//     } catch (...) {
//         cerr << "Error receiving from connection with " << clientSocket << endl;
//         return -1;
//     }

//     char raw[Conn::maxMsgSize + 1] = { '\0' }; size_t nBytes;
//     while ((nBytes = recv(connection.socket, raw, Conn::maxMsgSize, 0)) > 0) {
//         raw[nBytes] = '\0';
//         if (raw[nBytes - 1] == '\n')
//             raw[nBytes - 1] = '\0';

//         {
//             lock_guard<mutex> coutLock{outputMutex};
//             Utils::debugPrint("Received ", nBytes, " bytes...");
//             cout << "(" << connection.nick << ")" << " <- '" << raw << '\'' << endl;
//         }
//     }

//     {
//         lock_guard<mutex> coutLock{outputMutex};
//         cout << ": Peer " << clientSocket << " disconnected" << endl;
//     }

//     connections[clientSocket].first.active = false;
//     close(connection.socket);
//     return 0;
// }
