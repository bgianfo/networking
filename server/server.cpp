// $com CC $@ -o $* -lsocket -lnsl -library=iostream
//
// File:	tcp-server.C
// Author:	K. Reek
// Modified by:	Narayan J. Kulkarni
// Description:	Multithreaded Server, full duplex TCP connection
// Usage:	tcp-server port
//		where 'port' is the well-known port number to use.
//
// This program uses TCP to establish connections with clients and process
// requests.
//

#include <stdlib.h>
#include <unistd.h>

#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;

#include <vector>
  using std::vector;

#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread.h>

#include "common.h"

//
// Global variables used to track program state across threads.
//

int threadCount; // Global variable tracking thread count

vector<record_t> database; // Our "database" of records

/**
 * Try to add a given record to the database.
 *
 * @param[in] rec - The new record to add.
 * @param[in] sock - The client socket to use when responding.
 *
 * @return True on success, false on failure.
 */
bool addRecord( record_t rec, int sock )
{

  bool exists = false;
  for ( size_t i = 0; i < database.size(); ++i )
  {
    if ( database[i].id == rec.id )
    {
      exists = true;
      break;
    }
  }

  record_t response;
  bzero( &response, sizeof( response ) );
  if ( exists )
  {
    response.command = ADD_FAILURE;
  }
  else
  {
    // Insert the new record
    database.push_back( rec );
    response.command = ADD_SUCCESS;
  }

  write( sock, (char*) &response, sizeof( response ) );
  return not exits;
}

/**
 * Try to fetch an existing record from the database.
 *
 * @param[in] rec - The record to look for, using id field.
 * @param[in] sock - The socket to use when sending result.
 *
 * @return True on sucess, fales on failure.
 */
bool getRecord( record_t rec, int sock )
{

  bool found = false;
  record_t result;
  for ( size_t i = 0; i < database.size(); ++i )
  {
    if ( database[i].id == rec.id )
    {
      found = true;
      memcpy( (char*) &result, (char*) &(database[i]), sizeof( database ) );
      break;
    }
  }

  if ( found )
  {
    result.command = ADD_SUCCESS;
  }
  else
  {
    bzero( &result, sizeof( result ) );
    result.command = RET_FAILURE;
  }

  write( sock, (char*) &result, sizeof( response ) );
  return exists;
}

/**
 * Threading function to respond to a incoming client request.
 *
 * @param[in] socket - The socket file descriptor, casted to a void* 
 * in order to work with the threading library.
 *
 * @return EXIT_SUCCESS
 */
void* handleClient( void* socket )
{

  // Do the proper, portable conversion from (void*) to int
  long lsocket = reinterpret_cast<long>( socket );
  int threadSocket = static_cast<int>( lsocket );

  //
  // Unpack the request
  //
  record_t request;
  read( threadSocket,
        reinterpret_cast<char*>( &request ),
        sizeof( request ) );

  //
  // Perform the actual action requested
  //
  switch ( request.command )
  {
    case add_t:
      addRecord( request, threadSocket );
      break;
    case retrive_t:
      getRecord( request, threadSocket );
      break;

    default:
      break;
  }

  //
  // Close the socket and exit this thread 
  //
  close( threadSocket );
  threadCount--;
  thr_exit( static_cast<void*>( EXIT_SUCCESS ) );

  return EXIT_SUCCESS;
}

/**
 * Program entry point.
 *
 * @param[in] argc - The number of command line arguments.
 * @param[in] argv - The actual command line arguments.
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int main( int argc, char **argv )
{

  thread_t chld_thr;
  
  threadCount = 0;

  //
  // Make sure that the port argument was given
  //
  if ( argc != 2 )
  {
          cerr << "Usage: " << argv[0] << " port" << endl;
          exit( EXIT_FAILURE );
  }

  //
  // Get the port number, and make sure that it is legitimate
  //
  int port = atoi( argv[ 1 ] );
  if ( port < PORT_MIN || port > PORT_MAX )
  {

          cerr << port << ": invalid port number" << endl;
          exit( EXIT_FAILURE );
  }

  //
  // Create a stream (TCP) socket.  The protocol argument is left 0 to
  // allow the system to choose an appropriate protocol.
  //
  int sock = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sock < 0 )
  {
          cerr << "socket: " << strerror( errno ) << endl;
          exit( EXIT_FAILURE );
  }

  //
  // Fill in the addr structure with the port number given by
  // the user.  The OS kernel will fill in the host portion of
  // the address depending on which network interface is used.
  //
  struct sockaddr_in server;
  bzero( &server, sizeof( server ) );
  server.sin_family = AF_INET;
  server.sin_port = htons( port );

  //
  // Bind this port number to this socket so that other programs can
  // connect to this socket by specifying the port number.
  //
  if ( bind( sock, (struct sockaddr *)&server, sizeof( server ) ) < 0 )
  {
          cerr << "bind: " << strerror( errno ) << endl;
          exit( EXIT_FAILURE );
  }

  /* set the level of thread concurrency we desire */
  thr_setconcurrency(2);

  // Make this socket passive, that is, one that awaits connections
  // rather than initiating them.
  if ( listen( sock, 5 ) < 0 )
  {
          cerr << "listen: " << strerror( errno ) << endl;
          exit( EXIT_FAILURE );
  }

  //
  // Now start serving clients
  //
  for(;;)
  {

          cout << "WAITING FOR A CONNECTION ..." << endl;
          struct sockaddr_in cli_addr; 
          socklen_t clilen = sizeof(cli_addr);
          int newsockfd = accept( sock, (struct sockaddr *) &cli_addr, &clilen ); 

          if ( newsockfd < 0 )
          {
                  fprintf( stderr,"server: accept error\n" );
                  exit( EXIT_FAILURE );
          }

          // Create a thread to handle this connection.
          thr_create( NULL, 0, handleClient, (void*) newsockfd, THR_DETACHED, &chld_thr ); 

          /* The server is now free to accept another socket request */
          cout << "NEW SERVER THREAD CREATED." << endl;
          threadCount++;
  }
  return EXIT_SUCCESS;
}
