#pragma once

#include <Arduino.h>

namespace EDHealthCheck
{
    class ReadyResult
    {
    public:
        ReadyResult(bool isReady, std::string message, bool isWarning = false) : _isReady(isReady), _message(message), _isWarning(isWarning) {}

        bool isReady() { return _isReady; }
        std::string getMessage() { return _message; }
        bool isWarning() { return _isWarning; }

    private:
        bool _isReady;
        std::string _message;
        bool _isWarning;
    };

    class Ready
    {
    public:
        virtual ReadyResult ready() = 0;
    };
}
