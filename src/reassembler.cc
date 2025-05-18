#include "reassembler.hh"
#include "debug.hh"

#include <algorithm>
#include <ranges>

using namespace std;

auto Reassembler::split( uint64_t pos ) noexcept
{
  // 找到第一个大于等于pos的元素 即 以pos为下边界查找元素
  auto it = buf_.lower_bound( pos );
  if ( it != buf_.end() and it->first == pos ) {
    return it;
  }
  if ( it == buf_.begin() ) {
    return it;
  }
  // 数据段的尾部需要和it有重叠 分段处理
  if ( const auto pit = prev( it ); pit->first + pit->second.size() > pos ) {
    const auto res = buf_.emplace_hint( it, pos, pit->second.substr( pos - pit->first ) );
    pit->second.resize( pos - pit->first );
    return res;
  }
  // 没有重叠 直接返回
  return it;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  const auto try_colse = [&]() noexcept -> void {
    debug( "try_colse() first_index:{}, is_last_substring", first_index, is_last_substring );
    if ( end_index_.has_value() and end_index_.value() == writer().bytes_pushed() ) {
      output_.writer().close();
      return;
    }
  };
  // 空数据包
  if ( data.empty() ) {
    if ( !end_index_.has_value() and is_last_substring ) {
      end_index_.emplace( first_index );
    }
    return try_colse();
  }
  // 如果流关闭 或者没有可用空间 直接返回
  // 哪里体现Reassembler的内存限制
  if ( writer().is_closed() or writer().available_capacity() == 0 ) {
    return;
  }

  // 滑动窗口边界 [已确认字节数，能够接收字节数)
  // 已确认字节数 作为下一个期望字节的索引
  const uint64_t write_index = writer().bytes_pushed();
  // 能够接收的字节数 作为无法接收字节的索引
  const uint64_t can_receive_index = write_index + writer().available_capacity();
  // 数据索引范围 [first_index, first_index + data.size())
  // 如果与窗口没有交集 直接返回
  if ( first_index + data.size() <= write_index || first_index >= can_receive_index ) {
    return;
  }
  // 考虑数据裁剪，如果数据与窗口有交集 但是超出窗口范围 需要裁剪数据
  // 裁剪后数据不完整 清除结束标记
  if ( first_index + data.size() > can_receive_index ) {
    data.resize( can_receive_index - first_index );
    is_last_substring = false;
  }
  // 如果数据包首字节小于窗口首字节 则[first_index,write_index)的数据已经被接收 裁剪即可
  if ( first_index < write_index ) {
    data.erase( 0, write_index - first_index );
    first_index = write_index;
  }

  // 如果没有值 并且是最后一个数据包
  if ( !end_index_.has_value() and is_last_substring ) {
    end_index_.emplace( first_index + data.size() );
  }
  //  数据重组填充
  // 删除重复字符
  // 返回值为 >= pos的第一个元素 如果buf_中所有元素都大于或者小于 不会删除 只有有重叠的部分才会删除
  const auto upper = split( first_index + data.size() );
  const auto lower = split( first_index );
  // 更新total_pending_bytes_ 减去重复字符串
  ranges::for_each( ranges::subrange( lower, upper ) | views::values,
                    [&]( const auto& str ) { total_pending_bytes_ -= str.size(); } );
  // 将数据添加到总pending字节数
  total_pending_bytes_ += data.size();

  // 插入数据
  buf_.emplace_hint( buf_.erase( lower, upper ), first_index, move( data ) );

  // 数据推送逻辑
  while ( !buf_.empty() ) {
    // 获取首索引和字符串
    auto&& [index, str] = *buf_.begin();
    // 首索引不等于窗口首索引 直接返回
    if ( index != writer().bytes_pushed() )
      break;
    // 写入首数据 弹出首数据 总缓存字节数减去首数据大小
    output_.writer().push( str );
    total_pending_bytes_ -= str.size();
    buf_.erase( buf_.begin() );
  }

  // 尝试关闭writer 条件为存在结束标记 并且全部写入到writer中
  try_colse();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  debug( "count_bytes_pending() called total_pending_bytes_ : {}", total_pending_bytes_ );
  return total_pending_bytes_;
}
