#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

auto NetworkInterface::make_arp(const uint16_t opcode, const EthernetAddress &dst_mac, const uint32_t dst_ip) const noexcept -> ARPMessage
{
  ARPMessage stARPMess;
  stARPMess.opcode = opcode;
  stARPMess.sender_ethernet_address = ethernet_address_;
  stARPMess.sender_ip_address = ip_address_.ipv4_numeric();
  stARPMess.target_ethernet_address = dst_mac;
  stARPMess.target_ip_address = dst_ip;
  return stARPMess;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(string_view name,
                                   shared_ptr<OutputPort> port,
                                   const EthernetAddress &ethernet_address,
                                   const Address &ip_address)
    : name_(name), port_(notnull("OutputPort", move(port))), ethernet_address_(ethernet_address), ip_address_(ip_address)
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string(ethernet_address_) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop)
{
  debug("send_datagram called");
  EthernetFrame stEthernetFrame{};

  // 源mac
  stEthernetFrame.header.src = this->ethernet_address_;

  // 首先在ARP映射中查询IP地址对应的MAC地址 IP-ARP映射使用unordered_map进行存储
  uint32_t next_hop_numric = next_hop.ipv4_numeric();
  if (ip_arp_map_.find(next_hop_numric) != ip_arp_map_.end())
  {
    // 如果查到，将IP数据包打包为以太网帧并发送
    // 目的mac 从ARP映射中查询
    stEthernetFrame.header.dst = ip_arp_map_[next_hop_numric].first;
    // 类型 IPV4
    stEthernetFrame.header.type = EthernetHeader::TYPE_IPv4;
    // 数据
    stEthernetFrame.payload = serialize(dgram);
    // 发送mac帧
    transmit(stEthernetFrame);
  }
  else
  {
    // 如果没查到，发送ARP广播请求，收到ARP响应后再打包为以太网帧发送
    // 发送ARP广播请求 构造ARP广播请求报文
    // 5s内不要发送重复ARP请求
    if (arp_send_time.find(next_hop_numric) != arp_send_time.end() and arp_send_time[next_hop_numric] + 5000 > timestamp_)
      ;
    else
    {
      ARPMessage stARPMessage = make_arp(ARPMessage::OPCODE_REQUEST, {}, next_hop_numric);
      // 发送arp请求
      // ARP广播
      stEthernetFrame.header.dst = ETHERNET_BROADCAST;
      // 帧类型为ARP
      stEthernetFrame.header.type = EthernetHeader::TYPE_ARP;
      // 将ARP帧打包为以太网帧并发送
      stEthernetFrame.payload = serialize(stARPMessage);
      // 记录ARP请求发送的时间戳
      arp_send_time[next_hop_numric] = timestamp_;
      transmit(stEthernetFrame);
    }
    // IP数据报进入队列排队
    multimap_datagrams_send_.emplace(next_hop.ipv4_numeric(), std::make_pair(dgram, timestamp_));
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame(EthernetFrame frame)
{
  // 打印帧头string
  debug("recv_frame called, line : {}, frame header: {}", __LINE__, frame.header.to_string());
  // 目的地址非广播地址 并且 目的地址非本机MAC
  if (frame.header.dst != ETHERNET_BROADCAST and ethernet_address_ != frame.header.dst)
  {
    debug("recv_frame called, line : {}, ethernet packet dst address is not valid", __LINE__);
    return;
  }
  // 判断帧类型
  if (frame.header.type == EthernetHeader::TYPE_IPv4)
  {
    // 解析payload为IPV4报文
    InternetDatagram stIPv4{};
    // 解析payload为IPV4报文
    if (not parse(stIPv4, frame.payload))
    {
      return;
    }
// 以下几种情况IP报文不能接收 ttl为0 目的地址不是本机IP checksum不正确
    datagrams_received_.emplace(std::move(stIPv4));
  }
  else if (frame.header.type == EthernetHeader::TYPE_ARP)
  {
    // 解析payload为ARP报文
    ARPMessage stARPMess;
    //! \todo 解析payload为ARP报文
    if (not parse(stARPMess, frame.payload))
    {
      return;
    }
// 以下几种情况不能接收此ARP报文
// 目的地址非广播并且非本机mac地址
// 分为ARP请求和ARP响应 ARP请求是目的IP为本机 ARP响应是目的IP和目的mac都为本机
// 目的IP非本机 or ARP响应时目的mac非本机、则return
    // 其实还需要检查 ARPMessage Req时目的MAC是不是广播MAC地址
    if (ip_address_.ipv4_numeric() != stARPMess.target_ip_address or (ARPMessage::OPCODE_REPLY == stARPMess.opcode and ethernet_address_ != stARPMess.target_ethernet_address))
    {
      return;
    }
    // 提取IP地址和MAC地址的映射关系 并记录时间戳
    // 首先看是否有此ARP映射 如果有ARP映射 需要删除
    if (ip_arp_map_.find(stARPMess.sender_ip_address) != ip_arp_map_.end())
    {
      // 有ARP映射
      // 删除旧时间戳
      time_ip_map_.erase(ip_arp_map_[stARPMess.sender_ip_address].second);
    }
    // 更新表项
    ip_arp_map_[stARPMess.sender_ip_address] = std::make_pair(stARPMess.sender_ethernet_address, timestamp_);
    // 更新时间戳
    time_ip_map_[ip_arp_map_[stARPMess.sender_ip_address].second] = stARPMess.sender_ip_address;
    // 如果是ARP请求报文 发送ARP回复报文
    if (stARPMess.opcode == ARPMessage::OPCODE_REQUEST)
    {
      // 构造ARP回复报文并发送
      ARPMessage stARPReply = make_arp(ARPMessage::OPCODE_REPLY, stARPMess.sender_ethernet_address, stARPMess.sender_ip_address);
      // 构造二层mac帧
      EthernetFrame stEthReply{};
      stEthReply.header.type = EthernetHeader::TYPE_ARP;
      stEthReply.header.dst = stARPReply.target_ethernet_address;
      stEthReply.header.src = stARPReply.sender_ethernet_address;
      stEthReply.payload = serialize(stARPReply);
      transmit(stEthReply);
    }
    // 遍历队列 发送arp报文对应的IP地址的报文
    if (multimap_datagrams_send_.find(stARPMess.sender_ip_address) != multimap_datagrams_send_.end())
    {
      // 那就说明有IP数据包需要发送
      // 从multimap中取出所有键值对
      auto range = multimap_datagrams_send_.equal_range(stARPMess.sender_ip_address);
      for (auto it = range.first; it != range.second;)
      {
        InternetDatagram stIPv4 = std::move(it->second.first);
        // 打包成二层发送
        EthernetFrame stEthReply{};
        stEthReply.header.type = EthernetHeader::TYPE_IPv4;
        stEthReply.header.dst = ip_arp_map_[stARPMess.sender_ip_address].first;
        stEthReply.header.src = ethernet_address_;
        stEthReply.payload = serialize(stIPv4);
        transmit(stEthReply);
        // 删除映射
        it = multimap_datagrams_send_.erase(it);
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick)
{
  debug("tick({}) called", ms_since_last_tick);
  timestamp_ += ms_since_last_tick;
  // 遍历time_ip_map_ 找到过期的映射关系 删除
  for (auto it = time_ip_map_.begin(); it != time_ip_map_.end();)
  {
    // ARP表项超时时间为30S
    if (it->first + ARP_TIMEOUT < timestamp_)
    {
      // 删除IP-ARP映射
      ip_arp_map_.erase(it->second);
      // 删除时间戳-IP映射关系
      it = time_ip_map_.erase(it);
    }
    else
    {
      break;
    }
  }
  // 缓存ARP请求 过期时间为5S
  for(auto it = multimap_datagrams_send_.begin(); it!= multimap_datagrams_send_.end();++it)
  {
    if(it->second.second + ARP_REQ_TIMEOUT < timestamp_)
    {
      // 删除ARP过期请求对应的IP数据包
      multimap_datagrams_send_.erase(it);
    }
    
  }
}
