#include <iostream>
#include <thread>

#include "spsc.h"

int main() {
  std::atomic_size_t counter;
  constexpr size_t n = 10000000;
  std::atomic_bool stop;
  SpscQueue<int> q(100);

  std::thread t1([&] {
    for (;;) {
      int x;
      if (q.Read(x)) {
        counter += x;
      } else if (stop) {
        break;
      }
    }
  });

  std::thread t2([&] {
    for (size_t i = 0; i != n; ++i) {
      while (!q.Write(2)) {
      }
    }
    stop = true;
  });

  t2.join();
  t1.join();

  std::cout << counter << std::endl;

  return 0;
}
