/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#endif

#include <atomic>
#include <future>
#include <thread>

#ifdef TEST_BOOST
#include "asiothreadpool.h"
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/future.hpp>
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#ifdef TEST_PPL
#ifdef _WIN32
#include <ppl.h>
#else
#include <pplx/pplxtasks.h>
#endif
#endif
#include "rlutil.h"
#include "threadpool.h"
#include <cassert>
#include <chrono>

std::atomic<int> count(0);

struct Task {
  Task() noexcept = default;
  void operator()() const { count++; }
};

struct std_async_adapter {
  std_async_adapter(int) {}
  template <typename T>
  auto post(T &&t)
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER <= 1800)
      -> std::future<void>
#endif
  {
    return std::async(std::launch::async, std::forward<T>(t));
  }
};

#ifdef TEST_BOOST
struct boost_async_adapter {
  boost_async_adapter(int) {}
  template <typename T>
  auto post(T &&t)
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER <= 1800)
      -> boost::future<void>
#endif
  {
    return boost::async(boost::launch::async, std::forward<T>(t));
  }
};
#endif

#ifdef TEST_PPL
struct ppl_adapter {
  ppl_adapter(int) {}
  template <typename T>
  auto post(T &&t)
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER <= 1800)
#ifdef _WIN32
      -> concurrency::task<void>
#else
      -> pplx::task<void>
#endif
#endif
  {

#ifdef _WIN32
    return concurrency::create_task(std::forward<T>(t));
#else
    return pplx::create_task(std::forward<T>(t));
#endif
  }
};
#endif

template <typename PREFIX>
void printResult(PREFIX &prefix, std::uint64_t avg, std::uint64_t max,
                 std::uint64_t min, std::uint64_t post) {
  std::stringstream title;
  title << std::setw(18) << prefix << " (time/task)";
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
  std::cout << min << " ns";
  rlutil::setColor(rlutil::GREEN);
  std::cout << " avg_task_post: ";
  rlutil::setColor(rlutil::YELLOW);
  std::cout << post << " ns" << std::endl;
  rlutil::setColor(rlutil::WHITE);
}

//------------------------------------------------------------------------------

template <typename TPOOL, typename RT, typename PREFIX>
void benchmark(PREFIX &prefix, int numProducers, int numConsumers, int jobs,
               int batches) {
  assert(jobs % numProducers ==
         0); // jobs should be exactly integral times of numProducers

  std::uint64_t max, min, sum, postsum;
  std::chrono::high_resolution_clock::time_point s, e, m;
  postsum = min = max = sum = 0;
  count = 0;
  TPOOL pool(numConsumers);
  std::vector<std::vector<RT>> fss(numProducers);
  std::vector<std::thread> producers;

  for (int ri = 0; ri < batches; ++ri) {
    count = 0;

    s = std::chrono::high_resolution_clock::now();

    int i = 0;
    for (auto &fs : fss) {
      fs.reserve(jobs / numProducers);

      producers.emplace_back(std::thread([&]() mutable {
        for (int j = 0; j < jobs / numProducers; ++j)
          fs.emplace_back(pool.post(Task()));
      }));
    }

    for (auto &t : producers)
      t.join();

    m = std::chrono::high_resolution_clock::now();
    for (auto &fs : fss) {
      for (auto &f : fs)
        f.get();
    }

    e = std::chrono::high_resolution_clock::now();

    if (ri == 0)
      min = std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();
    max = std::max(
        max,
        (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(e - s)
            .count());
    min = std::min(
        min,
        (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(e - s)
            .count());
    postsum +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(m - s).count();
    sum += std::chrono::duration_cast<std::chrono::nanoseconds>(e - s).count();
    assert(count == jobs);
    for (auto &fs : fss)
      fs.clear();
    producers.clear();
  }
  printResult(prefix, sum / batches / jobs, max / jobs, min / jobs,
              postsum / batches / jobs);
}

void batch_bm(int numProducers, int numConsumers, int jobs, int batches) {
  rlutil::setColor(rlutil::BLACK);
  rlutil::setBackgroundColor(rlutil::WHITE);
  std::cout << "Benchmark Test Run: " << numProducers << " Producers "
            << numConsumers << "(* not applied) Consumers "
            << " with " << jobs << " tasks and run " << batches << " batches";
  rlutil::setBackgroundColor(rlutil::BLACK);
  std::cout << std::endl;
  rlutil::setBackgroundColor(rlutil::BLACK);

  benchmark<async::threadpool, std::future<void>>(
      "async::threapool", numProducers, numConsumers, jobs, batches);

  benchmark<std_async_adapter, std::future<void>>("*std::async", numProducers,
                                                  numConsumers, jobs, batches);

#ifdef TEST_PPL
#ifdef _WIN32
  benchmark<ppl_adapter, concurrency::task<void>>(
      "*Microsoft::PPL", numProducers, numConsumers, jobs, batches);
#else
  benchmark<ppl_adapter, pplx::task<void>>("*Microsoft::PPL", numProducers,
                                           numConsumers, jobs, batches);
#endif

#endif

#ifdef TEST_BOOST

  benchmark<AsioThreadPool, std::future<void>>("AsioThreadPool", numProducers,
                                               numConsumers, jobs, batches);

  benchmark<boost_async_adapter, boost::future<void>>(
      "*boost::async", numProducers, numConsumers, jobs, batches);

#endif
  std::cout << std::endl << std::endl;
}

int main(int argc, char *argv[]) {
  int threads = std::thread::hardware_concurrency();
  int jobs = 21000;
  int batches = 100;
  for (int i = 1; i < threads; ++i) {
    if (jobs % i == 0) {
      batch_bm(i, threads - i, jobs, batches);
    }
  }
  if (threads == 1)
    batch_bm(1, 1, jobs, batches);

  return 0;
}
