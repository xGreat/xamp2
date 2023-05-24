//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
class DefaultFactory {
public:
    T *Create() {
        return new T();
    }
};

template 
<
    typename T,
    typename FactoryType = DefaultFactory<T>
>
class XAMP_BASE_API_ONLY_EXPORT ObjectPool : public std::enable_shared_from_this<ObjectPool<T, FactoryType>> {
private:
    class ReturnToPool;

    using factory_type = FactoryType;
    using pool_type = ObjectPool<T, FactoryType>;
    using deleter_type = pool_type::ReturnToPool;
    using ptr_type = std::unique_ptr<T>;

public:
    using return_ptr_type = std::unique_ptr<T, deleter_type>;

    explicit ObjectPool(size_t init_size)
        : current_size_(0)
        , max_size_(2 * init_size)
        , init_size_(init_size) {
        Init();
    }

    ObjectPool(size_t init_size, typename std::remove_reference<factory_type>::type &&factory)
        : current_size_(0)
        , max_size_(2 * init_size)
        , init_size_(init_size)
        , factory_(std::move(factory)) {
        Init();
    }

    ObjectPool(size_t init_size,
               typename std::conditional<std::is_reference<factory_type>::value, factory_type, const factory_type &>::type factory)
        : current_size_(0)
        , max_size_(2 * init_size)
        , init_size_(init_size)
        , factory_(factory) {        
        Init();
    }
    
    void Release(ptr_type &object) {
        std::unique_lock<FastMutex> lck(object_mutex_);
        objects_.push_back(std::move(object));
        idle_cv_.notify_one();
    }

    return_ptr_type Acquire() {
        std::unique_lock<FastMutex> lck(object_mutex_);

        while (true) {
            auto size = objects_.size();

            if (size > 0) {
                return_ptr_type ptr(objects_.back().release(), deleter_type{this->shared_from_this()});
                objects_.pop_back();
                return ptr;
            } else {
                auto obj = this->CreateObject();
                if (obj != nullptr)
                    return return_ptr_type(obj, deleter_type{this->shared_from_this()});
                idle_cv_.wait(lck);
            }
        }
    }

private:
    size_t current_size_;
    size_t max_size_;
    size_t init_size_;

    FastMutex object_mutex_;
    FastConditionVariable idle_cv_;

    factory_type factory_;
    std::vector<std::unique_ptr<T>> objects_;

    T *CreateObject() {
        if (current_size_ < max_size_) {
            current_size_++;
            return factory_.Create();
        }
        else {
            return nullptr;
        }
    }

    void Init()  {
        for (int i = 0; i < init_size_; ++i) {
            objects_.emplace_back(CreateObject());
        }
    }

    class ReturnToPool {
    private:
        std::weak_ptr<pool_type> pool_;

    public:
        explicit ReturnToPool(const std::shared_ptr<pool_type> &ptr)
            : pool_(ptr) {
        }

        void operator()(T *object) {
            ptr_type ptr(object);
            if (auto sp = pool_.lock()) {
                try {
                    sp->Release(ptr);
                } catch (...) {
                }
            }
        }
    };
};

XAMP_BASE_NAMESPACE_END
