#include <iostream>
#include <thread>

#include "spsc.h"

int main() {
  std::atomic_size_t counter;
  constexpr size_t n = 10000000;
  std::atomic_bool stop;
  SpscQueue<> q;

  std::thread t1([&] {
    for (;;) {
      if (q.Read()) {
        ++counter;
      } else if (stop) {
        break;
      }
    }
  });

  std::thread t2([&] {
    for (size_t i = 0; i != n; ++i) {
      while (!q.Write()) {
      }
    }
    stop = true;
  });

  t2.join();
  t1.join();

  std::cout << counter << std::endl;

  return 0;
}
