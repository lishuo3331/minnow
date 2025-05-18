#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // Set the Initial Sequence Number if necessary
  if ( writer().has_error() ) {
    return;
  }
  if ( message.RST ) {
    reader().set_error();
    return;
  }
  if ( !zero_point_.has_value() ) {
    if ( !message.SYN ) {
      return;
    }
    zero_point_.emplace( message.seqno );
  }
  // 计算绝对序列号 如果是SYN 则序列号从0开始，如果非SYN 需要-1
  reassembler_.insert( message.seqno.unwrap( zero_point_.value(), writer().bytes_pushed() )
                         + ( message.SYN ? 0 : -1 ),
                       message.payload,
                       message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // const uint16_t window_size = min(static_cast<uint32_t>(writer().available_capacity()),
  // static_cast<uint32_t>(UINT16_MAX));
  const uint16_t window_size = writer().available_capacity() > UINT16_MAX
                                 ? static_cast<uint16_t>( UINT16_MAX )
                                 : static_cast<uint16_t>( writer().available_capacity() );
  // 说明收到了SYN
  if ( zero_point_.has_value() ) {
    return { Wrap32::wrap( writer().bytes_pushed() + 1 + writer().is_closed(), zero_point_.value() ),
             window_size,
             writer().has_error() };
  }
  return { nullopt, window_size, writer().has_error() };
}
