#pragma once
#ifndef I_AM_ATOMIC_FALSE_SHARING_H_
#define I_AM_ATOMIC_FALSE_SHARING_H_

#if __has_feature(__cpp_lib_hardware_interference_size)
inline constexpr size_t kCachelineAlignment = std::hardware_destructive_interference_size;
#else
inline constexpr size_t kCachelineAlignment = 64;
#endif

template <bool Enabled, class T>
struct EnableCachelineAlignedIf {
  alignas(Enabled ? kCachelineAlignment : alignof(T)) T value;
};

template <class T>
using CachelineAligned = EnableCachelineAlignedIf<true, T>;

#endif  // I_AM_ATOMIC_FALSE_SHARING_H_
