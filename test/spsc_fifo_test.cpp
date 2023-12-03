#include <iostream>
#include <thread>

#include "spsc.h"

int main() {
  std::atomic_bool ok = true;
  constexpr size_t n = 100000;
  std::atomic_bool stop;
  SpscQueue<size_t> q(100);

  std::thread t1([&] {
    size_t expected = 0;
    size_t actual = 0;
    for (;;) {
      if (q.Read(actual)) {
        if (expected != actual) {
          ok = false;
        }
        ++expected;
      } else if (stop) {
        break;
      }
    }
  });

  std::thread t2([&] {
    for (size_t i = 0; i != n; ++i) {
      while (!q.Write(i)) {
      }
    }
    stop = true;
  });

  t2.join();
  t1.join();

  std::cout << (ok ? "PASS" : "FAIL") << std::endl;

  return 0;
}
