#pragma once

#include <stdexcept>

template <typename T>
class Singleton {
public:
    template<typename... Args>
    static T* instance(Args&&... args) {
        if (!_instance) {
            _instance = new T(std::forward<Args>(args)...);
        }
        return _instance;
    }

    static T* getInstance() {
        if (_instance == nullptr) {
            throw std::logic_error("Singleton instance not created yet. Call Instance() first.");
        }
        return _instance;
    }

    static void destroyInstance() {
        delete _instance;
        _instance = nullptr;
    }

    Singleton() = default;
    virtual ~Singleton() = default;
private:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    static T* _instance;
};

template <typename T> T* Singleton<T>::_instance = nullptr;
