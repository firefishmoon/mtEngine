#include "loggersystem.h"
#include "eventsystem.h"

template<> MT_API mtEventSystem* Singleton<mtEventSystem>::_instance = nullptr;

b8 mtEventSystem::initialize() {
    MT_LOG_INFO("Event System Initialized");
    return true;
}
b8 mtEventSystem::shutdown() {
    MT_LOG_INFO("Event System Shutdown");
    return true;
}

size_t mtEventSystem::registerEvent(mtEventType event, mtEventHandler handler) {
    size_t id = _nextHandlerId++;
    _eventHandlers[static_cast<int>(event)].emplace_back(id, handler);
    return id;
}

b8 mtEventSystem::unregisterEvent(mtEventType event, size_t token) {
    auto& handlers = _eventHandlers[static_cast<int>(event)];
    for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        if (it->first == token) {
            handlers.erase(it);
            return true;
        }
    }
    return false;
}

void mtEventSystem::emitEvent(mtEvent event) {
    for (auto &entry : _eventHandlers[static_cast<int>(event.type)]) {
        entry.second(event);
    }
}
