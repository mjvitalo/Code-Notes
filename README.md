# Code-Notes

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
