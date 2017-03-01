#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <array>
#include <string>
#include <iostream>

using namespace boost::asio;
using namespace boost::asio::ip;

int main()
{
  boost::asio::io_service aios;

  boost::asio::ip::tcp::resolver resolver(aios);
  boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(
      boost::asio::ip::tcp::resolver::query("localhost", "2222"));

  boost::asio::ip::tcp::socket socket(aios);
  boost::asio::connect(socket, endpoint);

  for(;;)
  {
    std::array<char, 256> buf{};
    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(std::string("hi")));
    size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if(error == boost::asio::error::eof)
    {
      std::cout << "server closed socket\n";
      break;
    }
    else if(error)
    {
      std::cout << error.message() << "\n";
    }

    std::cout.write(buf.data(), len);
    std::cout << '|';
  }
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
