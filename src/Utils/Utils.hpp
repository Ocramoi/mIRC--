#ifndef UTILS_H_
#define UTILS_H_

#include "../Client/Client.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <array>
#include <algorithm>

using std::cout;
using std::endl;
using std::forward;
using std::string;
using std::function;
using std::shared_ptr;
using std::stringstream;
using std::array;
using std::any_of;
using std::pair;

using return_t = char;
using handlers_t = unordered_map<
    string,
    pair<string, function<return_t(const string&, mutex&, const shared_ptr<Client>&)>>
    >;
using connectionParams_t = array<string, 2>;

/*
** TODO prod print (starts with ': ')
*/

class Utils {
    public:
        template <typename ...T>
        static inline void debugPrint(T&& ...pack) {
            #ifndef DEBUG
            return;
            #endif
            cout << "(DEBUG) ";
            (cout << ... << forward<T>(pack)) << endl;
        }

        static inline string ipToString(const sockaddr *addr, const socklen_t &addrLen)
        {
            char addressBuffer[NI_MAXHOST];
            getnameinfo(
                addr, addrLen,
                addressBuffer, sizeof(addressBuffer),
                0, 0, NI_NUMERICHOST
            );
            return string(addressBuffer);
        }

        static void showHandlerUsage(mutex& outputMutex);
        static bool handleCommand(const string& cmmd, mutex& outputMutex, const shared_ptr<Client>& client);
        static connectionParams_t getConnectionString(const string& raw);
        static return_t connectParameters(const string& raw, const shared_ptr<Client>& client);

        static handlers_t cmmdHandlers;
};

#endif // UTILS_H_
