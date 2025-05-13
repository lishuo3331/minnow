#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // 如果socket关闭或者没有空间或者插入数据为空 直接返回
  if (is_closed() or available_capacity() == 0 or data.empty())
  {
    return;
  }
  // 如果data的长度大于剩余长度 截取
  if (data.length() > available_capacity())
  {
    data.resize(available_capacity());
  }
  total_buffered_ += data.length();
  total_pushed_ += data.length();

  buffer_.emplace(move(data));
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - total_buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_; // Your code here.
}

string_view Reader::peek() const
{
  return buffer_.empty() ? string_view{} : string_view{buffer_.front()}.substr(removed_prefix_);
}

void Reader::pop( uint64_t len )
{
  total_popped_ += len;
  total_buffered_ -= len;

  while (len > 0 and !buffer_.empty())
  {
    const uint64_t &size{buffer_.front().size() - removed_prefix_};
    // 如果下一个string的长度大于len 则跳过
    if (len < size)
    {
      removed_prefix_ += len;
      break;
    }
    // 如果下一个string的长度小于len 则pop
    buffer_.pop();
    // index 清零
    removed_prefix_ = 0;
    // len减去pop的长度
    len -= size;
  }
}

bool Reader::is_finished() const
{
  return closed_ and total_buffered_ == 0;
}

uint64_t Reader::bytes_buffered() const
{
  return total_buffered_;
}

uint64_t Reader::bytes_popped() const
{
  return total_popped_;
}

