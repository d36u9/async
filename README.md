# async
[[License(Boost Software License - Version 1.0)](http://www.boost.org/LICENSE_1_0.txt)]

## Welcome
async is a tiny C++ header-only high-performance library for async calls handled by a thread-pool, which is built on top of an unbounded MPMC lock-free queue.

It's written in C++14 (C++11 support with preprocessor macros).

## Features
* interchangeable with std::async, accepts all kinds of callable instances, like static functions, member functions, functors, lambdas
* dynamically changeable thread-pool size at run-time
* tasks are managed in a lock-free queue
* provided lock-free queue doesn't have restricted limitation as boost::lockfree::queue
* low-latency for the task execution thanks to underlying lock-free queue

## Tested Platforms& Compilers
(old versions of OSs or compilers may work, but not tested)
* Windows 10 Visual Studio 2015+
* Linux Ubuntu 16.04 gcc5.4+/clang 3.8+
* MacOS Sierra 10.12.5 clang-802.0.42

## Getting Started
## Building the test& benchmark

### C++11 compilers
If your compiler only supports C++11, please edit CMakeLists.txt with the following change:
```
set(CMAKE_CXX_STANDARD 14)
#change to 
set(CMAKE_CXX_STANDARD 11)
```

### Build& test with Microsoft C++ REST SDK
If your OS is Windows or has cppresetsdk installed& configured on Linux or Mac, please edit CMakeLists.txt to enable PPL test:
```
option(WITH_CPPRESTSDK "Build Cpprestsdk Test" OFF)
#to
option(WITH_CPPRESTSDK "Build Cpprestsdk Test" ON)
```


### Build for Linux or Mac
```
#to use clang (linux) with following export command
#EXPORT CC=clang-3.8
#EXPORT CXX=clang++-3.8
#run the following to set up release build, (for MasOS Xcode, you can remove -DCMAKE_BUILD_TYPE for now, and choose build type at build-time)
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RELEASE
#now build the release
cmake --build build --config Release
#or debug
cmake --build build --config Debug
#or other builds
cmake --build build --config RelWithDebInfo
cmake --build build --config MinSizeRel
```

### Build for Windows
```
#for VS 2015
cmake -H. -Bbuild -G "Visual Studio 14 2015 Win64"
#or VS 2017
cmake -H. -Bbuild -G "Visual Studio 15 2017 Win64"
#build the release from command line or you can open the project file in Visual Studio, and build from there
cmake --build build --config Release
```

## How to use it in your project/application
simply copy all headers in async sub-folder to your project, and include those headers in your source code.

## Thread Pool Indrodction
### Thread Pool intializations

```
async::threadpool tp; //by default, thread pool size will be the same number of your hardware CPU core/threads
async::threadpool tp(8); //create a thread pool with 8 threads
async::threadpool tp(0); //create a thread pool with no threads available, it's in pause mode
```

### resize the thread pool
```
async::threadpool tp(32);
...//some operations
tp.configurepool(16);// can be called at anytime (as long as tp is still valid) to reset the pool size
                     // no interurption for running tasks
```
### submit the task
*static functions, member functions, functors, lambdas are all supported
```
int foo(int i) { return ++i; }
auto pkg = tp.post(foo, i); //retuns a std::future
pkg.get(); //will block
```

## multi-producer multi-consumer unbounded lock-free queue Indrodction
The design: A simple and classic implementation. It's link-based 3-level depth nested container with local array for each level storage and simulated tagged pointer for linking.
The size of each level, and tag bits can be configured through TRAITS (please see source for details).
The queue with default traits seetings can store up to 1 Trillion elements/nodes (at least 1 Terabyte memory space).

### element type requirements
* nothrow destructible
* optional (better to be true)
  * nothrow constructible
  * nothrow move-assignable

NOTE: the exception thrown by constructor is acceptable. Although it'd be better to keep ctor noexcept if possible.
noexcept detection is turned off by default, it can be turned on by setting  `TRAIT::NOEXCEPT_CHECK` to true.
With `TRAIT::NOEXCEPT_CHECK` on(true), queue will enable exception handling if ctor or move assignment may throw exceptions.


### queue intializations
```
async::queue<T> q; //default constructor, it's unbounded

async::queue<T> q(1000); // pre-allocated 1000 storage nodes, the capcity will increase automatically after 1000 nodes are used
```
### usage
```
// enqueues a T constructed from args, supports the following constructions:
// move, if args is a T rvalue
// copy, if args is a T lvalue, or
// emplacement if args is an initializer list that can be passed to a T constructor
async::queue<T>::enqueue(Args... args)

async::queue<T>::dequeue(T& data) //type T should have move assignment operator,
//e.g.
async::queue<int> q;
q.enqueue(11);
int i(0);
q.dequeue(i);

```
### bulk operations
It's convienent for bulk data, and also can boost the throughput.
exception handling is not available in bulk operations even with `TRAIT::NOEXCEPT_CHECK` being true.
bulk operations are suitable for plain data types, like network/event messages.

```
int a[] = {1,2,3,4,5};
int b[5];
q.bulk_enqueue(std::bengin(a), 5);
auto popcount = q.bulk_dequeue(std::begin(b), 5); //popcount is the number of elemtnets sucessfully pulled from the queue.
//or like the following code:
std::vector<int> v;
auto it = std::inserter(v, std::begin(v));
popcount = q.bulk_dequeue(it, 5);
```

## Unit Test
The unit test code provides most samples for usage.

## Benchmark
NOTE: the results may vary on different OS platforms and hardware.
### thread pool benchmark
The benchmark is a simple demonstration.
NOTE: may require extra config, please see CMakeLists.txt for detailed settings
The test benchamarks the following task/job based async implementation:
* async::threadpool (this library)
* std::async
* boost::async
* AsioThreadPool (my another implementation based on boost::asio, has very stable and good performance, especially on Windows with iocp)
* Microsoft::PPL (pplx from [cpprestsdk](https://github.com/Microsoft/cpprestsdk) on Linux& MacOS) or PPL on windows)

e.g. Windows 7 64bit Intel i7-4790 16GB RAM Visual Studio 2015 Update 3
```
Benchmark Test Run: 1 Producers 7(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 762 ns  max: 966 ns  min: 755 ns avg_task_post: 852 ns
       *std::async (time/task) avg: 1458 ns  max: 1777 ns  min: 1325 ns avg_task_post: 1420 ns
   *Microsoft::PPL (time/task) avg: 1251 ns  max: 1353 ns  min: 1171 ns avg_task_post: 1208 ns
    AsioThreadPool (time/task) avg: 1099 ns  max: 1159 ns  min: 1073 ns avg_task_post: 1064 ns
     *boost::async (time/task) avg: 405040 ns  max: 433880 ns  min: 395135 ns avg_task_post: 405016 ns
...
Benchmark Test Run: 4 Producers 4(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 463 ns  max: 617 ns  min: 409 ns avg_task_post: 427 ns
       *std::async (time/task) avg: 722 ns  max: 767 ns  min: 695 ns avg_task_post: 685 ns
   *Microsoft::PPL (time/task) avg: 722 ns  max: 898 ns  min: 647 ns avg_task_post: 676 ns
    AsioThreadPool (time/task) avg: 576 ns  max: 682 ns  min: 501 ns avg_task_post: 541 ns
     *boost::async (time/task) avg: 143796 ns  max: 167185 ns  min: 137736 ns avg_task_post: 143770 ns
...
Benchmark Test Run: 7 Producers 1(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 325 ns  max: 412 ns  min: 299 ns avg_task_post: 276 ns
       *std::async (time/task) avg: 850 ns  max: 1506 ns  min: 728 ns avg_task_post: 805 ns
   *Microsoft::PPL (time/task) avg: 736 ns  max: 841 ns  min: 693 ns avg_task_post: 688 ns
    AsioThreadPool (time/task) avg: 785 ns  max: 881 ns  min: 728 ns avg_task_post: 409 ns
     *boost::async (time/task) avg: 126887 ns  max: 154059 ns  min: 123282 ns avg_task_post: 126857 ns
```

e.g. Windows 10 64bit Intel i7-6700K 16GB RAM 480GB SSD Visual Studio 2017 (cl 19.11.25507.1 x64)
```
...
Benchmark Test Run: 4 Producers 4(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 454 ns  max: 554 ns  min: 394 ns avg_task_post: 372 ns
       *std::async (time/task) avg: 902 ns  max: 981 ns  min: 828 ns avg_task_post: 681 ns
   *Microsoft::PPL (time/task) avg: 685 ns  max: 753 ns  min: 659 ns avg_task_post: 622 ns
    AsioThreadPool (time/task) avg: 456 ns  max: 492 ns  min: 395 ns avg_task_post: 372 ns
     *boost::async (time/task) avg: 42186 ns  max: 48573 ns  min: 36278 ns avg_task_post: 37202 ns
...

Benchmark Test Run: 6 Producers 2(* not applied) Consumers  with 12000 tasks and run 100 batches
    async::threapool (time/task) avg: 327 ns  max: 400 ns  min: 305 ns avg_task_post: 227 ns
         *std::async (time/task) avg: 896 ns  max: 1009 ns  min: 771 ns avg_task_post: 693 ns
     *Microsoft::PPL (time/task) avg: 712 ns  max: 773 ns  min: 639 ns avg_task_post: 654 ns
      AsioThreadPool (time/task) avg: 485 ns  max: 535 ns  min: 435 ns avg_task_post: 240 ns
       *boost::async (time/task) avg: 36062 ns  max: 39274 ns  min: 34510 ns avg_task_post: 33730 ns
```

e.g. MacOS 10.12.5 clang Intel i7-6700K 16GB RAM 250GB SSD clang-802.0.42 (Microsoft::PPL(cpprestsdk::pplx) is superisingly good compared with other libraries on MacOS)
```
Benchmark Test Run: 1 Producers 7(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 8518 ns  max: 8628 ns  min: 8451 ns avg_task_post: 8395 ns
       *std::async (time/task) avg: 13607 ns  max: 15908 ns  min: 13079 ns avg_task_post: 13470 ns
   *Microsoft::PPL (time/Task) avg: 736 ns  max: 906 ns  min: 650 ns avg_task_post: 707 ns
    AsioThreadPool (time/task) avg: 8597 ns  max: 8706 ns  min: 8526 ns avg_task_post: 8475 ns
     *boost::async (time/task) avg: 12147 ns  max: 12319 ns  min: 11845 ns avg_task_post: 12114 ns
...
Benchmark Test Run: 4 Producers 4(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/Task) avg: 5912 ns  max: 5984 ns  min: 5776 ns avg_task_post: 5777 ns
       *std::async (time/Task) avg: 9730 ns  max: 9995 ns  min: 9374 ns avg_task_post: 9573 ns
   *Microsoft::PPL (time/Task) avg: 376 ns  max: 398 ns  min: 344 ns avg_task_post: 348 ns
    AsioThreadPool (time/Task) avg: 6108 ns  max: 6172 ns  min: 6050 ns avg_task_post: 5975 ns
     *boost::async (time/Task) avg: 8929 ns  max: 9631 ns  min: 8760 ns avg_task_post: 8876 ns
...
Benchmark Test Run: 7 Producers 1(* not applied) Consumers  with 21000 tasks and run 100 batches
  async::threapool (time/task) avg: 3449 ns  max: 3517 ns  min: 3387 ns avg_task_post: 3317 ns
       *std::async (time/Task) avg: 11272 ns  max: 11511 ns  min: 11118 ns avg_task_post: 11109 ns
   *Microsoft::PPL (time/task) avg: 382 ns  max: 489 ns  min: 325 ns avg_task_post: 295 ns
    AsioThreadPool (time/Task) avg: 3917 ns  max: 3973 ns  min: 3872 ns avg_task_post: 3403 ns
     *boost::async (time/Task) avg: 10248 ns  max: 10906 ns  min: 9536 ns avg_task_post: 10192 ns     
```

e.g. Ubuntu 17.04 Intel i7-6700K 16GB RAM 100GB HDD gcc 6.3.0
```
Benchmark Test Run: 1 Producers 7(* not applied) Consumers  with 21000 tasks and run 1 batches
  async::threapool (time/task) avg: 1284 ns  max: 1284 ns  min: 1284 ns avg_task_post: 1224 ns
       *std::async (time/task) avg: 11985 ns  max: 11985 ns  min: 11985 ns avg_task_post: 9780 ns
   *Microsoft::PPL (time/task) avg: 1597 ns  max: 1597 ns  min: 1597 ns avg_task_post: 1572 ns
    AsioThreadPool (time/task) avg: 1445 ns  max: 1445 ns  min: 1445 ns avg_task_post: 1393 ns
     *boost::async (time/task) avg: 4700 ns  max: 4700 ns  min: 4700 ns avg_task_post: 4665 ns
...
Benchmark Test Run: 4 Producers 4(* not applied) Consumers  with 21000 tasks and run 1 batches
  async::threapool (time/task) avg: 383 ns  max: 556 ns  min: 269 ns avg_task_post: 320 ns
       *std::async (time/task) avg: 33691 ns  max: 33691 ns  min: 33691 ns avg_task_post: 30377 ns
   *Microsoft::PPL (time/task) avg: 3718 ns  max: 3718 ns  min: 3718 ns avg_task_post: 3692 ns
    AsioThreadPool (time/task) avg: 555 ns  max: 1036 ns  min: 493 ns avg_task_post: 495 ns
     *boost::async (time/task) avg: 16124 ns  max: 18450 ns  min: 10895 ns avg_task_post: 16075 ns
...
Benchmark Test Run: 7 Producers 1(* not applied) Consumers  with 21000 tasks and run 1 batches
  async::threapool (time/task) avg: 436 ns  max: 436 ns  min: 436 ns avg_task_post: 198 ns
       *std::async (time/task) avg: 36362 ns  max: 36362 ns  min: 36362 ns avg_task_post: 32807 ns
   *Microsoft::PPL (time/task) avg: 3771 ns  max: 3771 ns  min: 3771 ns avg_task_post: 3751 ns
    AsioThreadPool (time/task) avg: 539 ns  max: 569 ns  min: 505 ns avg_task_post: 253 ns
     *boost::async (time/task) avg: 21519 ns  max: 21519 ns  min: 21519 ns avg_task_post: 21480 ns

```
### queue benchmark
The benchmark uses producers-consumers model, and doesn't provide all the detailed measurements.
* async::queue
* boost::lockfree::queue
* boost::lockfree::spsc_queue  (only for single-producer-single-consumer test)

e.g. Windows 10 64bit Intel i7-6700K 16GB RAM 480GB SSD Visual Studio 2017 (cl 19.11.25507.1 x64)
```
Single Producer Single Consumer Benchmark with 10000 Ops and run 1000 batches
Benchmark Test Run: 1 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 28 ns  max: 118 ns  min: 24 ns
          async::queue (time/op) avg: 88 ns  max: 137 ns  min: 49 ns
boost::lockfree::queue (time/op) avg: 142 ns  max: 169 ns  min: 116 ns
boost::lockfree::spsc_queue (time/op) avg: 11 ns  max: 37 ns  min: 10 ns

Benchmark Test Run: 1 Producers 7 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 37 ns  max: 115 ns  min: 32 ns
          async::queue (time/op) avg: 194 ns  max: 271 ns  min: 108 ns
boost::lockfree::queue (time/op) avg: 219 ns  max: 255 ns  min: 200 ns
...
Benchmark Test Run: 6 Producers 2 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 37 ns  max: 77 ns  min: 33 ns
          async::queue (time/op) avg: 101 ns  max: 161 ns  min: 67 ns
boost::lockfree::queue (time/op) avg: 163 ns  max: 196 ns  min: 112 ns

Benchmark Test Run: 7 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 39 ns  max: 78 ns  min: 35 ns
          async::queue (time/op) avg: 57 ns  max: 117 ns  min: 54 ns
boost::lockfree::queue (time/op) avg: 139 ns  max: 177 ns  min: 95 ns
```
e.g. MacOS 10.12.5 Intel i7-6700K 16GB RAM 250GB SSD clang-802.0.42
```
Single Producer Single Consumer Benchmark with 10000 Ops and run 1000 batches
Benchmark Test Run: 1 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 26 ns  max: 105 ns  min: 25 ns
          async::queue (time/op) avg: 119 ns  max: 973 ns  min: 44 ns
boost::lockfree::queue (time/op) avg: 156 ns  max: 173 ns  min: 124 ns
boost::lockfree::spsc_queue (time/op) avg: 11 ns  max: 35 ns  min: 5 ns
...
Benchmark Test Run: 4 Producers 4 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 37 ns  max: 134 ns  min: 32 ns
          async::queue (time/op) avg: 146 ns  max: 155 ns  min: 121 ns
boost::lockfree::queue (time/op) avg: 202 ns  max: 222 ns  min: 177 ns
...
Benchmark Test Run: 7 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 30 ns  max: 173 ns  min: 25 ns
          async::queue (time/op) avg: 120 ns  max: 140 ns  min: 110 ns
boost::lockfree::queue (time/op) avg: 158 ns  max: 833 ns  min: 68 ns
```
e.g. Ubuntu 17.04 Intel i7-6700K 16GB RAM 100GB HDD gcc 6.3.0
```
Single Producer Single Consumer Benchmark with 10000 Ops and run 1000 batches
Benchmark Test Run: 1 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 59 ns  max: 448 ns  min: 28 ns
          async::queue (time/op) avg: 136 ns  max: 278 ns  min: 72 ns
boost::lockfree::queue (time/op) avg: 181 ns  max: 236 ns  min: 58 ns
boost::lockfree::spsc_queue (time/op) avg: 7 ns  max: 167 ns  min: 5 ns

Benchmark Test Run: 1 Producers 7 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 27 ns  max: 289 ns  min: 24 ns
          async::queue (time/op) avg: 186 ns  max: 212 ns  min: 154 ns
boost::lockfree::queue (time/op) avg: 228 ns  max: 288 ns  min: 194 ns
...
Benchmark Test Run: 4 Producers 4 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 33 ns  max: 159 ns  min: 28 ns
          async::queue (time/op) avg: 148 ns  max: 370 ns  min: 125 ns
boost::lockfree::queue (time/op) avg: 183 ns  max: 452 ns  min: 161 ns
...
Benchmark Test Run: 7 Producers 1 Consumers  with 10000 Ops and run 1000 batches
async::queue::bulk(16) (time/op) avg: 32 ns  max: 90 ns  min: 27 ns
          async::queue (time/op) avg: 151 ns  max: 762 ns  min: 122 ns
boost::lockfree::queue (time/op) avg: 194 ns  max: 605 ns  min: 151 ns

```
## coding style
all code has been formated by clang-format. It may be more easy to read in text editor or may be not :)

## Many Thanks to 3rd party libraries and their developers
* [Boost](http://www.boost.org/)
* [Boost CMake](https://github.com/Orphis/boost-cmake) Easy Boost integration in CMake projects!
* [Catch](https://github.com/philsquared/Catch) A powerful test framework for unit test.
* [cpprestsdk](https://github.com/Microsoft/cpprestsdk) The C++ REST SDK is a Microsoft project for cloud-based client-server communication in native code using a modern asynchronous C++ API design.
* [rlutil](https://github.com/tapio/rlutil) provides cross-platform console-mode functions to position and colorize text.
