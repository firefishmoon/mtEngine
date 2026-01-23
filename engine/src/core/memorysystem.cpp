#include <memory.h>
#include <iostream>
#include "loggersystem.h"
#include "memorysystem.h"

template<> MT_API mtMemorySystem* Singleton<mtMemorySystem>::_instance = nullptr;

struct _mtMemHeader {
    size_t size;
    mtMemTag tag;
};

b8 mtMemorySystem::initialize() {
    std::cout << "mtMemorySystem::initialize()" << std::endl;
    MT_LOG_INFO("Memory System Initialized");
    memset(tagInfos, 0, sizeof(tagInfos));
    return true;
}

b8 mtMemorySystem::shutdown() {
    mtLoggerSystem::getInstance()->log(LogLevel::INFO, "Memory System Shutdown");
    return true;
}

void* mtMemorySystem::allocate(mtMemTag tag, size_t size) {
    const u32 realsize = static_cast<u32>(size + sizeof(_mtMemHeader));
    void* ptr = malloc(realsize);
    _mtMemHeader* header = static_cast<_mtMemHeader*>(ptr);
    header->size = size;
    header->tag = tag;
    ptr = static_cast<void*>(static_cast<char*>(ptr) + sizeof(_mtMemHeader));
    mtLoggerSystem::getInstance()->log(LogLevel::DEBUG, "Allocated {} bytes at {}", size, ptr);
    tagInfos[static_cast<size_t>(tag)].totalAllocated += size;
    tagInfos[static_cast<size_t>(tag)].allocationCount += 1;
    return ptr;
}

void mtMemorySystem::deallocate(void* ptr) {
    mtLoggerSystem::getInstance()->log(LogLevel::DEBUG, "Deallocated memory at {}", ptr);
    _mtMemHeader* header = (_mtMemHeader*)(static_cast<char*>(ptr) - sizeof(_mtMemHeader));
    ptr = static_cast<void*>(header);
    tagInfos[static_cast<size_t>(header->tag)].allocationCount -= 1;
    tagInfos[static_cast<size_t>(header->tag)].totalAllocated -= header->size;
    free(ptr);
}

static const char* mem_tag_str[] = {
    "GENERAL",
    "STD_CONTAINERS",
    "RENDERING",
    "PHYSICS",
    "AUDIO",
    "AI",
    "NETWORKING",
    "UI"
};

void mtMemorySystem::reportMemoryUsage() {
    mtLoggerSystem::getInstance()->log(LogLevel::INFO, "Memory Usage Report:");
    for (size_t i = 0; i < static_cast<size_t>(mtMemTag::COUNT); ++i) {
        mtMemTagInfo& info = tagInfos[i];
        // left-align the tag string into a fixed-width column for neat output
        mtLoggerSystem::getInstance()->log(LogLevel::INFO, "  {:<12} : {} bytes in {} allocations", mem_tag_str[i], info.totalAllocated, info.allocationCount);
    }
}

