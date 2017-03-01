// http://stackoverflow.com/questions/670891/is-there-a-way-for-multiple-processes-to-share-a-listening-socket
// g++ -std=c++14 server.cpp -o server -l boost_system  -lpthread
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include <string>

int main()
{
  using boost::asio::ip::tcp;
  boost::asio::io_service io_service; 
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 2222));

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
