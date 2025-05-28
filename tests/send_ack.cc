#include "random.hh"
#include "sender_test_harness.hh"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

using namespace std;

int main()
{
  try {
    auto rd = get_random_engine();

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Repeat ACK is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( Push { "a" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "a" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Old ACK is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( Push { "a" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "a" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 2 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( Push { "b" } );
      test.execute( ExpectMessage {}.with_no_flags().with_data( "b" ) );
      test.execute( ExpectNoSegment {} );
      test.execute( AckReceived { Wrap32 { isn + 1 } } );
      test.execute( ExpectNoSegment {} );
      test.execute( HasError { false } );
    }

    // credit for test: Jared Wasserman (2020)
    // 不能简单判断ack的seqno和已发送未确认数据包的seqno，还需要判断ack的seqno是否超过了当前isn_，如果超过了当前isn_，则为无效ack
    {
      TCPConfig cfg;
      const Wrap32 isn( rd() );
      cfg.isn = isn;

      TCPSenderTestHarness test { "Impossible ackno (beyond next seqno) is ignored", cfg };
      test.execute( Push {} );
      test.execute( ExpectMessage {}.with_no_flags().with_syn( true ).with_payload_size( 0 ).with_seqno( isn ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( AckReceived { Wrap32 { isn + 2 } }.with_win( 1000 ) );
      test.execute( ExpectSeqnosInFlight { 1 } );
      test.execute( HasError { false } );
    }
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return 1;
  }

  return EXIT_SUCCESS;
}
