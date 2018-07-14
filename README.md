# Code-Notes
Examples and explanations from different sources and experts about techniques I have found useful.

# C++
Improvements from using C++14: https://kfrlib.com/blog/how-c14-and-c17-help-to-write-faster-and-better-code-real-world-examples/
[cppcon 2015](https://github.com/cppcon/cppcon2015)

[Writing good C++](https://github.com/CppCon/CppCon2015/blob/master/Keynotes/Writing%20Good%20C++14%20By%20Default/Writing%20Good%20C++14%20By%20Default%20-%20Herb%20Sutter%20-%20CppCon%202015.pdf)

[Use STL algorithms.](https://github.com/CppCon/CppCon2015/blob/master/Presentations/STL%20Algorithms%20in%20Action/STL%20Algorithms%20in%20Action%20-%20Michael%20VanLoon%20-%20CppCon%202015.pdf) Use STL algorithms to simplify code and clarify intent.

Vectors, cache locality, and predictable memory access for performance gains: http://www.stroustrup.com/Software-for-infrastructure.pdf

# RCU
Dissertation: http://www.rdrop.com/users/paulmck/RCU/RCUdissertation.2004.07.14e1.pdf

Read-copy update (RCU) is a synchronization mechanism that was added to the Linux kernel in October of 2002. RCU achieves scalability improvements by allowing reads to occur concurrently with updates. In contrast with conventional locking primitives that ensure mutual exclusion among concurrent threads regardless of whether they be readers or updaters, or with reader-writer locks that allow concurrent reads but not in the presence of updates, RCU supports concurrency between a single updater and multiple readers. RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete. RCU defines and uses efficient and scalable mechanisms for publishing and reading new versions of an object, and also for deferring the collection of old versions. These mechanisms distribute the work among read and update paths in such a way as to make read paths extremely fast. In some cases (non-preemptable kernels), RCU's read-side primitives have zero overhead [LWN](http://lwn.net/Articles/262464/).

Also: http://kukuruku.co/hub/cpp/lock-free-data-structures-the-inside-rcu

#Lock Free / Wait Free Data Structures

CDS is a C++ template library of lock-free and fine-grained algorithms. It contains a collection of concurrent data structure implementations

http://libcds.sourceforge.net/

A nice write up goes this this concurrent queue implementation:

https://github.com/cameron314/concurrentqueue

Boost Lockfree

http://www.boost.org/doc/libs/1_62_0/doc/html/lockfree.html

Herb Sutter's proposal for atomic shared pointers has a  correct  and  ABA safe  implementation  of  a thread safe  singly  linked  list  that supports insert/erase at the front only (like a stack) but also supports finding values in the list. It is written entirely in portable C++ 11, except only that it uses the paper’s proposed atomic\<shared_ptr\<Node\>\>, which is discussed below.

http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4058.pdf


# Hardware Transactional Memory

[C++ Transactional Memory](http://en.cppreference.com/w/cpp/language/transactional_memory)

The idea behind hardware transactional memory is to allow two cores to execute code speculatively if they touch global state, and both should always succeed unless they touch the same bit of global state. For example, if two cores are updating entries in a tree, both will read from the root to a leaf and then each will modify one leaf. In a traditional locking model, you might have a single lock for the entire tree, so that accesses are serialized. With a transactional approach, each starts a transaction, walks to the desired node, and then updates it. The transactions would always succeed, unless both cores tried to update the same node.

In this case, the hardware keeps track of which cache lines have been read from and which have been written to. If another core reads the same cache line addresses as you do, there's no conflict. If either writes to a cache line that another has read or written to, then there's a conflict, and one of the transactions will fail. (Which one fails is defined by implementation.)

Easy to use synchronization construct
  -As easy to use as coarse-grain locks
  -Programmer declares, system implements

Often performs as well as fine-grain locks
   - Automatic read-read concurrency & fine-grain concurrency

Failure atomicity & recovery
   -No lost locks when a thread fails
   -Failure recovery = transaction abort + restart

Composability
  - Safe & scalable composition of software modules
  
```C++
#include <iostream>
#include <vector>
#include <thread>
int f()
{
    static int i = 0;
    synchronized { // begin synchronized block
        std::cout << i << " -> ";
        ++i;       // each call to f() obtains a unique value of i
        std::cout << i << '\n';
        return i; // end synchronized block
    }
}
int main()
{
    std::vector<std::thread> v(10);
    for(auto& t: v)
        t = std::thread([]{ for(int n = 0; n < 10; ++n) f(); });
    for(auto& t: v)
        t.join();
}
```
References

    - http://www.cs.cmu.edu/afs/cs/academic/class/15418-s12/www/lectures/20_transactionalmem.pdf
    - http://www.quepublishing.com/articles/article.aspx?p=2142912
    - http://en.cppreference.com/w/cpp/language/transactional_memory

# Handle Signals
Block signals on all threads so we don't have to worry about interrupted system calls and can be predictable about what thread will handle a signal.
```C++
// Block signals on all threads we create so that we don't
// have to worry about interrupted system calls and such things.
// Handle our signals in a thread in keeping with best practices
// for using signal with threads. See Butenhof, pg. 229.
//
// boost::asio::detail::posix_signal_blocker block_all_signals; // boost example to block everything.
//
// sigset_t new_mask, old_mask_; // Manual block everything.
// sigfillset(&new_mask);
// const bool blocked_ = (pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask_) == 0);
 
// Block a few signals and listen for them via sigwait().
sigset_t oldmask{}, mask{};
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGQUIT);
sigaddset(&mask, SIGTERM);
sigaddset(&mask, SIGUSR1);
int err{};
if( (err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask )) != 0 )
{
  perror("unable to set signal mask");
  exit(EXIT_FAILURE);
}
 
auto signal_handler = [](sigset_t& mask){
  int err{}, signo{};
  // Always use sigwait to work with asynchronous signals within
  // threaded code. [Butenhof, pg. 227] Synchronous signals include
  // SIGFPE, SIGSEGV, and SIGTRAP and are delivered to the thread
  // that caused the hardware exception always, regardless.
  err = sigwait( &mask, &signo );
  if( err )
  {
    perror("Failure waiting on signals.");
    std::abort();
  }
  switch( signo ){
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      exit_signal_handler(signo);
      break;
    default:
      psignal( signo, "Unexpected signal received." );
      exit(EXIT_FAILURE);
  }
  };
std::thread sig_handler( signal_handler, std::ref(mask) );
sig_handler.detach();
```
or with Boost perhaps you can do the following
```C++
#include <boost/asio.hpp>
#include <signal.h>
// Block signals on all threads we create so that we don't
// have to worry about interrupted system calls and such things.
// Handle our signals in a thread in keeping with best practices
// for using signal with threads. See Butenhof, pg. 229.
boost::asio::detail::posix_signal_blocker block_all_signals;
 
auto signal_handler = [](boost::asio::io_service& io_service ){
   sigset_t mask{};
   sigemptyset(&mask);
   sigaddset(&mask, SIGINT);
   sigaddset(&mask, SIGQUIT);
   sigaddset(&mask, SIGTERM);
   int err{}, signo{};
   // Always use sigwait to work with asynchronous signals within
   // threaded code. [Butenhof, pg. 227] Synchronous signals include
   // SIGFPE, SIGSEGV, and SIGTRAP and are delivered to the thread
   // that caused the hardware exception always, regardless.
   err = sigwait( &mask, &signo );
   if( err )
   {
     perror("Failure waiting on signals.");
     std::abort();
   }
   switch( signo ){
      case SIGINT:
      case SIGQUIT:
      case SIGTERM:
        io_service.stop();
        break;
      default:
        psignal( signo, "Unexpected signal received." );
        exit(EXIT_FAILURE);
    }
};
 
daemon(0,0);
boost::asio::io_service io_service;
std::thread sig_handler(signal_handler, std::ref(io_service));
sig_handler.detach();
 
// We have on acceptor listening for connection attemps.
tcp_acceptor server(io_service);
 
// Run until SIGINT or SIGTERM is received.
io_service.run();
```

 
# Edit Distance & Levenshtein Distance

We used this algorithm in the in some clustering work.

    https://en.wikipedia.org/wiki/Levenshtein_distance
    https://github.com/erikerlandson/edit_distance
    https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
    http://people.cs.pitt.edu/~kirk/cs1501/Pruhs/Fall2006/Assignments/editdistance/Levenshtein%20Distance.htm
```C++
// The "difference", or "distance", between two clusters of ids is defined
// in terms of the Levenshtein distance. The Levenshtein distance is a string
// metric for measuring the difference between two sequences. Informally, the
// Levenshtein distance between two words is the minimum number of single-character
// edits (i.e. insertions, deletions or substitutions) required to change one
// word into the other.
//
// This is also known, in the general case, as "edit distance" and we could
// just use the boost Sequence Algorithms.
//
// Note that it is assumed clusters are sorted by call id before the comparison.
//
// (Note: https://github.com/erikerlandson/edit_distance)
using cluster = std::vector<unsigned>;
 
inline unsigned
Levenshtein_distance(const cluster& s1, const cluster& s2)
{
  const std::size_t len1 = s1.size(), len2 = s2.size();
  std::vector<unsigned> col(len2+1), prevCol(len2+1);
  std::iota( prevCol.begin(), prevCol.end(), 0 );
 
  for(unsigned i = 0; i < len1; i++)
  {
    col[0] = i+1;
    for(unsigned j = 0; j < len2; j++)
    {
      col[j+1] = std::min({ prevCol[1 + j] + 1, col[j] + 1, prevCol[j] + (s1[i]==s2[j] ? 0 : 1) });
    }
    col.swap(prevCol);
  }
  return prevCol[len2];
}
```
# Container Sorted Insert
```C++
using container = std::vector<int>;
 
void insert( container &cont, int value ) {
    auto it = std::lower_bound( cont.begin(), cont.end(),
       value, std::greater<int>() ); // find proper position in descending order
    cont.insert( it, value ); // insert before iterator it
}
```
# Access Once

Not needed with C11/C++11 as you can just use an atomic type in general, perhaps applicable if you are implementing something like [RCU](http://www.rdrop.com/~paulmck/RCU/) with [memory_order_consume](https://github.com/CppCon/CppCon2015/blob/master/Presentations/The%20Sad%20Story%20of%20memory_order_consume/The%20Sad%20Story%20of%20memory_order_consume%20-%20Paul%20E.%20McKenney%20-%20CppCon%202015.pdf). Never use [volatile for synchronization](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-volatile).

Useful when referencing data in a multi-threaded app without memory barriers or other synchronization elements active in the function. Maybe a better name would be FORCE_ACCESS_HERE(), as suggested in the LWN article comments by mathstuf.

This avoids macros, works with volatile objects because the redundant volatile is legal, the type deduction and reference removal is implicit in the template signature, and it avoids the redundant address-of and deference operators common in another approaches (static casting between reference types is defined to do the same things as taking the address, casting and then dereferencing).
```C++
template<typename T>
inline T volatile &access_once(T &t) {
    return static_cast<T volatile &>(t);
}
```
See:

   - http://stackoverflow.com/questions/12393562/c-access-once
   - https://lwn.net/Articles/508991/
   - McKenney, pg 115

# Processor usage
Logic is from: http://stackoverflow.com/questions/3017162/how-to-get-total-cpu-usage-in-linux-c.
```C++
// g++ -std=c++11 main.cpp to compile
// run by:  "watch ./a.out"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <iterator>
#include <thread>
#include <chrono>
#include <cmath>
 
enum cpu_usage
{
  user_mode = 0,
  user_mode_low_prio,
  system_mode,
  idle,
  iowait,
  irq,
  soft_irq,
  steal,
  guest
};

enum class totals
{
  jiffies = 0, system_usage, io_irq, steal, guest
};
 
typedef std::tuple< double, double, double, double, double > tuple;
 
template< totals val >
double difference( tuple& t2, tuple& t1 )
{
  const int which = static_cast<int>(val);
  return std::fdim(std::get<which>(t2), std::get<which>(t1)); 
}

tuple cpu_load()
{
   // Read the first line in /pro/stat to get the values  we need.
   std::string cpu_stats;
   std::fstream sys_stats( "/proc/stat", std::ios::in );
   std::getline( sys_stats, cpu_stats );
 
   // Strip of the leading " cpu ", 5 characters, that is present on the first
   // line when you cat "/proc/stat".
   cpu_stats = cpu_stats.substr( 5 );
   std::istringstream stream(cpu_stats);
   std::vector<double> values(
     (std::istream_iterator<double>(stream)),
     (std::istream_iterator<double>()));
 
    if( values.size() <= cpu_usage::guest )
    {
    std::cerr << "/proc/stat has unanticipated format: only "
        << values.size() << " columns.\n";
    exit(EXIT_FAILURE);
    }
 
    // The values are how many jiffies spent in:  user mode, user mode with low
    // priority, system mode, idle, iowait, irq, softirq, steal, guest.
    auto total_jiffies = std::accumulate( values.begin(), values.end(), 0);
 
    auto system_usage = values[cpu_usage::user_mode] +
                    values[cpu_usage::user_mode_low_prio] +
                    values[cpu_usage::system_mode] +
                    values[cpu_usage::irq] +
                    values[cpu_usage::soft_irq];
 
    auto io_irq = values[cpu_usage::irq] +
              values[cpu_usage::iowait] +
              values[cpu_usage::soft_irq];
 
    // "steal" is stolen time: the time spent in other operating systems when
    // running in a virtualized environment.
    auto steal = values[cpu_usage::steal];
 
    // "guest" is the ninth value, it is the time spent funning a virtual CPU
    // for a guest operating system.
    auto guest = values[cpu_usage::guest];
 
    return std::make_tuple( total_jiffies, system_usage, io_irq, steal, guest );
}

int main( int argc, char** argv )
{
  auto time1 = cpu_load();
  std::this_thread::sleep_for( std::chrono::seconds(1) );
  auto time2 = cpu_load();
 
  auto total  = difference<totals::jiffies>(time2, time1);
  auto work   = difference<totals::system_usage>(time2, time1);
  auto io_irq = difference<totals::io_irq>(time2, time1);
  auto stole  = difference<totals::steal>(time2, time1);
  auto guest  = difference<totals::guest>(time2, time1);
  auto percent = (work/total)*100;
  std::cout << "CPU Load: " << percent << "\n";
 
  if( io_irq )
    std::cout << "CPU time waiting for IO & irq handling "
            << (io_irq/total)*100 << "%\n";
 
  if( stole )
    std::cout << "CPU percent used for another VM: " << (stole/total)*100;
 
  if( guest )
    std::cout << "CPU percent used for running a guest OS: " << (guest/total)*100;
 
  return EXIT_SUCCESS;
}
```
# Thread Safe Interface Idiom

The thread safe interface is detailed in this paper http://www.cs.wustl.edu/~schmidt/PDF/locking-patterns.pdf and is an approach to simplifying code and reducing lock contention. This approaches avoids dead locks. From the paper:

    Multi-threaded components typically contain multiple inter-
    face and implementation methods that perform computations
    on state that is encapsulated by the component. Component
    state is protected by a lock that prevents race conditions by
    serializing methods in the component that access the state.
    Component methods often call each other to carry out their
    computations. In multi-threaded components with poorly de-
    signed intra-component method invocation behavior, how-
    ever, the following forces will be unresolved:
    Avoid Self-deadlock:
    Thread-safe components should be designed to avoid ‘self-deadlock.’
    Self-deadlock will occur if one component method acquires a non-recursive
    component lock and calls another method that tries to reacquire the
    same lock.
    Minimal locking overhead:
    Thread-safe components should be designed to incur only the
    minimal locking over-head necessary to prevent race conditions.
    However, if a recursive component lock is selected to avoid the
    self-deadlock problem outlined above, additional overhead will be
    incurred to acquire and release the lock multiple times across intra-
    component method calls.
```C++
#include <thread>
#include <cstdlib>
#include <mutex>
#include <algorithm>
#include <vector>
#include <iostream>
 
// Thread safe interface example.
struct active_object
{
  using Guard = std::lock_guard<std::mutex>;
 
  // Public methods acquire the lock and
  // call to the private implementation methods
  // passing the guard.
  void do_work( const int& item )
  {
    Guard g(mutex_);
    do_work( item, g );
  }
 
  private:
 
  // Private methods are always be called with the lock
  // held.
  void do_work( const int& item, Guard& g)
  {
    // We are free to call any private method.
    if( work_okay_to_do(item, g))
    {
      work_.push_back(item);
    }
    else
    {
      std::cout << "ignoring " << item << "\n";
    }
  }
   
  // Passing the guard ensures the lock is held: we
  // know we are the only thread accessing the data.
  bool work_okay_to_do( const int& item, Guard& ) const
  {
    return std::end(work_) ==
      std::find(std::begin(work_), std::end(work_), item);
  }
 
  mutable std::mutex mutex_;
  std::vector<int> work_;
};
 
void do_stuff(active_object& ao)
{
  for( auto a : {1,2,4,3,5})
  {
    ao.do_work(a);
  }
}
 
int main()
{
  active_object busy_guy;
  std::thread a( do_stuff, std::ref(busy_guy) );
  std::thread b( do_stuff, std::ref(busy_guy) );
  std::thread c( do_stuff, std::ref(busy_guy) );
 
  a.join();
  b.join();
  c.join();
 
  return EXIT_SUCCESS;
}
```

# Concurrent<T> & Active Objects

## Active Object
The Active Object design pattern decouples method execution from method invocation to enhance concurrency and simplify synchronized access to an object that resides in its own thread of control.

    What it is http://www.cs.wustl.edu/~schmidt/PDF/Act-Obj.pdf
    Herb Sutter's take http://www.drdobbs.com/parallel/prefer-using-active-objects-instead-of-n/225700095
    Herb Sutter's Concurrent<T> https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism and his slides https://onedrive.live.com/?cid=F1B8FF18A2AEC5C5&id=F1B8FF18A2AEC5C5%211176&parId=root&o=OneUp
    Concurrent Object Wrappers https://juanchopanzacpp.wordpress.com/2013/03/01/concurrent-object-wrapper-c11/
    Normal C++ Active Object Wrapper https://github.com/KjellKod/active-object

## Concurrent<T>
Aka "Active Object".

Herb Sutter's Concurrent<T> https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism and his slides https://onedrive.live.com/?cid=F1B8FF18A2AEC5C5&id=F1B8FF18A2AEC5C5%211176&parId=root&o=OneUp
  
  
# Parametrized Construction and Destruction in Smart Pointers: shared_ptr<> array
## Constructing a std::unique_ptr<> with a deleter.
```C++
template <typename T, typename D, typename ...Args>
std::unique_ptr<T,D> make_unique_with_deleter( D deleter, Args&&...args )
{
    return std::unique_ptr<T,D>(
        new T( std::forward<Args>(args)... ),
        std::move( deleter ) );
}
```

## Shared pointer to array
```C++
std::shared_ptr<int> sp( new int[10], std::default_delete<int[]>() );
```

## With std::unique_ptr<>
```C++
#include <cstdio>
#include <string>
#include <memory>
 
// Custom deleter for a pointer you would like to treat as an array.
auto deleter = [](char*p){ my_equivalent_to_free(p); };
std::unique_ptr<uint8_t*, decltype(deleter)> charup( (char*)(malloc(5), deleter);
charup[0] = 'a';
 
// std::unique pointer only invokes the destructor if the
// pointer is non-null. std::shared_ptr<> always calls the
// destructor.
typedef std::unique_ptr<FILE> file_ptr;
 
// boost::shared_ptr<> & std::shared_ptr<> will always call the destroy function,
// and pclose() will segfault on NULL.
void shared_pipe_destroy( void* p )
{
  if( p ) pclose( p );
}
 
// Note that you don't have to write a check close function
// for different destroy, you can just write one using boost::bind
// and pass the destroy function as a paremeter.
// void checked_close( void* p, void (*f)(void*) );
// shared_ptr<FILE> f( ..., boost::bind( checked_close, ... ));
// or
// shared_ptr<FILE> f( ..., []( void* p){ if(p) pclose(p); });
 
void shared_file_close( void* p )
{
  if( p ) flclose( p );
}
 
void writeString( file_ptr fp, std::string s )
{
  fwrite( s.c_str(), sizeof(char), s.size(), fp.get() );
}
 
int main( int, char*\[\] )
{
  file_ptr fp( fopen("2003-smart-pointers-in-tr1.txt", "w"), shared_file_close );
  writeString( fp, "Chocolate cake is awesome.\n" );
  writeString( fp, "And so is banana bread!\n" );
 
  std::string curl_request = ".....";
  boost::shared_ptr<FILE> request( ::popen( curl_request.c_str(), "re" ), shared_pipe_destroy );
  if( !request )
  {
      boost::system::error_code ec (errno, boost::system::system_category ());
      LSError( lslog, "", "%s:%d popen - %s. Unable to execute provisioning request.\n",
              __FUNCTION__, __LINE__, ec.message().c_str());
  }
 
  return 0; // No need to fclose, pclose the file here\!
}
```

# Errors

The header <system_error> defines types and functions used to report error conditions originating from the operating system, streams I/O, std::future, or other low-level APIs.
```C++
#include <system_error>
#include <string>
#include <iostream>
  
int main()
{
    // http://en.cppreference.com/w/cpp/error/system_error
    const std::system_error err(errno, std::system_category());
    std::cout << err.what() << '\n';
     
    // http://en.cppreference.com/w/cpp/error/error_condition
    const std::error_condition e = make_error_condition(std::errc(errno));
    if(e)
      std::cout << "trouble";
    std::cout << e.message() << "\n";
}
```

# Count Cores & CPUs
See http://www.richweb.com/cpu_info for details.
From here:

    Num of CPU Sockets: cat /proc/cpuinfo | grep "physical id" | sort | uniq | wc -l
    Total number of cores: cat /proc/cpuinfo | egrep "core id|physical id" | tr -d "\n" | sed s/physical/
    nphysical/g | grep -v ^$ | sort | uniq | wc -l
    Also this helps to see the processor, core, socket relationship:    egrep "(( id|processo).*:|^ *$)" /proc/cpuinfo
    There is a */sys* interface also: /sys/devices/system/cpu
```C++
#include <sched.h>
 
void report_num_cpus()
{
    cpu_set_t cs;
    CPU_ZERO(&cs);
    unsigned count(0);
 
    // Won't adjust for hyperthreading: with hyperthreading
    // it reports double the number of actual cores.
    sched_getaffinity(0, sizeof(cs), &cs );
    for( size_t i = 0; i < sizeof(cs); ++i )
        if( CPU_ISSET(i, &cs) )
            ++count;
 
    std::cout << "Number of cpus: " << count << "\n";
}
```
# Processor usage

Logic is from: http://stackoverflow.com/questions/3017162/how-to-get-total-cpu-usage-in-linux-c
```C++
// g++ -std=c++0x main.cpp to compile
// run by:  "watch ./a.out"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <iterator>
#include <thread>
#include <chrono>
#include <cmath>
 
enum cpu_usage
{
  user_mode = 0,
  user_mode_low_prio,
  system_mode,
  idle,
  iowait,
  irq,
  soft_irq,
  steal,
  guest
};
 
enum class totals
{
  jiffies = 0, system_usage, io_irq, steal, guest
};
 
typedef std::tuple< double, double, double, double, double > tuple;
 
template< totals val >
double difference( tuple& t2, tuple& t1 )
{
  const int which = static_cast<int>(val);
  return std::fdim(std::get<which>(t2), std::get<which>(t1)); 
}
 
tuple cpu_load()
{
   // Read the first line in /pro/stat to get the values  we need.
   std::string cpu_stats;
   std::fstream sys_stats( "/proc/stat", std::ios::in );
   std::getline( sys_stats, cpu_stats );
 
   // Strip of the leading " cpu ", 5 characters, that is present on the first
   // line when you cat "/proc/stat".
   cpu_stats = cpu_stats.substr( 5 );
   std::istringstream stream(cpu_stats);
   std::vector<size_t> values(
     (std::istream_iterator<size_t>(stream)),
     (std::istream_iterator<size_t>()));
 
    if( values.size() <= cpu_usage::guest )
    {
      std::cerr << "/proc/stat has unanticipated format: only "
        << values.size() << " columns.\n";
      exit(EXIT_FAILURE);
    }
 
    // The values are how many jiffies spent in:  user mode, user mode with low
    // priority, system mode, idle, iowait, irq, softirq, steal, guest.
    auto total_jiffies = std::accumulate( values.begin(), values.end(), 0);
 
    auto system_usage = values[cpu_usage::user_mode] +
                    values[cpu_usage::user_mode_low_prio] +
                    values[cpu_usage::system_mode] +
                    values[cpu_usage::irq] +
                    values[cpu_usage::soft_irq];
 
    auto io_irq = values[cpu_usage::irq] +
              values[cpu_usage::iowait] +
              values[cpu_usage::soft_irq];
 
    // "steal" is stolen time: the time spent in other operating systems when
    // running in a virtualized environment.
    auto steal = values[cpu_usage::steal];
 
    // "guest" is the ninth value, it is the time spent funning a virtual CPU
    // for a guest operating system.
    auto guest = values[cpu_usage::guest];
 
    return std::make_tuple( total_jiffies, system_usage, io_irq, steal, guest );
}
 
int main( int argc, char** argv )
{
  auto time1 = cpu_load();
  std::this_thread::sleep_for( std::chrono::seconds(1) );
  auto time2 = cpu_load();
 
  auto total  = difference<totals::jiffies>(time2, time1);
  auto work   = difference<totals::system_usage>(time2, time1);
  auto io_irq = difference<totals::io_irq>(time2, time1);
  auto stole  = difference<totals::steal>(time2, time1);
  auto guest  = difference<totals::guest>(time2, time1);
  auto percent = (work/total)*100;
  std::cout << "CPU Load: " << percent << "\n";
 
  if( io_irq )
    std::cout << "CPU time waiting for IO & irq handling "
            << (io_irq/total)*100 << "%\n";
 
  if( stole )
    std::cout << "CPU percent used for another VM: " << (stole/total)*100;
 
  if( guest )
    std::cout << "CPU percent used for running a guest OS: " << (guest/total)*100;
 
  return EXIT_SUCCESS;
}
```

# JSON example
With the normal C++ json library you can just write normal C++: Json can be treated as string literals, or STL like containers (https://github.com/nlohmann/json/#stl-like-access), with ease.
```C++
#include <sstream>
#include "json.hpp" // Normal C++ Json Library: https://github.com/nlohmann/json/
                    // https://github.com/nlohmann/json/#examples
 
void example() {
  // Write json.
  const nlohmann::json ping =
  {
    {"messageId", to_string(uuid)},
    {"event", "Ping"},
    {"source", "mcu"},  
    {"sourceId", i.binding_key()}, 
    {"version", "1.0"},
    {"timestamp", time_stamp.count()}, // Timestamp: milliseconds since epoch.
  };
  // create object from string literal
  json j = "{ \"happy\": true, \"pi\": 3.141 }"_json;
 
  // or even nicer with a raw string literal
  auto j2 = R"(
  {
    "happy": true,
    "pi": 3.141
  }
  )"_json;
  // deserialize from standard input
  json j;
  std::cin >> j;
 
  // serialize to standard output
  std::cout << j;
 
  // the setw manipulator was overloaded to set the indentation for pretty printing
  std::cout << std::setw(4) << j << std::endl; 
}
```
reference : https://github.com/nlohmann/json/#examples

# UTF 8 Handling
```C++
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <iomanip>
 
// utility function for output
void hex_print(const std::string& s)
{
    std::cout << std::hex << std::setfill('0');
    for(unsigned char c : s)
        std::cout << std::setw(2) << static_cast<int>(c) << ' ';
    std::cout << std::dec << '\n';
}
 
int main()
{
    // wide character data
    std::wstring wstr =  L"z\u00df\u6c34\U0001d10b"; // or L"zß水𝄋"
 
    // wide to UTF-8
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
    std::string u8str = conv1.to_bytes(wstr);
    std::cout << "UTF-8 conversion produced " << u8str.size() << " bytes:\n";
    hex_print(u8str);
 
    // wide to UTF-16le
    std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>> conv2;
    std::string u16str = conv2.to_bytes(wstr);
    std::cout << "UTF-16le conversion produced " << u16str.size() << " bytes:\n";
    hex_print(u16str);
}
```
# File Writer
boost::asio::posix::stream_descriptor can be initialized with a file descriptor to start an non-blocking I/O operation on that file descriptor. In the example, stream is linked to the file descriptor STDOUT_FILENO to write a string asynchronously to the standard output stream.
Posix Stream Descriptor
```C++
#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <unistd.h>
 
using namespace boost::asio;
 
int main()
{
  io_service ioservice;
 
  posix::stream_descriptor stream{ioservice, STDOUT_FILENO};
  auto handler = [](const boost::system::error_code&, std::size_t) {
    std::cout << ", world!\n";
  };
  async_write(stream, buffer("Hello"), handler);
 
  ioservice.run();
}
```

# ASIO & Threads
```C++
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <thread>
#include <iostream>
 
using namespace boost::asio;
 
int main()
{
  io_service ioservice1;
  io_service ioservice2;
 
  steady_timer timer1{ioservice1, std::chrono::seconds{3}};
  timer1.async_wait([](const boost::system::error_code &ec)
    { std::cout << "3 sec\n"; });
 
  steady_timer timer2{ioservice2, std::chrono::seconds{3}};
  timer2.async_wait([](const boost::system::error_code &ec)
    { std::cout << "3 sec\n"; });
 
  std::thread thread1{[&ioservice1](){ ioservice1.run(); }};
  std::thread thread2{[&ioservice2](){ ioservice2.run(); }};
  thread1.join();
  thread2.join();
}
```

# Naive Sharing

Some simple example code of sharing a socket between two (or more) processes. parent process would call bind(), listen() etc, the child processes would just process requests by accept(), send(), recv() etc. This is just an example to show some networking code.

http://stackoverflow.com/questions/670891/is-there-a-way-for-multiple-processes-to-share-a-listening-socket
```C++
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <string>
using boost::asio::ip::tcp;
 
int main()
{
  boost::asio::io_service io_service;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8888));
 
  const std::string msg = fork() ? "hello from parent" : "hello from child";
  while(true)
  {
    boost::asio::ip::tcp::socket socket(io_service);
    acceptor.accept(socket);
    std::cout << msg << ": got connection\n";
    boost::asio::write(socket, boost::asio::buffer(msg + "\n"));
  }
 
  return EXIT_SUCCESS;
}
```

# Leaky Bucket
The leaky bucket is an algorithm that may be used to determine whether some sequence of discrete events conforms to defined limits on their average and peak rates or frequencies. Wikipedia: https://en.wikipedia.org/wiki/Leaky_bucket

```C++
namespace lb // leaky bucket
{
  // Class std::chrono::steady_clock represents a monotonic clock.
  // The time points of this clock cannot decrease as physical time
  // moves forward. This clock is not related to wall clock time
  // (for example, it can be time since last reboot), and is most
  // suitable for measuring intervals.
  using clock = std::chrono::steady_clock;
 
  struct token_bucket
  {
    double tokens_ = 0;
    double capacity_ = 0;
    double fill_rate_ = 0;
    clock::time_point time_stamp_{clock::now()};
  };
 
    bool consume(const double& tokens, token_bucket& tb)
  {
    if(tokens <= tb.tokens_)
      tb.tokens_ -= tokens;
    else
      return false;
    return true;
  }
 
  auto get_tokens(token_bucket& tb) noexcept
  {
    const auto now = clock::now();
 
    if(tb.tokens_ < tb.capacity_)
    {
      const auto delta = tb.fill_rate_*(now - tb.time_stamp_).count();
      tb.tokens_ = std::min(tb.capacity_, tb.tokens_ + delta);
    }
 
    return tb.time_stamp_ = clock::now(), tb.tokens_;
  }
}
```
# Fold Expressions
http://en.cppreference.com/w/cpp/language/fold
```C++
#include <future>
#include <iostream>
#include <iostream>
#include <vector>
#include <climits>
#include <cstdint>
#include <type_traits>
#include <utility>
 
template<typename... Args>
void log(Args&&... args) {
    (std::cout <<  ...  << args) << std::endl;
}
 
template<typename... Args>
std::future<void> log_async(Args&&... args) {
  return std::async(std::launch::async, [args...] { log(args...); });
}
     
template<typename T, typename... Args>
void push_back_vec(std::vector<T>& v, Args&&... args)
{
    (v.push_back(args), ...);
}
  
// Compile-time endianness swap based on http://stackoverflow.com/a/36937049
// "The advantage of this form is that because it doesn't use loops or recursion,
// you're pretty much guaranteed to get optimal assembly output - on x86-64,
// clang even manages to work out to use the bswap instruction."
template<class T, std::size_t... N>
constexpr T bswap_impl(T i, std::index_sequence<N...>) {
  return (((i >> N*CHAR_BIT & std::uint8_t(-1)) << (sizeof(T)-1-N)*CHAR_BIT) | ...);
}
 
template<class T, class U = std::make_unsigned_t<T>>
constexpr U bswap(T i) {
  return bswap_impl<U>(i, std::make_index_sequence<sizeof(T)>{});
}
 
int main()
{
    auto f = log_async(1, 2, 3);
    f.wait();
 
    std::vector<int> v;
    push_back_vec(v, 6, 2, 45, 12);
    push_back_vec(v, 1, 2, 9);
    for (int i : v) std::cout << i << ' ';
 
    static_assert(bswap<std::uint16_t>(0x1234u)==0x3412u);
    static_assert(bswap<std::uint64_t>(0x0123456789abcdefULL)==0xefcdab8967452301ULL);
}
```
# Subexpressions of an initialization list expression are evaluated in order.
```C++
int a[] = {get1(), get2()} will execute get1 before executing get2. We use the comma operator below to support operations which do not return a value.
//To call do() on every argument, I can do something like this:
template <class... Args>
void doSomething(Args... args) {
    int x[] = {(args.do(), 0)...};
}
 
// To do more complex things, you can put them in another function:
template <class Arg>
void process(Arg arg, int &someOtherData) {
    // You can do something with arg here.
}
 
template <class... Args>
void doSomething(Args... args) {
    int someOtherData;
    int x[] = {(process(args, someOtherData), 0)...};
}
```
Source: http://stackoverflow.com/questions/7230621/how-can-i-iterate-over-a-packed-variadic-template-argument-list

# Memory Barriers

Nice description here: http://lwn.net/Articles/576486/

Note the memory model of the Linux Kernel is different when compared to the memory model of C++11/C11, which is more relaxed, see:

    http://lwn.net/Articles/691128/ - "The C11 model is based on acquire/release semantics — one-way barriers that are described in the 2014 article and this article. Much of the kernel, instead, makes use of load/store barriers, which are stricter, two-way barriers."
    http://infolab.stanford.edu/pub/cstr/reports/csl/tr/95/685/CSL-TR-95-685.pdf
    http://www.cl.cam.ac.uk/~pes20/weakmemory/index.html

# Atomic Shared Pointer
Atomic smart pointers are described in https://isocpp.org/files/papers/N4162.pdf (this is a small pdf and in appendix 5 - pg 9 - is a nice, simple, example) and are about making assignment of the pointer atomic, not just the reference counting, so that the following code can be written. Anthony Williams has an implementation here https://bitbucket.org/anthonyw/atomic_shared_ptr.

Having this ability has have made addressing the contention issues on MP much easier (as in we can just change the type in the std:vector<> we use, and eliminate many lines of code). This strategy also motivated the recent performance change on PSEMS, where we also could benefit from this approach in the future. A second example is here also.
Atomic smart pointers
```C++
// g++ -std=cs++14 -lpthread ./main.cpp -I./
#include <thread>
#include <array>
#include <atomic>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
namespace
{
  constexpr size_t buf_sz = 1024*1024;
  using buffer_t = std::shared_ptr<uint8_t>;
  using buffer_list_t = std::array< buffer_t, 25 >;
}
 
void producer( std::atomic<bool>& run, const unsigned id, buffer_list_t& buffers )
{
  while( run )
  {
    buffer_t buf((uint8_t*)malloc(buf_sz), free); // Allocate a new buffer, say to hold a frame.
    memset( buf.get(), 0xa, buf_sz );  // Write the data to it.
    std::atomic_store(&buffers[id], buf); // Update the entry, say for the specific encoder.
    std::this_thread::sleep_for( std::chrono::milliseconds{10} );
  }
}
 
void consumer( std::atomic<bool>& run, const unsigned id, buffer_list_t& buffers )
{
  while( run )
  {
    // Here if buf is the last reference to this entry, because the producer updates
    // the entry, then it will be cleaned up by the shared pointer destructor.
    auto buf = std::atomic_load(&buffers[id]); // Get a pointer to the buffer.   
     
    if( buf )
      memset( buf.get(), 0xb, buf_sz );  // Modify some data in it for fun.
    else
      std::cout << "no buffers yet for " << id << "\n";
 
    std::this_thread::sleep_for( std::chrono::milliseconds{10} );
  }
}
 
int main()
{
  std::vector<std::thread> threads;
  std::atomic<bool> run{true};
  buffer_list_t buffers;
  size_t i = 0;
 
  for( auto& b : buffers )
  {
    threads.push_back( std::thread( producer, std::ref(run), i, std::ref(buffers) ));
    threads.push_back( std::thread( producer, std::ref(run), i, std::ref(buffers) ));
 
    threads.push_back( std::thread( consumer, std::ref(run), i, std::ref(buffers) ));
    threads.push_back( std::thread( consumer, std::ref(run), i, std::ref(buffers) ));
    ++i;
  }
 
  std::this_thread::sleep_for( std::chrono::seconds(5) );
  run = false;
  for( auto& t : threads )
    t.join();
 
  return EXIT_SUCCESS;
}
```
