#include "router.hh"
#include "debug.hh"

#include <iostream>
#include <bitset>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num)
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/"
       << static_cast<int>(prefix_length) << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)")
       << " on interface " << interface_num << "\n";
  // 首先查询IP是否在路由表中，如果在，更新数据
  // 否则，插入数据
  // 目的网段可能会配置子网网址，所以需要与上子网掩码的前prefix_length位进行匹配
  uint32_t mask = (prefix_length == 0) ? 0 : 0xffffffff << (32 - prefix_length);
  
  if (route_table_.find(route_prefix & mask) != route_table_.end())
  {
    // 说明目前存在目标网段
    auto range = route_table_.equal_range(route_prefix & mask);
    auto it = range.first;
    for (; it != range.second; ++it)
    {
      auto &[pre_len, hop, inter_index] = it->second;
      // 判断子网掩码是否相同，如果相同，更新数据
      if (pre_len == prefix_length)
      {
        // 更新hop和接口下标
        hop = next_hop;
        inter_index = interface_num;
        break;
      }
    }
    // 如果it == range.second 则说明不存在子网掩码相同的情况，插入数据
    if (it == range.second)
    {
      route_table_.emplace(route_prefix & mask, std::make_tuple(prefix_length, next_hop, interface_num));
    }
  }
  else
  {
    route_table_.emplace(route_prefix & mask, std::make_tuple(prefix_length, next_hop, interface_num));
  }
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug("route() called");
  // 有一个难点是，如何实现IP地址的匹配，最长匹配原则
    for (auto interface_it = interfaces_.begin(); interface_it != interfaces_.end(); ++interface_it)
    {
      // debug("route() called, line : {}, intreface : {}", __LINE__, interface_it->get()->name());
      // 进行1次遍历从接口收包
      // 从接口队列收包 按顺序处理
      auto &datagrames_queue = interface_it->get()->datagrams_received();
      while (!datagrames_queue.empty())
      {
        // 放在收包循环内部，避免污染下一个数据包的转发
        std::optional<Address> next_hop{};
        size_t interface_index{};
        uint8_t prefix_length{};
        auto packet = datagrames_queue.front();
        // 调试记录1，提前丢弃数据报，避免无效处理
        if(packet.header.ttl <= 1)
        {
          debug("TTL value is less than or equal to 1");
          // 丢弃该数据报
          datagrames_queue.pop();
          continue;
        }
        // 在路由表中查找最长匹配的路由 路由表中主键为网段地址 但是数据包中为IP地址
        // 匹配
        for (auto route_table_it = route_table_.begin(); route_table_it != route_table_.end(); ++route_table_it)
        {
          // 如果数据包的目的地址 与上子网掩码 等于网段地址 则说明匹配
          // 不应该与上子网掩码 应该与上前子网掩码个数位都为1的32位源码进行匹配
          uint32_t mask = (std::get<0>(route_table_it->second) == 0) ? 0 : 0xffffffff << (32 - std::get<0>(route_table_it->second));
          // debug("route called, line : {}, make : {}", __LINE__, mask);
          if ((packet.header.dst & mask) == route_table_it->first)
          {
            // 当前子网掩码是否大于prefix_length 如果不大于 则不进行匹配
            // 如果next_hop没有value 则数据报的next_hop则为数据报的目的地址
            // 如果有value 则数据报的next_hop则为表项的next_hop
            // Error: Host default_router did NOT receive an expected Internet datagram: IPv4 len=32 proto=144 ttl=63 src=10.0.0.2 dst=1.2.3.4
            // 调试记录2 子网掩码初始值为0，默认路由子网掩码为0，因此将>修改为>=
            if (std::get<0>(route_table_it->second) >= prefix_length)
            {
              if (std::get<1>(route_table_it->second).has_value())
              {
                next_hop = std::get<1>(route_table_it->second);
              }
              else
              {
                // 如果没有地址 则说明是直连 下一跳地址就是目的地址
                next_hop.emplace(Address::from_ipv4_numeric(packet.header.dst));
              }
              // 子网掩码长度
              prefix_length = std::get<0>(route_table_it->second);
              // 接口下标
              interface_index = std::get<2>(route_table_it->second);
            }
          }
        }
        // 从接口发包
        // 说明有对应的转发表项
        // 判断获取了下一跳地址
        if (next_hop.has_value())
        {
          // TTL值-1
          packet.header.ttl--;
          // 重新计算数据报校验和
          packet.header.compute_checksum();
          // 获取接口并发包
          interface(interface_index)->send_datagram(packet, next_hop.value());
        }
        // 统一从队列中删除已发送的数据包 
        datagrames_queue.pop();
      }
    }
}
