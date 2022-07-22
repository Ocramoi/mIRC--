#ifndef COMM_H_
#define COMM_H_

#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

using status_t = char;
using indexing_t = std::string;
using socket_t = int;

using std::string;

namespace Conn {
    constexpr size_t maxConns{256},
        maxMsgSize{4096};
    constexpr int maxEvts{512};
    static string PORT{"6667"},
        ADDRESS{"127.0.0.1:6667"};
    using USER_CONNECTION_t = struct {
        string nick,
            ipString;
        int socket;
        bool active;
    };
};

#endif // COMM_H_
