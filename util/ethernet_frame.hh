#pragma once

#include "ethernet_header.hh"
#include "parser.hh"

#include <vector>

struct EthernetFrame
{
  EthernetHeader header {};
  std::vector<Ref<std::string>> payload {};


  // 网络字节流解码为结构化以太网帧
  void parse( Parser& parser )
  {
    header.parse( parser );
    parser.all_remaining( payload );
  }

  
  // 以太网帧编码为网络字节流
  void serialize( Serializer& serializer ) const
  {
    header.serialize( serializer );
    serializer.buffer( payload );
  }
};
