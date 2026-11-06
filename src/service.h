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
                _isWarning = false;
                _warningMessage = "";
            } else if (result.isWarning()) {
                _failsCount = 0;
                _successCount++;
                _isWarning = true;
                _warningMessage = result.getMessage();
            } else {
                _failsCount = 0;
                _successCount++;
                _isWarning = false;
                _warningMessage = "";
            }
        }

        int getFailsCount() { return _failsCount; }
        int getSuccessCount() { return _successCount; }
        std::string getMessage() { return _message; }
        bool isWarning() { return _isWarning; }
        std::string getWarningMessage() { return _warningMessage; }

    private:
        Ready* _check;
        int _failsCount = 0;
        int _successCount = 0;
        std::string _message = "";
        bool _isWarning = false;
        std::string _warningMessage = "";
    };
}
