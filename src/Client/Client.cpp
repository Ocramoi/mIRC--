#include "Client.hpp"
#include "../Utils/Utils.hpp"

decltype(Client::serverHints) Client::serverHints = {
    .ai_flags = AI_PASSIVE,
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    .ai_addr = NULL,
    .ai_canonname = NULL,
    .ai_next = NULL
}, Client::peerHints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
};

Client::~Client() {
    for (const auto& peer : connections) {
        shutdown(peer.second.first.socket, SHUT_RDWR);
        peer.second.second.wait();
    }

    listening.store(false);
    shutdown(serverSocket, SHUT_RDWR);

    close(serverSocket);
}

void Client::setup() {
    if (port == "0") port = Conn::PORT;

    if (setupListener()) {
        cerr << "Error while setting up listener..." << endl;
        exit(EXIT_FAILURE);
    }

    listenerHandler = async(std::launch::async, [&]() -> auto { return asyncListener(); });
}

auto Client::setupListener() -> decltype(Client::setupListener()) {
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

    return 0;
}

auto Client::asyncListener() -> decltype(Client::asyncListener()) {
    {
        lock_guard<mutex> coutLock{outputMutex};
        Utils::debugPrint("Starting listener...");
    }

    if (listen(serverSocket, Conn::maxConns) < 0) {
        cerr << "Couldn't listen on host" << endl;
        exit(EXIT_FAILURE);
    }

    {
        lock_guard<mutex> coutLock{outputMutex};
        Utils::debugPrint("Listening on port :", port);
    }

    while (listening.load()) {
        sockaddr_storage curClientAddr{};
        socklen_t clientLen{sizeof(decltype(curClientAddr))};

        auto newClientFd{accept(serverSocket, reinterpret_cast<sockaddr*>(&curClientAddr), &clientLen)};
        if (newClientFd < 0) {
            if (listening.load()) cerr << "Error accepting new client..." << endl;
            continue;
        }

        auto clientIp{Utils::ipToString(reinterpret_cast<sockaddr*>(&curClientAddr), clientLen)};

        if (connections.find(clientIp) != connections.end()) {
            lock_guard<mutex> coutLock{outputMutex};
            Utils::debugPrint(
                "Connection rejected! Peer ",
                clientIp,
                "already in pool"
            );
            close(newClientFd);
            continue;
        } else {
            lock_guard<mutex> conLock{connectionsLocker};
            connections[clientIp].first = { .nick = clientIp, .socket = newClientFd, .active = true };
            connections[clientIp].second = async(
                std::launch::async,
                [this, clientIp]() -> auto {
                    return this->receiveFromConnection(clientIp);
                }
            );
        }

        {
            lock_guard<mutex> coutLock{outputMutex};
            cout << ": New connection from " << clientIp << endl;
        }
    }

    return 0;
}

auto Client::receiveFromConnection(
    string clientIp
) -> decltype(Client::receiveFromConnection(clientIp)) {
    Conn::USER_CONNECTION_t connection{};
    try {
        connection = connections[clientIp].first;
    } catch (...) {
        cerr << "Error receiving from connection with " << clientIp << endl;
        return -1;
    }

    char raw[Conn::maxMsgSize + 1] = { '\0' }; size_t nBytes;
    while ((nBytes = recv(connection.socket, raw, Conn::maxMsgSize, 0)) > 0) {
        raw[nBytes] = '\0';
        if (raw[nBytes - 1] == '\n')
            raw[nBytes - 1] = '\0';

        {
            lock_guard<mutex> coutLock{outputMutex};
            Utils::debugPrint("Received ", nBytes, " bytes...");
            cout << "(" << connection.nick << ")" << " <- '" << raw << '\'' << endl;
        }
    }

    {
        lock_guard<mutex> coutLock{outputMutex};
        cout << ": Peer " << clientIp << " disconnected" << endl;
    }

    connections[clientIp].first.active = false;
    close(connection.socket);
    return 0;
}

auto Client::sendMsg(const string &msg) -> decltype(Client::sendMsg(msg)) {
    int nBytes;
    lock_guard<mutex> conLock{connectionsLocker};
    vector<indexing_t> discarted;

    for (const auto& peer : connections) {
        if (!peer.second.first.active) {
            discarted.push_back(peer.first);
            continue;
        }

        if ((nBytes = send(peer.second.first.socket, msg.c_str(), msg.length(), 0)) > 0) {
            lock_guard<mutex> coutLock{outputMutex};
            Utils::debugPrint("Sent ", nBytes, " bytes...");
            cout << "(" << peer.second.first.nick << ") -> " << msg << endl;
        } else
            cerr << "Error while sending to " << peer.first << endl;
    }

    for (const auto& adddr : discarted) {
        lock_guard<mutex> conLock{connectionsLocker};
        connections.erase(adddr);
    }

    return 0;
}

auto Client::connectToPeer(const string &addr, const string &port) -> decltype(Client::connectToPeer(addr, port)) {
    bool checkCon{false};
    {
        lock_guard<mutex> conLock{connectionsLocker};
        checkCon = connections.find(addr) != connections.end();
    }

    if (checkCon) {
        lock_guard<mutex> coutLock{outputMutex};
        cout << "Couldn't connect to " << addr << ", peer already connected" << endl;
        return 1;
    } else {
        lock_guard<mutex> coutLock{outputMutex};
        cout << ": Connecting to " << addr << ':' << port << endl;
    }

    int rv;
    addrinfo *peerTmp, *peerInfo;
    Conn::USER_CONNECTION_t con{ .nick = addr + ":" + port, .active = true };
    if ((rv = getaddrinfo(addr.c_str(), port.c_str(), &peerHints, &peerTmp)) < 0) {
        lock_guard<mutex> coutLock{outputMutex};
        cout << "Couldn't get addrinfo for " << addr << ": \t" << gai_strerror(rv) << endl;
        return 1;
    }

    for (peerInfo = peerTmp; peerInfo != NULL; peerInfo = peerTmp->ai_next) {
        if ((con.socket = socket(peerInfo->ai_family, peerInfo->ai_socktype, peerInfo->ai_protocol)) < 0) {
            lock_guard<mutex> coutLock{outputMutex};
            Utils::debugPrint("Could't create new socket...");
            close(con.socket);
            continue;
        }

        if (connect(con.socket, peerInfo->ai_addr, peerInfo->ai_addrlen) < 0) {
            Utils::debugPrint("Could't connect on new socket...");
            close(con.socket);
            continue;
        }

        break;
    }

    if (!peerInfo) {
        cerr << "Failed socket connection..." << endl;
        return -1;
    }

    freeaddrinfo(peerTmp);

    auto nickCopy{con.nick};
    {
        lock_guard<mutex> conLock{connectionsLocker};
        connections[con.nick].first = con;
        connections[con.nick].second = async(
            std::launch::async,
            [this, nickCopy]() -> auto {
                return this->receiveFromConnection(nickCopy);
            }
        );
    }

    {
        lock_guard<mutex> coutLock{outputMutex};
        cout << ": Connected to peer " << nickCopy << endl;
        return 1;
    }

    return 0;
}

auto Client::nConnectedPeers() -> decltype(Client::nConnectedPeers()) {
    return connections.size();
}
