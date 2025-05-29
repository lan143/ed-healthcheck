#pragma once

#include <list>
#include <ESPAsyncWebServer.h>
#include "service.h"

namespace EDHealthCheck
{
    class HealthCheck
    {
    public:
        void init(unsigned long checkInterval = 500, int successCount = 1, int failsCount = 3);
        void registerHandlers(AsyncWebServer* server);
        void registerService(Ready* check) { _services.push_back(Service(check)); }

        void loop();

    private:
        std::list<Service> _services;

        // config
        unsigned long  _checkInterval = 500;
        int _successCount = 1;
        int _failsCount = 3;

        unsigned long _lastCheckTime = 0;
    };
}
