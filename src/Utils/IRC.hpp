#ifndef IRC_H_
#define IRC_H_

#include "./Conn.hpp"

#include <iostream>
#include <sstream>
#include <vector>

using std::string;
using std::stringstream;
using std::vector;

namespace IRC {
    enum PREFIX_t { OUT = 0, NICK = 1 };
    enum COMMAND_t { PRIVMSG = 0, JOIN = 1 };

    using MSG = struct {
        PREFIX_t prefix;
        COMMAND_t command;
        vector<string> parameters;
    };

    char *buildMsg(MSG& msg);
    MSG parseMessage(char raw[Conn::maxMsgSize]);
};

#endif // IRC_H_
