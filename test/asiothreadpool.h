/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#pragma once
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class AsioThreadPool {
public:
  AsioThreadPool(size_t threads) : worker(service) {
    for (size_t i = 0; i < threads; ++i) {
      tpgroup.create_thread(
          boost::bind(&boost::asio::io_service::run, &service));
    }
  }

  ~AsioThreadPool() {
    service.stop();
    try {
      tpgroup.join_all();
    } catch (std::exception &) {
    }
  }

  template <typename Func, typename... Args>
  inline auto post(Func &&func, Args &&... args)
      -> std::future<typename std::result_of<Func(Args...)>::type> {
    auto taskptr = std::make_shared<
        std::packaged_task<typename std::result_of<Func(Args...)>::type()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    service.post([taskptr]() -> void { (*taskptr)(); });
    return taskptr->get_future();
  }

  void run_task(boost::function<void()> task) {
    try {
      task();
    } catch (std::exception &) {
    }
  }

private:
  boost::asio::io_service service;
  boost::asio::io_service::work worker;
  boost::thread_group tpgroup;
};
