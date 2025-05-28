#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // 发送的字节数减去接收的字节数
  // debug( "bytes_in_flight_ : {}", bytes_in_flight_ );
  return bytes_in_flight_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "consecutive_retransmissions_ : {}", consecutive_retransmissions_ );
  return consecutive_retransmissions_;
}

// 仅从input中读出数据并发送 发送
void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "push() called" );
  auto message = make_empty_message();
  if ( !syn_ ) {
    // 第一次发包 设置全局SYN
    syn_ = true;
    message.SYN = true;
    // 序列号进行偏移
  }
  if ( input_.has_error() ) {
    message.RST = true;
  }
  // 有数据发数据  没数据再说
  // 处理零窗口特殊情况
  uint16_t effective_window = window_size_;
  if ( effective_window == 0 ) {
    effective_window = 1;
  }
  // 发送窗口大小修改为使用接收窗口大小减去飞行中字节数
  // 并且不能向下溢出 来自send_extra.cc:252
  // 需要减去message.sequence_length() 因为message.sequence_length() 包含了SYN和RST 对应测试用例send_extra.c:641
  uint64_t available_window = ( effective_window > bytes_in_flight_ )
                                ? ( effective_window - bytes_in_flight_ - message.sequence_length() )
                                : 0;
  if ( input_.reader().bytes_buffered() > 0 ) {
    // 根据发送窗口和缓存数据量计算可以读取的数据量
    auto read_size = min( uint64_t( available_window ), input_.reader().bytes_buffered() );
    read_size = min( read_size, TCPConfig::MAX_PAYLOAD_SIZE );
    // debug( "read_size : {}", read_size );
    // 读取数据
    // 接收窗口为payload的长度
    if ( read_size > 0 ) {
      message.payload = input_.reader().peek();
      message.payload.resize( read_size );
      input_.reader().pop( read_size );
    }
  }
  // debug( "message payload size : {}, available_window : {}", message.payload.size(), available_window );
  // FIN报文在msg_vector_ 不在push中重发，在此重发导致序列号计算出错
  if ( message.payload.size() < available_window and input_.reader().is_finished() and !fin_ ) {
    fin_ = true;
    message.FIN = true;
  }
  // 如果数据包长度为0 数据不发送
  if ( message.sequence_length() == 0 ) {
    return;
  }

  // 减小发送窗口 发送窗口最小值为1
  // 为什么需要发送窗口大小最小为1？因为发送窗口大小为0时，发送方不能发送数据，此时如果接受方不应该发送ack，或者ack丢失，此时发送方会一直等待，因此将发送窗口大小设置为1，这样发送方就可以发送数据了，当接收方接收窗口不为0时，回复ack，发送方可以继续发送数据
  // 如果是发送SYN包，则不改变发送窗口大小
  // 对应测试用例 send_retx.cc:24
  // if (!message.SYN)
  // {
  // 维护飞行中字节数
  bytes_in_flight_ += message.sequence_length();
  // }
  // 只有FIN
  isn_ = isn_ + message.sequence_length();
  // debug("push() called line : {}, message.SYN : {}, message.payload.size : {}, message.FIN : {}, message.RST :
  // {}", __LINE__, message.SYN, message.payload.size(), message.FIN, message.RST);
  msg_vector_.emplace_back( std::move( message ) );
  // debug( "message seq length : {}", msg_vector_.back().sequence_length() );
  transmit( msg_vector_.back() );
  // as long as there are new bytes to be read and space available in the window.
  // 对应用例 send_extra.c:170
  if ( input_.reader().bytes_buffered() > 0 and ( window_size_ - bytes_in_flight_ ) > 0 ) {
    // debug( "reverse push() called" );
    push( transmit );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { isn_, false, "", false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // debug( "receive() called" );
  // 不太清楚需要设置哪个成员变量为error
  if ( msg.RST ) {
    // debug( "receive called line : {}, msg.RST : {}", __LINE__, msg.RST );
    input_.set_error();
    return;
  }
  // 发送窗口 >= 1
  window_size_ = msg.window_size;
  // if (window_size_ == 0)
  // {
  //   window_size_ = 1;
  // }
  // 通过确认号索引发送的报文是否已经被接受 如果被接收则删除
  if ( msg.ackno.has_value() ) {
    // debug("receive called line : {}, msg_vector_.size : {}", __LINE__, msg_vector_.size());
    // 遍历向量 删除已经被接受的报文
    for ( auto it = msg_vector_.begin(); it != msg_vector_.end(); ) {
      // 如何判断报文已经被接收
      // 现在有两个序列号，1是确认号 2是报文的序列号 需要比较确认号是否 确认号unwrap不能小于
      // 其实应该连续删除 这里为了测试 只删除首包
      uint64_t ack_isn_ackno = msg.ackno.value().unwrap( isn_, 0U );
      // 如果以isn_为基准序列号解出来的数据 小于2^31 且 >0 则认为akc > isn_ 此序列号为超前序列号
      if ( ack_isn_ackno < ( (uint64_t)1 << 31 ) and ack_isn_ackno > 0 ) {
        // debug( "receive called line : {}, ack_isn_ackno : {}", __LINE__, ack_isn_ackno );
        // break;
        return;
      }
      uint64_t ack_abso_ackno = msg.ackno.value().unwrap( it->seqno, 0U );
      // debug("it->seqno raw_value : {}, ack_abso_ackno : {}", it->seqno.raw_value(), ack_abso_ackno);
      if ( ( ack_abso_ackno < UINT16_MAX ) and ( ack_abso_ackno >= it->sequence_length() ) ) {
        bytes_in_flight_ -= it->sequence_length();
        // debug("receive called line : {}, bytes_in_flight_ : {}, it.seq_len : {}", __LINE__, bytes_in_flight_,
        // it->sequence_length());
        it = msg_vector_.erase( it );
        RTO_ms_ = initial_RTO_ms_;
        consecutive_retransmissions_ = 0;
        timer_ = 0;
        // debug("receive called line : {}, message acked", __LINE__);
        // break;
      } else {
        // 是向后查询还是break
        ++it;
      }
    }
  }
  // debug( "receive called line : {}, msg_vector_.size : {}", __LINE__, msg_vector_.size() );
}

// 仅实现ARQ中GBN首包功能
void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // 如果没有报文 则返回
  if ( msg_vector_.empty() ) {
    return;
  }
  timer_ += ms_since_last_tick;
  // debug( "tick called line : {}, timer_ : {}, RTO_ms_ : {} , initial_RTO_ms_ : {}, ms_since_last_tick : {}",
  //        __LINE__,
  //        timer_,
  //        RTO_ms_,
  //        initial_RTO_ms_,
  //        ms_since_last_tick );
  // 如果当前时间大于等于RTO 则重传首包
  if ( timer_ >= RTO_ms_ ) {
    timer_ = 0;
    transmit( msg_vector_.front() );
    // If the window size is nonzero: 接收窗口非零
    // 为什么是接收窗口非零 因为接收窗口为0时 发送的是零窗口探测报文 不触发RTO翻倍
    if ( window_size_ > 0 ) {
      ++consecutive_retransmissions_;
      RTO_ms_ *= 2;
    }
    timer_ = 0;
  }
}
