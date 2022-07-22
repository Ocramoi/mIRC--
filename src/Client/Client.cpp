#include "Client.hpp"
#include "../Utils/Utils.hpp"

decltype(Client::peerHints) Client::peerHints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
};

Client::~Client() {
    for (const auto& peer : connections) {
        shutdown(peer.second.first.socket, SHUT_RDWR);
        peer.second.second.wait();
    }

    listening.store(false);
    shutdown(clientSocket, SHUT_RDWR);

    close(clientSocket);
}

void Client::setup() {
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
            cout << "<- " << raw << endl;
        }
    }

    shutdown(connection.socket, SHUT_RDWR);
    connections[clientIp].first.active = false;
    connections.extract(clientIp);
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
            cout << "-> " << msg << endl;
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

auto Client::connected() -> decltype(connected()) {
    return !connections.empty();
}
