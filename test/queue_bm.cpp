/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#define NOMINMAX
#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#endif

#ifdef TEST_BOOST
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#endif

#include "queue.h"
#include "rlutil.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
template <typename NAME>
void printResult(NAME &name, std::uint64_t avg, std::uint64_t max,
                 std::uint64_t min) {
  std::stringstream title;
  title << std::setw(22) << name << " (time/op)";
  rlutil::setColor(rlutil::CYAN);
  rlutil::setBackgroundColor(rlutil::RED);
  std::cout << title.str();
  rlutil::setBackgroundColor(rlutil::BLACK);
  rlutil::setColor(rlutil::GREEN);
  std::cout << " avg: ";
  rlutil::setColor(rlutil::YELLOW);
  std::cout << avg << " ns";
  rlutil::setColor(rlutil::GREEN);
  std::cout << "  max: ";
  rlutil::setColor(rlutil::YELLOW);
  std::cout << max << " ns";
  rlutil::setColor(rlutil::GREEN);
  std::cout << "  min: ";
  rlutil::setColor(rlutil::YELLOW);
  std::cout << min << " ns" << std::endl;
  rlutil::setColor(rlutil::WHITE);
}

template <typename Q, typename NAME>
void benchmark(NAME &name, int numProducers, int numConsumers, uint64_t numOps,
               int batches) {
  Q q(0);

  std::chrono::high_resolution_clock::time_point s, e, m;
  std::atomic<uint64_t> sum(0);
  uint64_t sumt(0);
  uint64_t maxt(0);
  uint64_t mint(0);

  for (int b = 0; b < batches; ++b) {
    sum = 0;
    s = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> producers(numProducers);
    for (int t = 0; t < numProducers; ++t) {
      producers[t] = std::thread([&, t] {
        for (int i = t; i < numOps; i += numProducers) {
          q.enqueue(i);
        }
      });
    }

    std::vector<std::thread> consumers(numConsumers);
    for (int t = 0; t < numConsumers; ++t) {
      consumers[t] = std::thread([&, t] {
        uint64_t localSum = 0;
        int popv = -1;
        for (uint64_t i = t; i < numOps; i += numConsumers) {
          for (; !q.dequeue(popv);) {
          }
          localSum += popv;
        }
        sum += localSum;
      });
    }

    for (auto &t : producers) {
      t.join();
    }
    for (auto &t : consumers) {
      t.join();
    }
    e = std::chrono::high_resolution_clock::now();
    uint64_t sum1t =
        std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();
    if (b == 0)
      maxt = mint = sum1t;

    maxt = std::max(maxt, sum1t);
    mint = std::min(mint, sum1t);
    sumt += sum1t;
    assert(numOps * (numOps - 1) / 2 == sum);
  }

  uint64_t timeperOP = sumt / numOps / batches;
  printResult(name, sumt / batches / numOps, maxt / numOps, mint / numOps);
}

