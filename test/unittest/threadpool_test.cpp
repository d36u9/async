/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#include "catch.hpp"
#include "threadpool.h"
#include <chrono>
#include <iostream>
void noop() {}

int sum(int i, int j) { return i + j; }

struct Task {
  void operator()() {}
  double operator()(int i, int j) { return i + j; }
};

class A {
public:
  A(async::threadpool &pool) : tpool(pool) {}
  void noop() {}
  int sum(int i, int j) { return i + j; }
  int postsum(int i, int j) {
    auto rel = tpool.post(&A::sum, this, i, j);
    return rel.get();
  }
  async::threadpool &tpool;
};

TEST_CASE("threadpool config") {
  async::threadpool tp(4);
  for (; tp.idlesize() != 4;) {
  }
  CHECK(tp.idlesize() == 4);
}

TEST_CASE("threadpool post function") {
  async::threadpool tp(1);
  SECTION("function withouth inputs") {
    auto rel = tp.post(noop);
    rel.get();
  }
  SECTION("function withouth inputs") {
    auto rel = tp.post(sum, 11, 31);
    CHECK(rel.get() == 42);
  }
}

TEST_CASE("threadpool post lambda") {
  async::threadpool tp(1);

  SECTION("lamdba withouth inputs") {
    auto rel = tp.post([]() { return 42; });
    CHECK(rel.get() == 42);
  }

  SECTION("lamdba with inputs") {
    auto rel = tp.post([](int i, int j) { return i + j; }, 11, 22);
    CHECK(rel.get() == 33);
  }
}

TEST_CASE("threadpool post functor") {
  async::threadpool tp(1);
  SECTION("functor withouth inputs") {
    auto rel = tp.post(Task());
    rel.get();
  }

  SECTION("functor with inputs") {
    auto rel = tp.post(Task(), 11, 22);
    CHECK(rel.get() == 33);
  }
}

TEST_CASE("threadpool post class member function") {
  async::threadpool tp(1);
  A a(tp);

  SECTION("class member function without inputs") {
    auto rel = tp.post(&A::noop, a);
    rel.get();
  }

  SECTION("class member function with inputs") {
    auto rel = tp.post(&A::sum, a, 11, 22);
    CHECK(rel.get() == 33);
  }
  SECTION(
      "class member function post task with calling another member function") {
    CHECK(a.postsum(11, 22) == 33);
  }
}