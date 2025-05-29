#include <Json.h>
#include "healthcheck.h"

void EDHealthCheck::HealthCheck::init(unsigned long checkInterval, int successCount, int failsCount)
{
    _checkInterval = checkInterval;
    _successCount = successCount;
    _failsCount = failsCount;
}

void EDHealthCheck::HealthCheck::registerHandlers(AsyncWebServer* server)
{
    server->on("/healthckeck/healty", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        std::string payload = EDUtils::buildJson([this](JsonObject entity) {
            entity["healty"] = true;
        });

        response->write(payload.c_str());
        request->send(response);
    });

    server->on("/healthckeck/ready", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        bool isReady = true;

        std::string payload = EDUtils::buildJson([this, &isReady](JsonObject entity) {
            std::string message = "";

            for (auto service : _services) {
                if (service.getFailsCount() > _failsCount) {
                    isReady = false;
                    message = service.getMessage();
                }

                if (service.getSuccessCount() < _successCount) {
                    isReady = false;
                    message = "service success count less min sucess count";
                }
            }

            entity["ready"] = isReady;
            entity["message"] = message;
        });

        response->setCode(isReady ? 200 : 500);
        response->write(payload.c_str());
        request->send(response);
    });
}

void EDHealthCheck::HealthCheck::loop()
{
    if ((_lastCheckTime + _checkInterval) < millis()) {
        for (auto service : _services) {
            service.check();
        }

        _lastCheckTime = millis();
    }
}
