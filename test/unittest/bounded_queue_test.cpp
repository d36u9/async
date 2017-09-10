/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#include "bounded_queue.h"
#include "catch.hpp"
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <thread>
struct safe_trait : public async::bounded_traits {
  static constexpr bool NOEXCEPT_CHECK = true;
};

struct ThrowStruct_B {
  ThrowStruct_B() {}
  ThrowStruct_B(int i) {
    if (i == 2)
      throw i;
  }
  ThrowStruct_B(std::string &&str) {}
  ThrowStruct_B(std::string const &str) = delete;
  ThrowStruct_B &operator=(ThrowStruct_B &&ts) noexcept { return *this; }
};

struct Copy_B { // copy only
  Copy_B() {}
  Copy_B(const Copy_B &) noexcept {}
  Copy_B &operator=(Copy_B const &) noexcept { return *this; }
  Copy_B(Copy_B &&) = delete;
};

TEST_CASE("bounded_queue: enque/deque in order") {
  async::bounded_queue<int> q(3);
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);
  int i(0);
  q.dequeue(i);
  CHECK(i == 1);
  q.dequeue(i);
  CHECK(i == 2);
  q.dequeue(i);
  CHECK(i == 3);
}

TEST_CASE("bounded_queue: copy constructor only type") {
  async::bounded_queue<Copy_B> q(10);
  Copy_B c;
  for (int i = 0; i < 10; i++)
    q.enqueue(c);
  q.dequeue(c);
}

TEST_CASE("bounded_queue: move constructor only type") {
  async::bounded_queue<std::unique_ptr<int>> q(2);
  auto v = std::unique_ptr<int>(new int(1));
  CHECK(v.get() != nullptr);
  q.enqueue(std::move(v));
  q.enqueue(std::unique_ptr<int>(new int(2)));
  CHECK(v.get() == nullptr);
  std::unique_ptr<int> null_unique;
  CHECK(null_unique == nullptr);
  q.dequeue(null_unique);
  CHECK(null_unique != nullptr);
  CHECK(*null_unique == 1);
}

TEST_CASE("bounded_queue: queue full and empty test") {
  async::bounded_queue<int> q(2);
  CHECK(q.enqueue(1) == true);
  CHECK(q.enqueue(2) == true);
  CHECK(q.enqueue(3) == false);
  int d(0);

  CHECK(q.dequeue(d) == true);
  CHECK(d == 1);
  CHECK(q.dequeue(d) == true);
  CHECK(d == 2);
  CHECK(q.dequeue(d) == false);
}

TEST_CASE("bounded_queue: blocking_enqueue test") {
  async::bounded_queue<int> q(2);
  CHECK(q.enqueue(1) == true);
  CHECK(q.enqueue(2) == true);
  std::atomic<bool> start(false), end(false);
  std::thread t([&] {
    start = true;
    q.blocking_enqueue(3);
    end = true;
  });
  while (!start)
    continue;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  CHECK(end == false); // t should be blocked here
  int d(0);
  q.dequeue(d);
  CHECK(d == 1);
  while (!end)
    continue;
  CHECK(q.dequeue(d) == true);
  CHECK(d == 2);
  CHECK(q.dequeue(d) == true);
  CHECK(d == 3);
  CHECK(q.dequeue(d) == false);
  t.join();
}

TEST_CASE("bounded_queue: blocking_dequeue test") {
  async::bounded_queue<int> q(2);
  std::atomic<bool> start(false), end(false);
  std::atomic<int> td(0);
  std::thread t([&] {
    int d(0);
    start = true;
    q.blocking_dequeue(d);
    td = d;
    end = true;
  });
  while (!start)
    continue;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  CHECK(end == false); // t should be blocked here
  q.enqueue(42);
  while (!end)
    continue;
  CHECK(td == 42);
  int v(0);
  CHECK(q.dequeue(v) == false);
  t.join();
}

TEST_CASE("bounded_queue: SAFE_IMPL true ") {
  async::bounded_queue<int, safe_trait> q(3);
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);
  int i(0);
  q.dequeue(i);
  CHECK(i == 1);
  q.dequeue(i);
  CHECK(i == 2);
  q.dequeue(i);
  CHECK(i == 3);
}

TEST_CASE("bounded_queue: SAFE_IMPL true with throw constructor ") {
  async::bounded_queue<ThrowStruct_B, safe_trait> q(3);
  CHECK(q.enqueue(1) == true);
  CHECK(q.enqueue(2) == false); // supose to fail
  CHECK(q.enqueue(3) == true);
  ThrowStruct_B t;
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == false); // an invalid element should be skipped
  CHECK(q.dequeue(t) == true);
}

TEST_CASE("bounded_queue: SAFE_IMPL true with no throw constructor ") {
  async::bounded_queue<ThrowStruct_B, safe_trait> q(3);
  CHECK(q.enqueue(std::string("str1")) == true);
  CHECK(q.enqueue(std::string("str2")) == true);
  CHECK(q.enqueue(std::string("str3")) == true);
  ThrowStruct_B t;
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == false); // empty now
}

TEST_CASE("bounded_queue: multi-thread test") {
  const int iteration = 888;
  const int tcount = 5;
  async::bounded_queue<int> q(888);
  std::atomic<bool> start(false);
  std::atomic<bool> consumestop(false);
  std::vector<std::thread> threads;
  std::atomic<int> sum(0);
  for (int i = 0; i < tcount; ++i) {
    threads.push_back(std::thread([&, i]() mutable {
      for (; !start;)
        ;
      for (auto j = i; j < iteration; j += tcount) {
        q.enqueue(j);
      }
    }));
  }
  for (int i = 0; i < tcount; ++i) {
    threads.push_back(std::thread([&, i]() mutable {
      for (; !start;)
        ;

      int tsum = 0;
      bool pop = true;
      while (!consumestop || pop) {
        int v(0);
        pop = q.dequeue(v);
        if (pop)
          tsum += v;
      }
      sum += tsum;
    }));
  }
  start = true;

  int i = 0;
  for (auto &thread : threads) {
    thread.join();
    ++i;

    if (i == tcount) {
      consumestop = true;
    }
  }
  CHECK(sum == iteration * (iteration - 1) / 2);
}
