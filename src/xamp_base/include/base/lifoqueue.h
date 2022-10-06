//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stack>
#include <atomic>
#include <vector>

#include <base/base.h>

namespace xamp::base {

template
<
    typename T,
    typename V =
    std::enable_if_t
    <
    std::is_nothrow_move_assignable_v<T>
    >
>
class LIFOQueue {
public:
    explicit LIFOQueue(size_t size)
        : size_{size} {
    }

    void emplace(T&& item) noexcept {
        queue_.emplace(std::move(item));
    }

    T& top() {
        return queue_.top();
    }

    void pop() {
        queue_.pop();
    }

    bool empty() const {
        return queue_.empty();
    }

    size_t size() const noexcept {
        return queue_.size();
    }
private:
    size_t size_;
    std::stack<T, std::vector<T>> queue_;
};

template 
<
    typename T,
    typename V =
    std::enable_if_t
    <
    std::is_nothrow_move_assignable_v<T>
    >
>
class SpinLockFreeStack {
public:
    struct Node {
        Node *next;
        T value;
    };

    struct Head {
        uintptr_t aba;
        Node *node;

        Head() noexcept
            : aba(0)
            , node(nullptr) {
        }

        Head(Node* ptr) noexcept
            : aba(0)
            , node(ptr) {
        }
    };

    explicit SpinLockFreeStack(size_t size)
        : size_{size} {
        head_.store(Head(), std::memory_order_relaxed);
        buffer_.resize(size);
        for (size_t i = 0; i < size - 1; ++i) {
            buffer_[i].next = &buffer_[i + 1];
        }
        buffer_[size-1].next = nullptr;
        free_nodes_.store(Head(buffer_.data()),std::memory_order_relaxed);
    }

    bool TryDequeue(T& task) noexcept {
        auto *node = Pop(head_);
        if (!node) {
            return false;
        }

        task = std::move(node->value);
        Push(free_nodes_, node);
        return true;
    }

    template <typename U>
    bool TryEnqueue(U &&task) noexcept {
        auto *node = Pop(free_nodes_);
        if (!node) {
            return false;
        }

        node->value = std::forward<U>(task);
        Push(head_, node);
        return true;
    }

    size_t size() const noexcept {
        return 0;
    }

private:
    Node* Pop(std::atomic<Head>& h) {
        Head next{}, orig = h.load(std::memory_order_relaxed);
        do {
            if (orig.node == nullptr) {
                return nullptr;
            }
                
            next.aba = orig.aba + 1;
            next.node = orig.node->next;
        } while (!h.compare_exchange_weak(orig, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire));
        return orig.node;
    }

    void Push(std::atomic<Head>& h, Node* node) {
        Head next{}, orig = h.load(std::memory_order_relaxed);
        do {
            node->next = orig.node;
            next.aba = orig.aba + 1;
            next.node = node;
        } while (!h.compare_exchange_weak(orig, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire));
    }

    size_t size_;
    std::vector<Node> buffer_;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<Head> head_;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<Head> free_nodes_;
    uint8_t padding_[kCacheAlignSize - sizeof(free_nodes_)]{ 0 };
};

}

