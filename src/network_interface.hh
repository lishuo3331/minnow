#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "arp_message.hh"
#include "ipv4_datagram.hh"

#include <memory>
#include <queue>
#include <unordered_map>
#include <map>

// A "network interface" that connects IP (the internet layer, or network layer)
// with Ethernet (the network access layer, or link layer).

// This module is the lowest layer of a TCP/IP stack
// (connecting IP with the lower-layer network protocol,
// e.g. Ethernet). But the same module is also used repeatedly
// as part of a router: a router generally has many network
// interfaces, and the router's job is to route Internet datagrams
// between the different interfaces.

// The network interface translates datagrams (coming from the
// "customer," e.g. a TCP/IP stack or router) into Ethernet
// frames. To fill in the Ethernet destination address, it looks up
// the Ethernet address of the next IP hop of each datagram, making
// requests with the [Address Resolution Protocol](\ref rfc::rfc826).
// In the opposite direction, the network interface accepts Ethernet
// frames, checks if they are intended for it, and if so, processes
// the the payload depending on its type. If it's an IPv4 datagram,
// the network interface passes it up the stack. If it's an ARP
// request or reply, the network interface processes the frame
// and learns or replies as necessary.
class NetworkInterface
{
public:
  // An abstraction for the physical output port where the NetworkInterface sends Ethernet frames
  class OutputPort
  {
  public:
    virtual void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) = 0;
    virtual ~OutputPort() = default;
  };

  // Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
  // addresses
  NetworkInterface( std::string_view name,
                    std::shared_ptr<OutputPort> port,
                    const EthernetAddress& ethernet_address,
                    const Address& ip_address );

  // Sends an Internet datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next
  // hop. Sending is accomplished by calling `transmit()` (a member variable) on the frame.
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, pushes the datagram to the datagrams_in queue.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  void recv_frame( EthernetFrame frame );

  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );

  // Accessors
  const std::string& name() const { return name_; }
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
  // Human-readable name of the interface
  std::string name_;

  // The physical output port (+ a helper function `transmit` that uses it to send an Ethernet frame)
  std::shared_ptr<OutputPort> port_;
  void transmit( const EthernetFrame& frame ) const { port_->transmit( *this, frame ); }
  // 需要传入optype 目的mac 目的IP
  auto make_arp(const uint16_t opcode, const EthernetAddress& dst_mac, const uint32_t dst_i) const noexcept -> ARPMessage;

  

  // Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
  EthernetAddress ethernet_address_;

  // IP (known as internet-layer or network-layer) address of the interface
  Address ip_address_;

  // Datagrams that have been received
  std::queue<InternetDatagram> datagrams_received_ {};

  std::queue<uint32_t> queue_nexthop_datagrams_ {};

  std::queue<InternetDatagram> queue_datagrams_send_{};

  std::multimap<uint32_t, std::pair<InternetDatagram, size_t>> multimap_datagrams_send_ {};

  std::unordered_map<uint32_t, size_t> arp_send_time {};
  // 存储IP-ARP映射 用于记载有没有 还需要记录时间 如果在map中记录，每次更新的时间是o(n)，如果使用vector记录，则更新的时间复杂度为o(n)
  // 因此这样存储，unordered_map存储IP:MAC,时间映射
  std::unordered_map<uint32_t, std::pair<EthernetAddress, size_t>> ip_arp_map_ {};
  // map 存储时间，IP地址
  // 增加：o(lg(n))
  // 删除：o(1)
  // 更新：o(1)
  // 查询：o(1)
  // 插入需要排序
  std::map<size_t, uint32_t> time_ip_map_ {};
  // 时间戳 以0为初始值
  size_t timestamp_ {0};
  // ARP表项超时时间
  #define ARP_TIMEOUT 30000
  // ARP请求超时时间
  #define ARP_REQ_TIMEOUT 5000
};
