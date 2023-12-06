#pragma once
#ifndef I_AM_ATOMIC_SPSC_H_
#define I_AM_ATOMIC_SPSC_H_

#include <atomic>
#include <cstddef>
#include <utility>

#include "false_sharing.h"
#include "queue_storage.h"

// BasicSpscQueue provide the atomic head tail readwrite.
// Storage is responsible for elements storage.
template <class Storage, bool ForceAvoidingFalseSharing = false>
class BasicSpscQueue : Storage {
 public:
  template <class... Args>
  requires(requires(Args &&...args) {
    // Help the intellisense.
    Storage(std::forward<Args>(args)...);
  })
  explicit BasicSpscQueue(Args &&...args) : Storage(std::forward<Args>(args)...) {}

  ~BasicSpscQueue() {
    size_t head = head_.value.load(std::memory_order_acquire);
    size_t tail = tail_.value.load(std::memory_order_acquire);
    for (; head != tail; ++head) {
      Storage::DiscardOne(head, tail);
    }
  }

  template <class... Args>
  requires(requires(Storage &s, size_t head, size_t tail, Args &&...args) {
    // Help the intellisense.
    s.ConsumeOne(head, tail, std::forward<Args>(args)...);
  })
  bool Read(Args &&...args) {
    size_t head = head_.value.load(std::memory_order_relaxed);
    size_t tail = tail_.value.load(std::memory_order_acquire);
    if (this->ConsumeOne(head, tail, std::forward<Args>(args)...)) {
      head_.value.fetch_add(1, std::memory_order_release);
      return true;
    }
    return false;
  }

  template <class... Args>
  requires(requires(Storage &s, size_t head, size_t tail, Args &&...args) {
    // Help the intellisense.
    s.ProduceOne(head, tail, std::forward<Args>(args)...);
  })
  bool Write(Args &&...args) {
    size_t head = head_.value.load(std::memory_order_acquire);
    size_t tail = tail_.value.load(std::memory_order_relaxed);
    if (this->ProduceOne(head, tail, std::forward<Args>(args)...)) {
      tail_.value.fetch_add(1, std::memory_order_release);
      return true;
    }
    return false;
  }

  size_t Size() const noexcept {
    size_t head = head_.value.load(std::memory_order_acquire);
    size_t tail = tail_.value.load(std::memory_order_acquire);
    return tail - head;
  }

  bool Empty() const noexcept { return Size() == 0; }

 private:
  EnableCachelineAlignedIf<ForceAvoidingFalseSharing, std::atomic_size_t> head_;
  EnableCachelineAlignedIf<ForceAvoidingFalseSharing, std::atomic_size_t> tail_;
};

template <class T = void>
using SpscQueue = BasicSpscQueue<QueueDefaultStorage<T>>;

#endif  // I_AM_ATOMIC_SPSC_H_
