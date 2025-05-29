#pragma once

#include "ready.h"

namespace EDHealthCheck
{
    class Service
    {
    public:
        Service(Ready* check) : _check(check) {}

        void check()
        {
            ReadyResult result = _check->ready();
            if (!result.isReady()) {
                _failsCount++;
                _successCount = 0;
                _message = result.getMessage();
            } else {
                _failsCount = 0;
                _successCount++;
            }
        }

        int getFailsCount() { return _failsCount; }
        int getSuccessCount() { return _successCount; }
        std::string getMessage() { return _message; }

    private:
        Ready* _check;
        int _failsCount = 0;
        int _successCount = 0;
        std::string _message = "";
    };
}