template <typename Q, int BULK = 5, typename NAME>
void benchmark_bulk(NAME &name, int numProducers, int numConsumers,
                    uint64_t numOps, int batches) {
  Q q(0);
  std::string newname = name;
  newname += "(" + std::to_string(BULK) + ")";
  std::chrono::high_resolution_clock::time_point s, e, m;
  std::atomic<uint64_t> sum(0);
  uint64_t sumt(0);
  uint64_t maxt(0);
  uint64_t mint(0);
  std::atomic<bool> consumestop(false);
  for (int b = 0; b < batches; ++b) {
    sum = 0;
    s = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> producers(numProducers);
    for (int t = 0; t < numProducers; ++t) {
      producers[t] = std::thread([&, t] {
        int k = 0;
        int bulkdata[BULK];
        for (int i = t; i < numOps; i += numProducers) {
          bulkdata[k++] = i;
          if (k == BULK) {
            q.bulk_enqueue(bulkdata, k);
            k = 0;
          }
        }
        if (k > 0) {
          q.bulk_enqueue(bulkdata, k);
        }
      });
    }

    std::vector<std::thread> consumers(numConsumers);
    for (int t = 0; t < numConsumers; ++t) {
      consumers[t] = std::thread([&, t] {
        uint64_t localSum = 0;
        int popcnt = 0;
        int bulkdata[BULK];
        while (!consumestop || popcnt > 0) {
          popcnt = q.bulk_dequeue(std::begin(bulkdata), BULK);
          if (popcnt > 0) {
            for (int c = 0; c < popcnt; ++c)
              localSum += bulkdata[c];
          }
        }

        sum += localSum;
      });
    }

    for (auto &t : producers) {
      t.join();
    }
    consumestop = true;
    for (auto &t : consumers) {
      t.join();
    }
    e = std::chrono::high_resolution_clock::now();
    uint64_t sum1t =
        std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();
    if (b == 0)
      maxt = mint = sum1t;

    maxt = std::max(maxt, sum1t);
    mint = std::min(mint, sum1t);
    sumt += sum1t;
    assert(numOps * (numOps - 1) / 2 == sum);
  }

  uint64_t timeperOP = sumt / numOps / batches;
  printResult(newname, sumt / batches / numOps, maxt / numOps, mint / numOps);
}

#ifdef TEST_BOOST
template <typename T>
struct boost_queue_adapter : public boost::lockfree::queue<T> {
  boost_queue_adapter(size_t n) : boost::lockfree::queue<T>(n){};
  void enqueue(T &t) { this->push(t); }
  bool dequeue(T &t) { return this->pop(t); }
};

template <typename T, int S = 12000>
struct boost_spsc_queue_adapter : public boost::lockfree::spsc_queue<T> {
  boost_spsc_queue_adapter(size_t) : boost::lockfree::spsc_queue<T>(S){};
  void enqueue(T &t) { this->push(t); }
  bool dequeue(T &t) { return this->pop(t); }
};
#endif

void batch_bm(int numProducers, int numConsumers, int const ops, int batches) {
  rlutil::setColor(rlutil::BLACK);
  rlutil::setBackgroundColor(rlutil::WHITE);
  std::cout << "Benchmark Test Run: " << numProducers << " Producers "
            << numConsumers << " Consumers "
            << " with " << ops << " Ops and run " << batches << " batches";
  rlutil::setBackgroundColor(rlutil::BLACK);
  std::cout << std::endl;
  rlutil::setBackgroundColor(rlutil::BLACK);
  benchmark_bulk<async::queue<int>, 16>("async::queue::bulk", numProducers,
                                        numConsumers, ops, batches);
  benchmark<async::queue<int>>("async::queue", numProducers, numConsumers, ops,
                               batches);

#ifdef TEST_BOOST
  benchmark<boost_queue_adapter<int>>("boost::lockfree::queue", numProducers,
                                      numConsumers, ops, batches);
#endif
}

int main() {
  int threads = std::thread::hardware_concurrency();
  int ops = 10000;
  int batches = 1000;
#ifdef TEST_BOOST
  rlutil::setColor(rlutil::BLACK);
  rlutil::setBackgroundColor(rlutil::WHITE);
  std::cout << "Single Producer Single Consumer Benchmark with " << ops
            << " Ops and run " << batches << " batches";
  rlutil::setBackgroundColor(rlutil::BLACK);
  std::cout << std::endl;
  rlutil::setBackgroundColor(rlutil::BLACK);
  batch_bm(1, 1, ops, batches);
  benchmark<boost_spsc_queue_adapter<int, 10000>>("boost::lockfree::spsc_queue",
                                                  1, 1, ops, batches);
  std::cout << std::endl;
#endif

  for (int i = 1; i < threads; ++i) {
    batch_bm(i, threads - i, ops, batches);
    std::cout << std::endl << std::endl;
  }
  return 0;
}