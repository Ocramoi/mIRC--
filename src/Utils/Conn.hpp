#ifndef COMM_H_
#define COMM_H_

#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

using std::string;

namespace Conn {
    constexpr size_t maxConns{256},
        maxMsgSize{4096};
    static string PORT{"6667"};
    using USER_CONNECTION_t = struct {
        string nick;
        int socket;
        bool active;
    };
};

#endif // COMM_H_
