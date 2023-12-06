#pragma once
#ifndef I_AM_ATOMIC_SPSC_H_
#define I_AM_ATOMIC_SPSC_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

#include "false_sharing.h"

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

template <class T, class Alloc = std::allocator<T>>
class SpscQueueRingBufferStorage {
 public:
  SpscQueueRingBufferStorage(size_t n, const Alloc &alloc = Alloc()) : alloc_(alloc) {
    if (n == 0) {
      n = 1;
    }
    // Round up to the nearest power of 2.
    if (n != (n & -n)) {
      n <<= 1;
      do {
        n &= n - 1;
      } while (n != (n & -n));
    }
    buf_ = std::allocator_traits<Alloc>::allocate(alloc_, n);
    mask_ = n - 1;
  }

  ~SpscQueueRingBufferStorage() {
    std::allocator_traits<Alloc>::deallocate(alloc_, buf_, mask_ + 1);
  }

  bool ConsumeOne(size_t head, size_t tail, T &dst) {
    if (head != tail) {
      dst = std::move(buf_[head & mask_]);
      std::allocator_traits<Alloc>::destroy(alloc_, buf_ + (head & mask_));
      return true;
    }
    return false;
  }

  template <class... Args>
  bool ProduceOne(size_t head, size_t tail, Args &&...args) {
    if (tail - head <= mask_) {
      std::allocator_traits<Alloc>::construct(alloc_, buf_ + (tail & mask_), std::forward<Args>(args)...);
      return true;
    }
    return false;
  }

 private:
  T *buf_;
  size_t mask_;
  Alloc alloc_;
};

class SpscQueueNoneStorage {
 public:
  bool ConsumeOne(size_t head, size_t tail) { return head != tail; }
  bool ProduceOne(size_t head, size_t tail) { return true; }
};

template <class T>
using SpscQueueDefaultStorage = std::conditional_t<
    std::is_void_v<T>, SpscQueueNoneStorage, SpscQueueRingBufferStorage<T>>;

template <class T = void>
using SpscQueue = BasicSpscQueue<SpscQueueDefaultStorage<T>>;

#endif  // I_AM_ATOMIC_SPSC_H_
