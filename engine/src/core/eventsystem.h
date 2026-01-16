#pragma once

#include <vector>
#include <functional>
#include "../defines.h"
#include "../common/singleton.h"

enum class mtEventType {
    NONE = 0,
    KEYBOARD_PRESS,
    KEYBOARD_RELEASE,
    KEYBOARD_REPEAT,
    MOUSE,
    WINDOW,
    FRAME,
    CUSTOM,
    COUNT
};

struct mtEvent {
    mtEventType type;
    // Additional event data can be added here
    union {
        u32 data;
        f32 fdata;
    };

    mtEvent() : type(mtEventType::NONE), data(0) {}
    mtEvent(mtEventType t, u32 d) : type(t), data(d) {}
    mtEvent(mtEventType t, f32 fd) : type(t), fdata(fd) {}
};

typedef std::function<void(mtEvent)> mtEventHandler;

class MT_API mtEventSystem : public Singleton<mtEventSystem> {
public:
    mtEventSystem() = default;
    ~mtEventSystem() = default;

    b8 initialize() override;
    b8 shutdown() override;

    // Register an event handler. Returns a token (non-zero) that can be
    // used to unregister the handler later.
    size_t registerEvent(mtEventType event, mtEventHandler handler);
    // Unregister by token returned from registerEvent. Returns true if removed.
    b8 unregisterEvent(mtEventType event, size_t token);

    void emitEvent(mtEvent event);

private:
    std::vector<std::pair<size_t, mtEventHandler>> _eventHandlers[static_cast<int>(mtEventType::COUNT)];
    size_t _nextHandlerId = 1;
};
