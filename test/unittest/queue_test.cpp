/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#include "catch.hpp"
#include "queue.h"
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <thread>
struct safe_trait : public async::traits {
  static constexpr bool NOEXCEPT_CHECK = true;
};

struct ThrowStruct {
  ThrowStruct() {}
  ThrowStruct(int i) { throw i; }
  ThrowStruct(std::string &&str) {}
  ThrowStruct(std::string const &str) = delete;
  ThrowStruct &operator=(ThrowStruct &&ts) noexcept { return *this; }
};

struct Copy { // copy only
  Copy() {}
  Copy(const Copy &) noexcept {}
  Copy &operator=(Copy const &) noexcept { return *this; }
  Copy(Copy &&) = delete;
  // Copy& operator=(Copy &&) =  delete;
};

TEST_CASE("queue: enque/deque in order") {
  async::queue<int> q;
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

TEST_CASE("queue: copy constructor only type") {
  async::queue<Copy> q;
  Copy c;
  for (int i = 0; i < 10; i++)
    q.enqueue(c);
  q.dequeue(c);
}

TEST_CASE("queue: move constructor only type") {
  async::queue<std::unique_ptr<int>> q;
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

TEST_CASE("queue: SAFE_IMPL true ") {
  async::queue<int, safe_trait> q;
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

TEST_CASE("queue: SAFE_IMPL true with thrown constructor ") {
  async::queue<ThrowStruct, safe_trait> q;
  q.enqueue(1); // supposed to fail due to exception thrown in constructor
  CHECK(q.getNodeCount() == 4);
  q.enqueue(2);
  CHECK(q.getNodeCount() == 4);
  q.enqueue(3);
  CHECK(q.getNodeCount() == 4);
  ThrowStruct t;
  CHECK(q.dequeue(t) == false);
  CHECK(q.dequeue(t) == false);
  CHECK(q.dequeue(t) == false);
}

TEST_CASE("queue: SAFE_IMPL true with no throw constructor ") {
  async::queue<ThrowStruct, safe_trait> q;
  q.enqueue(std::string("str1"));
  CHECK(q.getNodeCount() == 4);
  q.enqueue(std::string("str2"));
  CHECK(q.getNodeCount() == 5);
  q.enqueue(std::string("str3"));
  CHECK(q.getNodeCount() == 6);
  ThrowStruct t;
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == true);
  CHECK(q.dequeue(t) == false); // empty now
}

TEST_CASE("queue: bulk_enqueue ") {
  int a[] = {1, 2, 3, 4, 5};
  async::queue<int> q;
  q.bulk_enqueue(std::begin(a), 5);
  int v(0);
  q.dequeue(v);
  CHECK(v == 1);
  q.dequeue(v);
  CHECK(v == 2);
  q.dequeue(v);
  CHECK(v == 3);
  q.dequeue(v);
  CHECK(v == 4);
  q.dequeue(v);
  CHECK(v == 5);
}

TEST_CASE("queue: bulk_dequeue ") {
  int a[5];
  async::queue<int> q;
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);
  q.enqueue(4);
  q.enqueue(5);

  auto count = q.bulk_dequeue(std::begin(a), 5);
  CHECK(count == 5);
  CHECK(a[0] == 1);
  CHECK(a[1] == 2);
  CHECK(a[2] == 3);
  CHECK(a[3] == 4);
  CHECK(a[4] == 5);
}
TEST_CASE("queue: bulk_dequeue 2") {
  std::vector<int> a;
  a.reserve(5);
  async::queue<int> q;
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);

  auto it = std::inserter(a, a.begin());
  auto count = q.bulk_dequeue(it, 5);
  CHECK(count == 3);
  CHECK(3 == a.size());
  CHECK(a[0] == 1);
  CHECK(a[1] == 2);
  CHECK(a[2] == 3);
}

TEST_CASE("queue: multi-thread test") {
  const int iteration = 888;
  const int tcount = 5;
  async::queue<int> q;
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
  std::atomic<size_t> totalCount(iteration);
  for (int i = 0; i < tcount; ++i) {
    threads.push_back(std::thread([&, i]() mutable {
      for (; !start;)
        ;

      int tsum = 0;
      bool pop = true;
      while (!consumestop || totalCount > 0) {
        int v(0);
        pop = q.dequeue(v);
        if (pop) {
          tsum += v;
          totalCount.fetch_sub(1);
        }
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

TEST_CASE("queue: multi-thread test with bulk operations") {
  const int iteration = 888;
  const int tcount = 5;
  async::queue<int> q;
  std::atomic<bool> start(false);
  std::atomic<bool> consumestop(false);
  std::vector<std::thread> threads;
  std::atomic<int> sum(0);
  for (int i = 0; i < tcount; ++i) {
    threads.push_back(std::thread([&, i]() mutable {
      for (; !start;)
        ;
      std::vector<int> iv;

      for (auto j = i; j < iteration; j += tcount) {
        iv.push_back(j);
        if (static_cast<int>(iv.size()) >= tcount) {
          q.bulk_enqueue(std::begin(iv), iv.size());
          iv.clear();
        }
      }
      if (iv.size() > 0)
        q.bulk_enqueue(iv.begin(), iv.size());
    }));
  }
  std::atomic<size_t> totalCount(iteration);
  for (int i = 0; i < tcount; ++i) {
    threads.push_back(std::thread([&, i]() mutable {
      for (; !start;)
        ;

      int tsum = 0;
      size_t pop = 0;
      while (!consumestop || totalCount > 0) {
        std::vector<int> v;
        v.reserve(5);
        auto it = std::inserter(v, std::begin(v));
        pop = q.bulk_dequeue(it, 5);
        if (pop > 0) {
          for (auto &&i : v)
            tsum += i;
          totalCount.fetch_sub(pop);
        }
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
