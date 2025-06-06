#pragma once

#include "exception.hh"
#include "network_interface.hh"

#include <optional>
// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface(std::shared_ptr<NetworkInterface> interface)
  {
    interfaces_.push_back(notnull("add_interface", std::move(interface)));
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface(const size_t N) { return interfaces_.at(N); }

  // Add a route (a forwarding rule)
  void add_route(uint32_t route_prefix,
                 uint8_t prefix_length,
                 std::optional<Address> next_hop,
                 size_t interface_num);

  // Route packets between the interfaces
  void route();
  // 路由表数据的增删改查 需要设计路由表的存储接口 实现不高于o(n)时间复杂度的数据结构
  // vector时间复杂度为o(n)，map unordered map时间复杂度为ologn
  // 目的网段-子网掩码-下一跳-网络接口 此为路由表的存储结构
  // 只存储目的网段够吗？其实是不够的，因为还需要考虑子网掩码的因素，比如192.168.0.0/24 和192.168.0.0/16可能同时存在，但是可能下一跳不同
  // 仍旧使用multimap进行存储，主键为目标网段，存储子网掩码，下一跳，网络接口，下一跳地址用于判断从哪个网络接口转发出去
  // 使用二叉树进行存储的话，需要考虑如何进行比较和查询
private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> interfaces_{};
  // 主键为目标网段和子网掩码的组合，值为下一跳地址和网络接口的下标，
  // std::map<stNetworkKey, std::pair<std::optional<Address>, size_t>> route_table_{};
  // 因为我不知道IP数据报目的地址的子网掩码，所以使用目标网段和子网掩码进行存储的方式是无效的，还是要使用IP地址进行存储
  // 主键为目标网段，值为子网掩码、下一跳、网络接口的下标
  std::multimap<uint32_t, std::tuple<uint8_t, std::optional<Address>, size_t>> route_table_{};
};
#if 0
// 结构体用于特化hash
using stNetworkKey = struct NetworkKey
{
  uint32_t networkAddr;
  uint8_t subnetMask;

  // 重载==运算符
  bool operator==(const NetworkKey &other) const
  {
    return networkAddr == other.networkAddr && subnetMask == other.subnetMask;
  }
  bool operator<(const NetworkKey &other) const
  {
    return networkAddr < other.networkAddr || (networkAddr == other.networkAddr && subnetMask < other.subnetMask);
  }
};

template <>
struct std::hash<stNetworkKey>
{
  size_t operator()(const stNetworkKey &key) const
  {
    return hash<uint32_t>()(key.networkAddr) ^ hash<uint8_t>()(key.subnetMask);
  }
};
#endif