#pragma once

template <typename T>
class Singleton {
public:
    template<typename... Args>
    static T* Instance(Args&&... args) {
        return nullptr;
    }

};
