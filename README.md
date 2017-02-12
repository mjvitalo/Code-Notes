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

Code Notes

    Edit
    Favourite
    Watching
    Share

    Pages
    Home 

Skip to end of banner

Go to start of banner
Skip to end of metadata

    Created by Mike Vitalo, last modified on Feb 09, 2017

Go to start of metadata

    C++
    RCU
    Lock Free / Wait Free Data Structures
    Hardware Transactional Memory
    Handle Signals
    Edit Distance & Levenshtein Distance
    Container Sorted Insert
    Access Once
    Boost
    Using ASIO for I/O, Timers, Networking, and Other Work
        Downloading
        CSS Update Daemon on the Icon
        ASIO as a work queue: 
    Subexpressions of an initialization list expression are evaluated in order.
    Memory Barriers
    GNU Libc Heap Allocation Behavior
    Atomic Shared Pointer
    Thread Safe Interface Idiom
    Concurrent<T>
    Active Object
    Sleep a thread
    Threads
    Threads with Time functions
    Replace Array
    Memory Allocation (new, detele, malloc)
    Replace loops with std algorithms
    Parametrized Construction and Destruction in Smart Pointers
    Count Cores & CPUs
    Processor usage
    Pipe Example
    Valid IP Address
    Print Tuple
    C++ Memento approach with tuples
    Trie, Radix Tree, Patricia Trie
        GCC Extensions
        Basic GCC header
        Using Standard Library
    Trimming Strings
    Printing Stack Traces
    File rotator example
    JSON example
    JASON POST Example (read JSON from HTTP, and print it as XML)
    List Child Processes
    UTF 8 Handling
        Using Standard Library

C++

Improvements from using C++14: https://kfrlib.com/blog/how-c14-and-c17-help-to-write-faster-and-better-code-real-world-examples/

cppcon 2015

Writing good C++ - clean and modern style.

Use STL algorithms. Use STL algorithms to simplify code and clarify intent.

Vectors, cache locality, and predictable memory access for performance gains: http://www.stroustrup.com/Software-for-infrastructure.pdf
RCU

Dissertation: http://www.rdrop.com/users/paulmck/RCU/RCUdissertation.2004.07.14e1.pdf

Read-copy update (RCU) is a synchronization mechanism that was added to the Linux kernel in October of 2002. RCU achieves scalability improvements by allowing reads to occur concurrently with updates. In contrast with conventional locking primitives that ensure mutual exclusion among concurrent threads regardless of whether they be readers or updaters, or with reader-writer locks that allow concurrent reads but not in the presence of updates, RCU supports concurrency between a single updater and multiple readers. RCU ensures that reads are coherent by maintaining multiple versions of objects and ensuring that they are not freed up until all pre-existing read-side critical sections complete. RCU defines and uses efficient and scalable mechanisms for publishing and reading new versions of an object, and also for deferring the collection of old versions. These mechanisms distribute the work among read and update paths in such a way as to make read paths extremely fast. In some cases (non-preemptable kernels), RCU's read-side primitives have zero overhead (LWN).

Also: http://kukuruku.co/hub/cpp/lock-free-data-structures-the-inside-rcu
Lock Free / Wait Free Data Structures

CDS is a C++ template library of lock-free and fine-grained algorithms. It contains a collection of concurrent data structure implementations

http://libcds.sourceforge.net/

A nice write up goes this this concurrent queue implementation:

https://github.com/cameron314/concurrentqueue

Boost Lockfree

http://www.boost.org/doc/libs/1_62_0/doc/html/lockfree.html

Herb Sutter's proposal for atomic shared pointers has a  correct  and  ABA safe  implementation  of  a thread safe  singly  linked  list  that supports insert/erase at the front only (like a stack) but also supports finding values in the list. It is written entirely in portable C++ 11, except only that it uses the paper’s proposed atomic<shared_ptr<Node>>, which is discussed below.

http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4058.pdf
Hardware Transactional Memory

C++ Transactional Memory

The idea behind hardware transactional memory is to allow two cores to execute code speculatively if they touch global state, and both should always succeed unless they touch the same bit of global state. For example, if two cores are updating entries in a tree, both will read from the root to a leaf and then each will modify one leaf. In a traditional locking model, you might have a single lock for the entire tree, so that accesses are serialized. With a transactional approach, each starts a transaction, walks to the desired node, and then updates it. The transactions would always succeed, unless both cores tried to update the same node.

In this case, the hardware keeps track of which cache lines have been read from and which have been written to. If another core reads the same cache line addresses as you do, there's no conflict. If either writes to a cache line that another has read or written to, then there's a conflict, and one of the transactions will fail. (Which one fails is defined by implementation.)
Easy to use synchronization construct

    As easy to use as coarse-grain locks

    Programmer declares, system implements

Often performs as well as fine-grain locks

    Automatic read-read concurrency & fine-grain concurrency

Failure atomicity & recovery

    No lost locks when a thread fails

    Failure recovery = transaction abort + restart

Composability

    Safe & scalable composition of software modules

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

References

    http://www.cs.cmu.edu/afs/cs/academic/class/15418-s12/www/lectures/20_transactionalmem.pdf
    http://www.quepublishing.com/articles/article.aspx?p=2142912

Handle Signals
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

or with boost you can do the following
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
