#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// Include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <vector>
#include <fstream>
#include <cstdlib>

struct detail
{
  std::string stuff_;
  unsigned more_stuff_;
};

struct super
{
  unsigned call_id_ = 0;
  unsigned conference_id_ = 0;

  std::string name_;
  std::vector<detail> details_;
};

template<typename Archive>
void serialize(Archive& archive, detail& d, const unsigned int version)
{
  archive & d.stuff_;
  archive & d.more_stuff_;
}

template<typename Archive>
void serialize(Archive& archive, super& s, const unsigned int version)
{
  archive & s.call_id_;
  archive & s.conference_id_ ;
  archive & s.name_;
  archive & s.details_;
}

std::ostream& operator<< (std::ostream &o, const super &a)
{
  o << "call_id_: " << a.call_id_ << "\tconference_id_: " << a.conference_id_ << "\tname: " << a.name_ << "details:\n";
  for(const auto& d : a.details_) o << "detail.stuff: " <<  d.stuff_ << "\tdetail.more_stuff_: " << d.more_stuff_ << "\n";
  return o;
}

int main()
{
  const super s{3,33, "super", {{"stuff", 2}, {"other stuff", 3}}};
  super rs;

  {
    std::fstream archive_file("archive.data");
    boost::archive::text_oarchive out_archive(archive_file);
    out_archive << s;
  }
  {
    std::fstream archive_file("archive.data");
    boost::archive::text_iarchive in_archive(archive_file);
    in_archive >> rs;
  }

  std::cout << rs << std::endl;
  return EXIT_SUCCESS;
}
