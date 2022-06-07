#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>

using std::cout;
using std::endl;
using std::forward;

namespace Utils {
    template <typename ...T>
    inline void debugPrint(T&& ...pack) {
        #ifndef DEBUG
        return;
        #endif
        (cout << ... << forward<T>(pack)) << endl;
    }
};

#endif // UTILS_H_
