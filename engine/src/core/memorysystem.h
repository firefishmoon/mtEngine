#pragma once

#include "../defines.h"
#include "../common/singleton.h"

enum class mtMemTag {
    GENERAL = 0,
    STD_CONTAINERS,
    RENDERING,
    PHYSICS,
    AUDIO,
    AI,
    NETWORKING,
    UI,
    COUNT
};

class MT_API mtMemorySystem : public Singleton<mtMemorySystem> {
public:
    mtMemorySystem() = default;
    ~mtMemorySystem() = default;

    b8 initialize() override;
    b8 shutdown() override;
    void* allocate(mtMemTag tag, size_t size);
    void deallocate(void* ptr);
    void reportMemoryUsage();

    template<typename T, typename... Args>
    T* allocate(mtMemTag tag, Args&&... args) {
        void* mem = allocate(tag, sizeof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

private:
    class mtMemTagInfo {
    public:
        size_t totalAllocated = 0;
        size_t allocationCount = 0;
    };
    mtMemTagInfo tagInfos[static_cast<size_t>(mtMemTag::COUNT)];
};

#define MT_ALLOCATE(tag, size) mtMemorySystem::getInstance()->allocate(tag, size)
#define MT_NEW(tag, type, ...) mtMemorySystem::getInstance()->allocate<type>(tag, ##__VA_ARGS__)
#define MT_DELETE(ptr) mtMemorySystem::getInstance()->deallocate(ptr)


