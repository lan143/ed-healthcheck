# ed-healthcheck

Небольшая библиотека проверки здоровья (healthcheck) для ESP32 из семейства проектов
домашней автоматизации. Поднимает два HTTP-эндпоинта на ESPAsyncWebServer и опрашивает
зарегистрированные сервисы через абстракцию `Ready`.

## Требования

- ESP32 и Arduino framework (библиотека рассчитана на `espressif32`).
- PlatformIO (или Arduino IDE).
- Зависимости:
  - **ESPAsyncWebServer** (форк ESP32Async);
  - **ed-utils** — сборка JSON через `EDUtils::buildJson`;
  - **ArduinoJson v6** (используется через ed-utils). Версия v7 **несовместима**:
    в ней удалён `DynamicJsonDocument`.

## Установка

### PlatformIO

Через git-репозиторий:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    https://github.com/lan143/ed-healthcheck.git
```

Транзитивные зависимости подтянутся из `library.json`. Если разрешение зависимостей
жалуется на ArduinoJson, добавьте её явно:

```ini
lib_deps =
    https://github.com/lan143/ed-healthcheck.git
    ArduinoJson@^6
```

Локальная разработка (подключение рабочей копии вместо репозитория):

```ini
lib_deps =
    file:///absolute/path/to/ed-healthcheck
    ArduinoJson@^6
```

### Arduino IDE

Склонируйте репозиторий и установите библиотеку через
**Sketch → Include Library → Add .ZIP Library…**, указав ZIP-архив с библиотекой.
Зависимости (ESPAsyncWebServer, ed-utils, ArduinoJson v6) нужно установить отдельно.

## Быстрый старт

```cpp
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <healthcheck.h>

class MqttService : public EDHealthCheck::Ready
{
public:
    EDHealthCheck::ReadyResult ready() override
    {
        if (!mqttConnected) {
            return EDHealthCheck::ReadyResult(false, "mqtt disconnected");
        }
        return EDHealthCheck::ReadyResult(true, "");
    }

    bool mqttConnected = false;
};

MqttService mqttService;
EDHealthCheck::HealthCheck healthCheck;
AsyncWebServer server(80);

void setup()
{
    healthCheck.init(500, 1, 3);
    healthCheck.registerService(&mqttService);

    healthCheck.registerHandlers(&server);
    server.begin();
}

void loop()
{
    healthCheck.loop();
}
```

Объект сервиса должен пережить `HealthCheck`: библиотека хранит сырой указатель
`Ready*` и не владеет им. До первой проверки в `loop()` эндпоинт `/healthcheck/ready`
вернёт 500.

## Эндпоинты

| Метод и путь | Ответ | Назначение |
| --- | --- | --- |
| `GET /healthcheck/healty` | всегда 200, `{"healty":true}` | liveness; зарегистрированные сервисы не опрашиваются |
| `GET /healthcheck/ready` | 200 `{"ready":true,"message":"","warning":true,"warningMessage":"..."}` либо 500 `{"ready":false,"message":"...","warning":false,"warningMessage":""}` | readiness; готовность всех сервисов |

Поля `warning` и `warningMessage` — аддитивные: они присутствуют в ответе всегда и не
влияют на HTTP-код (warning всегда сопровождается 200, если сервисы готовы). Если один
сервис в warning, а другой падает, в 500-ответе придёт `warning:true` вместе с `ready:false`.

Написание `healty` (вместо `healthy`) — историческая опечатка, зафиксированная в API.
Ключ JSON тоже `healty`; не переименовывайте его без явного согласования.

## Настройка

Параметры задаются в `init()`:

```cpp
healthCheck.init(checkInterval, successCount, failsCount);
```

| Параметр | По умолчанию | Назначение |
| --- | --- | --- |
| `checkInterval` | `500` мс | период опроса всех сервисов в `loop()` |
| `successCount` | `1` | минимум успешных проверок, чтобы сервис считался готовым |
| `failsCount` | `3` | максимум допустимых подряд идущих неудач (строгое сравнение: больше — неготов) |

## Как определяется готовность

Каждый зарегистрированный сервис оборачивается в `Service` со счётчиками успехов и
неудач. Успешная проверка (`ReadyResult::isReady() == true`) обнуляет счётчик неудач и
увеличивает счётчик успехов; неудачная — наоборот, увеличивает счётчик неудач, сбрасывает
счётчик успехов и сохраняет сообщение из `ReadyResult`.

Сервис считается готовым, если `getSuccessCount() >= successCount` и
`getFailsCount() <= failsCount`. Эндпоинт `/healthcheck/ready` возвращает 200, только
если готовы **все** сервисы. Если не готовы несколько, в поле `message` попадёт
сообщение последнего сбоившего сервиса в порядке регистрации (перебор идёт без
остановки на первом сбое). Если провалена проверка по числу успехов, сообщение —
`service success count less min sucess count` (опечатка `sucess` также часть API).

## Warning-режим

Сервис может сообщить о некритичной проблеме, вернув из `ready()` результат
`ReadyResult(true, "сообщение", true)`. Это warning: например, отказ некритичного
датчика, при котором сервис остаётся работоспособным.

- Warning не влияет на готовность и HTTP-код.
- На следующем успешном опросе без warning состояние снимается.
- Hard fail доминирует над warning: при `isReady() == false` warning сбрасывается.
- При нескольких warning-сервисах в `warningMessage` попадает сообщение последнего
  в порядке регистрации.
- Ключи `warning` и `warningMessage` присутствуют в ответе всегда.

## Ограничения

- **Потокобезопасность.** Список сервисов не защищён блокировками: `loop()` и
  HTTP-обработчики работают в разных задачах ESPAsyncWebServer. Регистрируйте сервисы
  до старта сервера и не изменяйте список во время работы.
- **Нет тестов и CI.** Единственная проверка — компиляция в потребительском проекте.
  Детали разработки и проверки описаны в `AGENTS.md`.
