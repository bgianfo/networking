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
	struct sockaddr_in address;
} sock_t;


int globalSeq = 0;

/**
 * 
 */
bool sendgram( sock_t s, Datagram* gram )
{

	int tryCount;
	bool received;
  int waitTime = 10;


	int len = 0;

	if ( gram->type == ACK || gram->type == SYN || gram->type == FIN )
	{
		len = sizeof( gram->type ) + sizeof( gram->seq );
	}
	else if ( gram->type == DATA )
  {
		len = sizeof( Datagram );
	}
	gram->seq = globalSeq;

  cerr << "Sending Packet" << endl;
	cerr << "Type: " << gram->type << endl;
	sendto( s.sock, gram, len,
			    0, (struct sockaddr*) &(s.address),
					sizeof( struct sockaddr ) );

	tryCount = 0;
	received = false;
	while ( tryCount < 6 )
	{

		bool timerExpired = false;
		start_timer( waitTime, timerExpired );

		while ( true )
		{
			Datagram response;

    tryAgain:
			cerr << "Trying to receive" << endl;
			if ( recv( s.sock, &response, sizeof( response ), 0 ) < 0 )
			{
				if ( response.type == ACK )
				{
					stop_timer();
    			cerr << "Recived ACK" << endl;
					return true;
				}

    	  cerr << "Recived non - ACK" << endl;
				cerr << "Type: " << response.type << endl;
			}
			else
 		  {	
				if ( errno == EINTR )
				{
					goto tryAgain;
				}
				perror("recv: ");
			}

			if ( timerExpired )
			{
				cerr << "Timer expired, try # " << tryCount << endl;
				tryCount++;
				break;
			}
		}

		stop_timer();
		cerr << "Resending Packet" << endl;
 	  sendto( s.sock, gram, sizeof( Datagram ),
			    0, (struct sockaddr*) &(s.address),
					sizeof( struct sockaddr ) );


	}

	return false;
}

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

	Datagram gram;
  bzero( &gram, sizeof( gram ) );

	gram.type = SYN;
	sendgram( s, &gram );

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
void addRecord( sock_t sock )
{
  record_t newRecord;
  newRecord.command = add_t;

  cout << "Enter id (interger):";
  newRecord.id = obtainInt( "ID should be a non-zero integer):" );

  cout << "Enter name (up to 32 char):";
  scanf( "%32s", newRecord.name );

  cout << "Enter age (integer):";
  newRecord.age = obtainInt( "Age should be a non-zero integer):" );

	Datagram gram;
	bzero( &gram, sizeof( Datagram ) );
	gram.type = DATA;
	gram.seq = globalSeq;
  memcpy( gram.data, &newRecord, sizeof( record_t ) );
  
	cerr << "Sending Data" << endl;
	sendgram( sock, &gram );

	Datagram resultDgram;
  bzero( &resultDgram, sizeof( resultDgram ) );

  record_t resultRec;
  bzero( &resultRec, sizeof( resultRec ) );

  // Get the result back from the server
getAddData:

	cerr << "Trying to receive Data" << endl;
	if ( recv( sock.sock, &resultDgram, sizeof( resultDgram ), 0 ) < 0 )
	{

		if ( resultDgram.type == DATA )
		{
		  memcpy( &resultRec, resultDgram.data, sizeof( record_t ) );
  
		  Datagram gram;
		  bzero( &gram, sizeof( Datagram ) );
		  gram.type = ACK;
		  gram.seq = globalSeq;
	    sendto( sock.sock, &gram, sizeof( gram.type ) + sizeof( gram.seq ),
	    			  0, (struct sockaddr*) &(sock.address),
		  			  sizeof( struct sockaddr ) );
		}
		else
		{
		  cerr << "Got wrong type" << endl;
		  goto getAddData;

		}
	}
	else
	{
		cerr << "Error trying to receive" << endl;
		goto getAddData;
	}
		
	

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
void retriveRecord( sock_t sock )
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
	gram.seq = 0;
  memcpy( gram.data, &findRecord, sizeof( record_t ) );
  

	sendgram( sock, &gram );


  // Now do the actual writing of the data out to the socket.
  // write( sock, (char*) &findRecord, sizeof(findRecord.command) + sizeof(findRecord.id) );

  record_t resultRec;
  bzero( &resultRec, sizeof( resultRec ) );

  // Get the result back from the server
getRecord:

	cerr << "Trying to receive" << endl;
	if ( recv( sock.sock, &resultRec, sizeof( resultRec ), 0 ) < 0 )
	{
		Datagram gram;
		bzero( &gram, sizeof( Datagram ) );
		gram.type = ACK;
	  sendto( sock.sock, &gram, sizeof( Datagram ),
	  			  0, (struct sockaddr*) &(sock.address),
					  sizeof( struct sockaddr ) );



	}
	else
	{
		cerr << "Error trying to receive" << endl;
		goto getRecord;
	}
			
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
				sendgram( sock, &gram );
        shutdown( sock.sock, SHUT_RDWR );
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
