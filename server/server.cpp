/**
 * Author: Brian Gianforcaro ( bjg1955@cs.rit.edu )
 *
 * Description: A server appiication that takes remote commands
 * to add, retrive records from a "database". 
 *
 * Usage: tcp-project2 port
 * 				
 * Where 'port' is the port number the server is listening on.
 */

#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;

#include <map>
  using std::map;

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h> // for bzero(..)

// Networking and sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// POSIX compliant threading
#include <pthread.h>

// Project specific header
#include "common.h"

/**
 * Data structure to use when passing different data
 * members to a new connection handler thread.
 */
typedef struct 
{
	int sock;
	int threadnum;
  sockaddr_in* address;
  socklen_t addressLen;
} sock_t;

//
// Global variables used to track program state across threads.
//

int threadCount; // Global variable tracking thread count

map<int,record_t> database; // Our "database" of records

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

  record_t response;
  bzero( &response, sizeof( response ) );

	bool exists = database.find( rec.id ) != database.end() ;

  if ( exists )
  {
    response.command = ADD_FAILURE;
    cout << "Record ID " << rec.id << " exists." << endl;
  }
  else
  {
    // Insert the new record
    database[rec.id] =  rec;
    response.command = ADD_SUCCESS;
    response.id = rec.id;
    cout << "Adding record" << endl;
    cout << "ID: " << rec.id << endl;
    cout << "Name: " << rec.name << endl;
    cout << "Age: " << rec.age << endl;
		cout << "Size of the database: " << database.size() << endl;
  }

  write( sock, &response, sizeof( response ) );
  return not exists;
}

/**
 * Try to fetch an existing record from the database.
 *
 * @param[in] rec - The record to look for, using id field.
 * @param[in] sock - The socket to use when sending result.
 *
 * @return True on success, false on failure.
 */
bool getRecord( record_t rec, int sock )
{

	record_t result;
  bzero( &result, sizeof( result ) );

	bool found = database.find( rec.id ) != database.end();

  if ( found )
  {
    result = database[rec.id];
    result.command = RET_SUCCESS;
    cout << "Record ID " << rec.id << " found." << endl;
  }
  else
  {
    result.command = RET_FAILURE;
		result.id = rec.id;
    cout << "Record ID " << rec.id << " not found." << endl;
  }

  write( sock, &result, sizeof( result ) );
  return found;
}

/**
 * Threading function to respond to a incoming client request.
 *
 * @param[in] arg - The socket file descriptor, casted to a void* 
 * in order to work with the threading library.
 *
 * @return EXIT_SUCCESS
 */
void* handleClient( void* arg )
{

  sock_t* incoming = (sock_t*)arg; 

  cout << "Entering Thread # " << (int)incoming->threadnum
       << " Client IP: " << inet_ntoa( incoming->address->sin_addr ) 
			 << ", Port: " <<  ntohs( incoming->address->sin_port ) << ":" << endl; 
	cout << "======================================================" << endl;

  int len; 
  record_t request;
  while ( ( len = read( incoming->sock, &request, sizeof( request ) ) ) > 0 )
  {

		cout << "Received " << len << " bytes from the socket " << endl;
		cout << "Command: " << request.command << endl;
    //
    // Perform the actual action requested
    //
    switch ( request.command )
    {
      case add_t:
        addRecord( request, incoming->sock );
        break;
      case retrive_t:
        getRecord( request, incoming->sock );
        break;
      default:
        break;
    }

    bzero( &request, sizeof( request ) );
  }

  //
  // Shutdown reads and writes to the socket
  //
  shutdown( incoming->sock, SHUT_RDWR );

  //
  // Close the socket and exit this thread 
  //
  close( incoming->sock );

 
  cout << "Exiting Thread # " << incoming->threadnum
       << " Client IP: " << inet_ntoa( incoming->address->sin_addr ) 
			 << ", Port: " <<  ntohs( incoming->address->sin_port ) << ":"
			 << " ... client closed the socket" << endl;
	cout << "======================================================" << endl;

  threadCount--;

	delete incoming->address;
	delete incoming;

	cout << "Total # of threads running at this time is " << threadCount << endl;

  pthread_exit( static_cast<void*>( EXIT_SUCCESS ) );

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
  // Create a TCP socket.
  //
  int sock = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sock < 0 )
  {
    cerr << "Socket: " << strerror( errno ) << endl;
    exit( EXIT_FAILURE );
  }

	//
  // Fill in the struct with the port number given by the user.
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
    cerr << "Bind: " << strerror( errno ) << endl;
    exit( EXIT_FAILURE );
  }

  // Make this socket passive, that is, one that awaits connections
  // rather than initiating them.
  if ( listen( sock, 10 ) < 0 )
  {
    cerr << "Listen: " << strerror( errno ) << endl;
    exit( EXIT_FAILURE );
  }

	cout << "MAIN THREAD - WAITING FOR THE FIRST CONNECTION FROM CLIENT ..." << endl;

  threadCount = 0;

  //
  // Now start serving clients
  //
  while ( true )
  {

		sock_t* incoming = new sock_t;
		incoming->address = new struct sockaddr_in;
		incoming->addressLen = sizeof( struct sockaddr_in ); 

		// Wait for any new connections.
    incoming->sock = accept( sock,
				                     (struct sockaddr*)incoming->address,
							 							 &(incoming->addressLen) ); 

    if ( incoming->sock < 0 )
    {
      cerr << "Server: accept error" << endl; 
      exit( EXIT_FAILURE );
    }

    incoming->threadnum = ++threadCount;

    pthread_t childThread;

		cout << "NEW THREAD CREATED: NO. " << threadCount  << endl;

    // Create a thread to handle this connection.
    pthread_create( &childThread, NULL, handleClient, (void*)incoming ); 

		cout << "MAIN THREAD - WAITING FOR THE NEXT CONNECTION FROM CLIENT ..." << endl;
  }
  return EXIT_SUCCESS;
}
