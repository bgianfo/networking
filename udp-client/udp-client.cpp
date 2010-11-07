/**
 * Author: Brian Gianforcaro ( bjg1955@cs.rit.edu )
 *
 * Description: A client appiication that can add, retrive
 * records from a remote database server. 
 *
 * Usage: udp-project3 hostname port
 * 				
 * Where 'hostname' is the name of the remote host on which
 * the server is running and 'port' is the port number it is using.
 */

// Stream stdout/stderr IO
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <string>	
	using std::string;

// Utilities, IO and Error checking
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // for bzero(..)

// Networking and sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>

// Project specific header
#include "Timer.h"
#include "common.h"
#include "datagram.h"

typedef struct
{ 
  int sock; 
  unsigned int seq;
  struct sockaddr_in address;
  socklen_t addrlen; 
} sock_t;

#define TIME_OUT 3

/**
 * Setup the connection to the server and return the sockets file descriptor.
 *
 * @param[in] hostname - The hostname to use when connecting.
 * @param[in] port - The port number to use when connecting.
 *
 * @return A file descriptor to the setup and connected socket. 
 */
sock_t setupSocket( char* hostname, int port )
{
  sock_t s;
  bzero( &s, sizeof( sock_t ) );

  // Initialize a socket to work with
  s.sock = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( s.sock < 0 )
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
  s.address.sin_family = AF_INET;
  s.address.sin_port = htons( port );
  s.address.sin_addr.s_addr = *(u_long *)hostent->h_addr;
  s.addrlen = sizeof( struct sockaddr );
	s.seq = 0;

  Datagram gram;
  bzero( &gram, sizeof( gram ) );
	gram.type = SYN;

	bool timedOut;
	int timeOuts = 0;

	do {
		sendto( s.sock, (char *)&gram, 8, 0, (struct sockaddr *)&s.address, s.addrlen );

		timedOut = false;

		start_timer( TIME_OUT, timedOut );
		recvfrom( s.sock, (char *)&gram, sizeof( gram ), 0, (struct sockaddr *)&s.address, &s.addrlen );
		stop_timer();

		if ( timedOut )
		{
			cout << "Connection time out #" << ++timeOuts << endl;
			
			if ( timeOuts > 5 ) 
			{
				cout << "Connection failed. Exiting..." << endl;
				close( s.sock );
				exit(EXIT_FAILURE);
			}
		}
	} while( timedOut );

	return s;
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


int obtainInt( string msg )
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
void addRecord( sock_t& sock )
{
  Datagram gram;
  record_t newRecord;
  newRecord.command = add_t;

  cout << "Enter id (interger):";
  newRecord.id = obtainInt( "ID should be a non-zero integer):" );

  cout << "Enter name (up to 32 char):";
  scanf( "%32s", newRecord.name );

  cout << "Enter age (integer):";
  newRecord.age = obtainInt( "Age should be a non-zero integer):" );

  bzero( &gram, sizeof( Datagram ) );
  gram.type = DATA;
  gram.seq = sock.seq;
  memcpy( gram.data, &newRecord, sizeof( record_t ) );
 	
  record_t resultRec;

	bool timedOut;
	int timeOuts = 0;
  struct sockaddr_in address;
	struct sockaddr* addr = (struct sockaddr*) &address;
	socklen_t len = sizeof( address );

	do
	{
		// Send add request
		sendto( sock.sock, (char *)&gram, sizeof( Datagram ), 0,
					  (struct sockaddr *)&sock.address, sock.addrlen );
		timedOut = false;
		start_timer( TIME_OUT, timedOut );
		unsigned int seqCheck;
	  bool finished = false;
		do
		{
			// Receive ACK
			recvfrom( sock.sock, (char *)&gram, sizeof(Datagram),0,
			        	addr, &len );

			seqCheck = gram.seq;
			if ( seqCheck != sock.seq )
			{
				cout << "Incorrect Seq number recieved." << endl;
			}

			if ( sock.address.sin_addr.s_addr != address.sin_addr.s_addr
					 || sock.address.sin_port != address.sin_port )
			{
				cout << "Packet arrived fom an unexpected source, discarding." << endl;
				finished = false;
			}
			else
			{
				finished = true;
			}

		}
		while ( seqCheck != sock.seq || not finished );
		stop_timer();

		if ( timedOut )
		{
			cout << "Add request time out #" << ++timeOuts << endl;
	
			if ( timeOuts > 5 )
			{
				cout << "Add request timed out..." << endl;
			}
		} 
	}
	while( timedOut && timeOuts < 5 );

	if ( not timedOut )
	{
	  tryAddAck:
		// Receive success/failure
		recvfrom( sock.sock, (char *)&gram, sizeof(gram), 0,
				      addr, &len );

		if ( sock.address.sin_addr.s_addr != address.sin_addr.s_addr
				 || sock.address.sin_port != address.sin_port )
		{
			cout << "Packet arrived fom an unexpected source, discarding." << endl;
			goto tryAddAck;
		}



		memcpy( &resultRec, gram.data, sizeof( record_t ) );

		if ( resultRec.command == ADD_SUCCESS )
		{
			cout << "ID " << newRecord.id << " added successfully" << endl;
		}
		else
		{
      cout << "ID " << newRecord.id << " already exists" << endl;
		}
 
		// Send ACK
		gram.type = ACK;
		gram.seq = sock.seq;
		sendto( sock.sock, (char *)&gram, 8, 0,
				    (struct sockaddr *)&sock.address,
						sock.addrlen );

		// Update Sequence Number
		++(sock.seq);
		cout << "Add: Incremented sequence#" << sock.seq << endl;
	}
}

/**
 * Attempt to retrieve a record from the remote database.
 *
 * @param[in] sock - The socket's file descriptor
 */
void retriveRecord( sock_t& sock )
{
  record_t findRecord;
  bzero( &findRecord, sizeof( findRecord ) );

  findRecord.command = retrive_t;
  findRecord.id = 0;

  cout << "Enter id (interger):";
  findRecord.id = obtainInt( "ID should be a non-zero integer):" );

  Datagram gram;
  bzero( &gram, sizeof( Datagram ) );
  gram.type = DATA;
  gram.seq = sock.seq;
  memcpy( gram.data, &findRecord, sizeof( findRecord ) );

  record_t resultRec;
  bzero( &resultRec, sizeof( resultRec ) );

	bool timedOut;
	int timeOuts = 0;
  struct sockaddr_in address;
	struct sockaddr* addr = (struct sockaddr*) &address;
	socklen_t len = sizeof( address );

	do
	{
		// Send add request
		sendto( sock.sock, &gram, sizeof( Datagram ), 0,
					  (struct sockaddr *)&sock.address, sock.addrlen );
		timedOut = false;
		start_timer( TIME_OUT, timedOut );
		unsigned int seqCheck;

		bool finished = false;
		do
		{
			// Receive ACK
			recvfrom( sock.sock, (char *)&gram, sizeof(gram),0,
			        	addr, &len );

			seqCheck = gram.seq;
			if ( seqCheck != sock.seq )
			{
				cout << "Incorrect Seq number received." << endl;
			}

			if ( sock.address.sin_addr.s_addr != address.sin_addr.s_addr
					 || sock.address.sin_port != address.sin_port )
			{
				cout << "Packet arrived fom an unexpected source, discarding." << endl;
				finished = false;
			}
			else
			{
				finished = true;
			}

		}
		while( seqCheck != sock.seq || not finished );
		stop_timer();

		if ( timedOut )
		{
			cout << "Retrieve request time out #" << ++timeOuts << endl;
	
			if ( timeOuts > 5 )
			{
				cout << "Retrieve request timed out..." << endl;
			}
		} 
	}
	while( timedOut && timeOuts < 5 );

	if ( not timedOut )
	{
    retryGet:
		// Receive success/failure
		recvfrom( sock.sock, (char *)&gram, sizeof(gram), 0,
			       	addr, &len );

		if ( sock.address.sin_addr.s_addr != address.sin_addr.s_addr
				 || sock.address.sin_port != address.sin_port )
		{
			cout << "Packet arrived fom an unexpected source, discarding." << endl;
			goto retryGet;
		}

		memcpy( &resultRec, gram.data, sizeof( record_t ) );

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
 
		// Send ACK
		gram.type = ACK;
		gram.seq = sock.seq;
		sendto( sock.sock, (char *)&gram, 8, 0,
				    (struct sockaddr *)&sock.address,
						sock.addrlen );

		// Update Sequence Number
		++(sock.seq);
		cout << "Ret: Incremented sequence# " << sock.seq << endl;
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
    
    sock_t sock = setupSocket( hostname, port );

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
        Datagram gram;
        bzero( &gram, sizeof( Datagram ) );
        gram.type = FIN;
				gram.seq = sock.seq;
			  sendto( sock.sock, (char *)&gram, 8, 0, (struct sockaddr *)&sock.address, sock.addrlen );
        close( sock.sock );
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
