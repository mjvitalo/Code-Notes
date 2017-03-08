// Not sure if the -mxc16 is needed: it enables 16 byte compare-exchange.
// 
// g++ -g --std=c++14 -mcx16 -o test main.cpp -lpthread -I./
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <array>
#include <vector>
#include <atomic_shared_ptr>

// Example use of atomic_shared_ptr<> to allow producers 
// to update buffers while consumers 
// use and dispose of them too.
namespace
{
  constexpr size_t array_size = 25;
  constexpr size_t buffer_size = 1024*1024;
  using buffers = std::array<jss::atomic_shared_ptr<uint8_t>, array_size>;

  void producer(buffers& array, const size_t index, std::atomic<bool>& run)
  {
    while(run)
    {
      jss::shared_ptr<uint8_t> buf(new uint8_t[buffer_size], std::default_delete<uint8_t[]>());
      memset(buf.get(), 0xa, buffer_size);

      array[index] = buf;

      //std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  }

  void consumer(buffers& array, const size_t index, std::atomic<bool>& run)
  {
    while(run)
    {
      auto p = array[index].load();
      if( p )
        memset(p.get(), 0xb, buffer_size);

      //std::this_thread::sleep_for(std::chrono::milliseconds{7});
    }
  }
}

int main(int argc, char* argv[] )
{
  std::vector<std::thread> threads;

  buffers bufs;
  std::atomic<bool> run{true};
  jss::atomic_shared_ptr<uint8_t> t;
  std::cout << "is lock free: " << t.is_lock_free() << "\n";

  for(size_t i = 0; i < bufs.size(); ++i)
  {
    threads.push_back(std::thread(producer, std::ref(bufs), i, std::ref(run)));
    threads.push_back(std::thread(producer, std::ref(bufs), i, std::ref(run)));

    threads.push_back(std::thread(consumer, std::ref(bufs), i, std::ref(run)));
    threads.push_back(std::thread(consumer, std::ref(bufs), i, std::ref(run)));
  }

  std::this_thread::sleep_for(std::chrono::minutes{5});

  run = false;
  for(auto& t : threads)
    t.join();
  

  return EXIT_SUCCESS;
}
