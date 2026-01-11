#pragma once

#include "../defines.h"
#include "../common/singleton.h"

enum class MemTag {
    GENERAL = 0,
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
    void* allocate(size_t size, MemTag tag = MemTag::GENERAL);
    void deallocate(void* ptr);
    void reportMemoryUsage();
private:
    class mtMemTagInfo {
    public:
        size_t totalAllocated = 0;
        size_t allocationCount = 0;
    };
    mtMemTagInfo tagInfos[static_cast<size_t>(MemTag::COUNT)];
};
