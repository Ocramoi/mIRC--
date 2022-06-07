#ifndef CLIENT_H_
#define CLIENT_H_

#include "../Utils/Conn.hpp"

#include <bits/stdc++.h>
#include <errno.h>
#include <future>
#include <mutex>
#include <atomic>
#include <unistd.h>
// #include <memory>

using std::string;
using std::unordered_map;
using std::cout;
using std::cerr;
using std::atomic_bool;
using std::async;
using std::mutex;
using std::lock_guard;
// using std::shared_ptr;
// using std::make_shared;

using status_t=char;

class Client {
    private:
        struct addrinfo serverHints{
                .ai_flags = AI_PASSIVE,
                // .ai_family = AF_INET,
                .ai_family = AF_UNSPEC,
                .ai_socktype = SOCK_STREAM,
                .ai_protocol = 0,
                .ai_addr = NULL,
                .ai_canonname = NULL,
                .ai_next = NULL
            },
            *serverAddrTmp{},
            *serverAddr{};
        struct sockaddr_storage curClientAddr;
        int serverSocket{INT_MAX}, clientSocket{INT_MAX};
        unordered_map<string, string> connections;
        atomic_bool listening{true};
        mutex &outputMutex;

        void setup();
        status_t setupListener();
        status_t setupSender();
        status_t asyncListener();
    public:
        Client(string &nick, string &ip_addr, mutex &_outputMutex) : outputMutex(_outputMutex) {
            setup();
        };
        ~Client() = default;
};

#endif // CLIENT_H_
