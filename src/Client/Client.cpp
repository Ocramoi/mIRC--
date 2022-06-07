#include "Client.hpp"
#include "../Utils/Utils.hpp"

void Client::setup() {
    if (setupListener()) {
        cerr << "Error while setting up listener..." << endl;
        exit(EXIT_FAILURE);
    }

    auto listenerHandler{async(std::launch::async, [&]() -> auto { return asyncListener(); })};
    listenerHandler.wait();
}

auto Client::setupListener() -> decltype(Client::setupListener()) {
    Utils::debugPrint("Setting up server address...");
    if (getaddrinfo(NULL, Conn::PORT.c_str(), &serverHints, &serverAddrTmp) != 0) {
        cerr << "Couldn't fetch address info!" << endl;
        return 1;
    }

    for (serverAddr = serverAddrTmp; serverAddr != NULL; serverAddr = serverAddr->ai_next) {
        Utils::debugPrint("Setting up server socket...");
        if ((serverSocket = socket(serverAddrTmp->ai_family, serverAddrTmp->ai_socktype, serverAddrTmp->ai_protocol)) < 0) {
            cerr << "Couldn't create listener socket!" << endl;
            continue;
        }

        Utils::debugPrint("Binding server socket...");
        if (bind(serverSocket, serverAddrTmp->ai_addr, serverAddrTmp->ai_addrlen)) {
            cerr << "Couldn't bind server socket to port " << Conn::PORT << endl;
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
    Utils::debugPrint("Listening on port ", Conn::PORT, "...");

    socklen_t clientLen{sizeof(curClientAddr)};

    while (1) {
        auto newClientFd{accept(serverSocket, reinterpret_cast<sockaddr*>(&curClientAddr), &clientLen)};
        if (newClientFd < 0) {
            cerr << "Error accepting new client..." << endl;
            continue;
        }

        char addressBuffer[NI_MAXHOST];
        getnameinfo(
            (struct sockaddr*)&curClientAddr, clientLen,
            addressBuffer, sizeof(addressBuffer),
            0, 0, NI_NUMERICHOST
        );
        Utils::debugPrint("Conection from: ", addressBuffer);

        string txt{"OK"};
        send(newClientFd, txt.c_str(), txt.size() + 1, 0);

        char raw[Conn::maxMsgSize + 1] = { '\0' }; size_t nBytes;
        if ((nBytes = recv(newClientFd, raw, Conn::maxMsgSize, 0)) < 0) {
            cerr << "Error receiving current message!" << endl;
            continue;
        }
        raw[Conn::maxMsgSize] = '\0';
        Utils::debugPrint("Received ", nBytes, " bytes...");
        cout << "MSG: " << raw << endl;

        close(newClientFd);
    }

    return 0;
}
