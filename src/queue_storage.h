#pragma once
#ifndef I_AM_ATOMIC_QUEUE_STORAGE_H_
#define I_AM_ATOMIC_QUEUE_STORAGE_H_

#include <cstddef>
#include <memory>
#include <utility>

template <class T, class Alloc = std::allocator<T>>
class QueueRingBufferStorage {
 public:
  QueueRingBufferStorage(size_t n, const Alloc &alloc = Alloc()) : alloc_(alloc) {
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

  ~QueueRingBufferStorage() {
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

  bool DiscardOne(size_t head, size_t tail) {
    if (head != tail) {
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

class QueueNoneStorage {
 public:
  bool ConsumeOne(size_t head, size_t tail) { return head != tail; }
  bool DiscardOne(size_t head, size_t tail) { return head != tail; }
  bool ProduceOne(size_t head, size_t tail) { return true; }
};

template <class T>
using QueueDefaultStorage = std::conditional_t<
    std::is_void_v<T>, QueueNoneStorage, QueueRingBufferStorage<T>>;

#endif  // I_AM_ATOMIC_QUEUE_STORAGE_H_
