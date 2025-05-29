#pragma once

#include <Arduino.h>

namespace EDHealthCheck
{
    class ReadyResult
    {
    public:
        ReadyResult(bool isReady, std::string message) : _isReady(isReady), _message(message) {}

        bool isReady() { return _isReady; }
        std::string getMessage() { return _message; }

    private:
        bool _isReady;
        std::string _message;
    };

    class Ready
    {
    public:
        virtual ReadyResult ready() = 0;
    };
}
