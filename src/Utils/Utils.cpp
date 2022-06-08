#include "Utils.hpp"

handlers_t Utils::cmmdHandlers = {
    {
        "/commands", {
            "[/command], shows available commands",
            [](const string&, mutex& outputMutex, const shared_ptr<Client>&) -> auto {
                showHandlerUsage(outputMutex);
                return 1;
            }
        }
    },
    { "/quit", { "[/quit], exits the program gracefully", [](const string&, mutex&, const shared_ptr<Client>&) -> auto { return 0; } } },
    {
        "/connect", {
            "[/connect IP PORT], connects to peer over IP:PORT",
            [](const string& parameters, mutex&, const shared_ptr<Client>& client) -> return_t {
                connectParameters(parameters, client);
                return 1;
            }
        }
    }
};

void Utils::showHandlerUsage(mutex& outputMutex) {
    lock_guard<mutex> coutLock{outputMutex};
    cout << "Available commands: " << endl;
    for (const auto& c : cmmdHandlers)
        cout << '\t' << c.first << ": " << c.second.first << endl;
}

auto Utils::handleCommand(const string& cmmd, mutex& outputMutex, const shared_ptr<Client>& client) -> bool {
    stringstream cmmdStream{cmmd};
    string handle; cmmdStream >> handle;
    if (cmmdHandlers.find(handle) == cmmdHandlers.end()) {
        showHandlerUsage(outputMutex);
        return true;
    }

    string parameters, tmp;
    while (cmmdStream >> tmp) parameters += tmp + " ";

    return cmmdHandlers[handle].second(parameters, outputMutex, client);
}

auto Utils::getConnectionString(const string &raw) -> connectionParams_t {
    connectionParams_t params;
    try {
        stringstream rawStream{raw};
        rawStream >> params[0] >> params[1];
    } catch (...) {
        params = { "", "" };
    }
    return params;
}

auto Utils::connectParameters(const string &raw, const shared_ptr<Client> &client) -> return_t {
    auto params{getConnectionString(raw)};
    if (any_of(params.begin(), params.end(), [=](auto s) -> auto { return s.empty(); })) return -1;
    client->connectToPeer(params[0], params[1]);
    return 1;
}
