/**
 * Author: Brian Gianforcaro ( bjg1955@cs.rit.edu )
 *
 * Description: A client appiication that can add, retrive
 * records from a remote database server. 
 *
 * Usage: tcp-project1 hostname port
 * 				
 * Where 'hostname' is the name of the remote host on which
 * the server is running and 'port' is the port number it is using.
 */

// Stream stdout/stderr IO
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

// Utilities, IO and Error checking
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h> // for bzero(..)
#include <string.h>

// Networking and sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "common.h"

/**
 * Setup the connection to the server and return the sockets file descriptor.
 *
 * @param[in] hostname - The hostname to use when connecting.
 * @param[in] port - The port number to use when connecting.
 *
 * @return A file descriptor to the setup and connected socket. 
 */
int setupSocket( char* hostname, int port )
{
  // Initialize a socket to work with
  int sock = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sock < 0 )
  {
    cerr << "socket: " << strerror( errno ) << endl;
    exit( EXIT_FAILURE );
  }

  // Resolve the host-name to an address
  struct hostent *hostent = gethostbyname( hostname );
  if ( NULL == hostent )
  {
    cerr << "gethostbyname: error code " << h_errno << endl;
    exit( EXIT_FAILURE );
  }

  // Setup socket address struct
  struct sockaddr_in address;
  bzero( &address, sizeof( address ) );
  address.sin_family = AF_INET;
  address.sin_port = htons( port );
  address.sin_addr.s_addr = *(u_long *)hostent->h_addr;

  // Connect the socket to the previously setup address
  if ( connect( sock, (struct sockaddr*)&address, sizeof( address ) ) < 0 )
  {
    cerr << "connect: " << strerror( errno ) << endl;
    exit( EXIT_FAILURE );
  }

  return sock;
}

/**
 * Print the usage statement for the client application.
 *
 * @param[in] binary - The name of the binary being executed.
 */
void usage( char* binary )
{
  cerr << "Usage: " << binary << " hostname port " << endl;
}


int obtainInt( char* msg )
{
  int value = 0;
  bool valid = false;
  while ( not valid )
  {
    int ret = scanf( "%d", &value );
    if ( ret == EOF || value == 0 )
    {
      cerr << msg;
    } 
    else
    {
      valid = true;
    }
  }
  return value; 
}

/**
 * Attempt to add a new record to the remote database.
 *
 * @param[in] sock - The socket's file descriptor
 */
void addRecord( int sock )
{
  record_t newRecord;
  newRecord.command = add_t;

  cout << "Enter id (interger):";
  newRecord.id = obtainInt( "ID should be a non-zero integer):" );

  cout << "Enter name (up to 32 char):";
  scanf( "%32s", newRecord.name );

  cout << "Enter age (integer):";
  newRecord.age = obtainInt( "Age should be a non-zero integer):" );

  // Now do the actual writing of the data out to the socket.
  write( sock, (char*) &newRecord, sizeof(newRecord) );

  record_t resultRec;
  bzero( &resultRec, sizeof( resultRec ) );

  read( sock, (char*) &resultRec, sizeof(resultRec.command) );

  if ( resultRec.command == ADD_SUCCESS )
  {
    cout << "ID " << newRecord.id << " added successfully" << endl;
  }
  else
  {
    cout << "ID " << newRecord.id << " already exists" << endl;
  }
}


/**
 * Attempt to retrive a record from the remote database.
 *
 * @param[in] sock - The socket's file descriptor
 */
void retriveRecord( int sock )
{
  record_t findRecord;
  bzero( &findRecord, sizeof( findRecord ) );

  findRecord.command = retrive_t;
  findRecord.id = 0;

  cout << "Enter id (interger):";
  findRecord.id = obtainInt( "ID should be a non-zero integer):" );

  // Now do the actual writing of the data out to the socket.
  write( sock, (char*) &findRecord, sizeof(findRecord.command) + sizeof(findRecord.id) );

  record_t resultRec;
  bzero( &resultRec, sizeof( resultRec ) );

  // Get the result back from the server
  read( sock, (char*) &resultRec, sizeof(resultRec) );

  // Display our results
  if ( resultRec.command == RET_SUCCESS )
  {
    cout << "ID: " << resultRec.id << endl;
    cout << "Name: " << resultRec.name << endl;
    cout << "Age: " << resultRec.age << endl;
  }
  else
  {
    cout << "ID " << findRecord.id << " does not exist" << endl;
  }
}


/**
 * Client main function
 *
 * @param[in] argc - The number of arguments passed to the program.
 * @param[in] argv - The arguments passed to the program.
 */
int main( int argc, char** argv )
{

  if ( argc != 3 )
  {	
    usage( argv[0] );
    return EXIT_FAILURE;
  }
  else
  {
    char* hostname = argv[HOSTNAME];
    int port = atoi( argv[PORTNUM] );

    // Validate the given port number
    if ( port < PORT_MIN or port > PORT_MAX )
    {
      cerr << port << ": invalid port number" << endl;
      exit( EXIT_FAILURE );
    }
    
    int sock = setupSocket( hostname, port );

    while ( true )	
    {

      cout << "Enter command (" << add_t << " for Add, " << retrive_t 
           << " for Retrive, " << quit_t << " to quit):"; 

      int cmd = 100; 
      scanf( "%d", &cmd );
      
      if ( cmd == add_t )
      {
        addRecord( sock );
      }
      else if ( cmd == retrive_t )
      {
        retriveRecord( sock );
      }
      else if ( cmd == quit_t )
      {
        shutdown( sock, SHUT_RDWR );
        close( sock );
        break;
      }
			else
			{
				cout << "Illegal command " << cmd << endl;
			}
    }
  }
  return EXIT_SUCCESS;
}
