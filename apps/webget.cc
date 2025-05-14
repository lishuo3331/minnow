#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // 协议为http协议
  Address addr( host, "http" );
  // 创建TCP套接字
  TCPSocket tcp_socket;
  // 连接到指定地址
  tcp_socket.connect( addr );
  // 构建HTTP请求并发送
  // string_view request("GET " + path + "HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  tcp_socket.write( string_view( "GET " + path + " HTTP/1.1\r\n" ) );
  tcp_socket.write( string_view( "Host: " + host + "\r\n" ) );
  tcp_socket.write( string_view( "Connection: close\r\n\r\n" ) );

  // 读取HTTP响应
  while ( !tcp_socket.eof() ) {
    string buffer;
    tcp_socket.read( buffer );
    std::cout << buffer;
  }
  // 关闭TCP套接字
  tcp_socket.close();
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
