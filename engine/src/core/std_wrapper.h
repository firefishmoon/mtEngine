#pragma once

#include "../defines.h"
#include "core/memorysystem.h"

#include <vector>
#include <queue>
#include <deque>

template<typename T>
class MT_API mtStdAllocator {
public:
    using value_type = T;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using size_type = std::size_t;

    mtStdAllocator() = default;

    template<typename U>
    mtStdAllocator(const mtStdAllocator<U>&) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(mtMemorySystem::getInstance()->allocate(mtMemTag::STD_CONTAINERS, n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) {
        mtMemorySystem::getInstance()->deallocate(p);
    }

    // template <typename U>
    // bool operator==(const mtStdAllocator<U>&) const noexcept { return true; }
    //
    // template <typename U>
    // bool operator!=(const mtStdAllocator<U>&) const noexcept { return false; }
};

template<typename T>
using mtVector = std::vector<T, mtStdAllocator<T>>;

template<typename T>
using mtDeque = std::deque<T, mtStdAllocator<T>>;

template<typename T>
using mtQueue = std::queue<T, mtDeque<T>>;
