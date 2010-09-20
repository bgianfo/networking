/**
 * Author: Brian Gianforcaro ( bjg1955@cs.rit.edu )
 *
 * Description: 
 *
 * Usage: tcp-project1 hostname port
 * 				
 * 				Where 'hostname' is the name of the remote host on which
 * 				the server is running and 'port' is the port number it is using.
 */

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <string>
	using std::string;

#include <stdlib.h>
#include <unistd.h>
#include <strings.h> // for bzero(..)
#include <string.h>


#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define	PORT_MIN	1024
#define	PORT_MAX	65535

#define HOSTNAME 1
#define PORTNUM 2

#define ADD_SUCCESS 0
#define ADD_FAILURE 1

#define RET_SUCCESS 0
#define RET_FAILURE 1


#define MAX_LEN 32

typedef struct { 
  int command; 
  int id; 
	char name[MAX_LEN]; 
  int   age;
} record_t;


enum {
	add_t,
	retrive_t,
	quit_t
} actions_t;


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
	if ( hostent == NULL )
	{
		cerr << "gethostbyname: error code " << h_errno << endl;
		exit( EXIT_FAILURE );
	}

	struct sockaddr_in address;
	bzero( &address, sizeof( address ) );

	address.sin_family = AF_INET;
	address.sin_port = htons( port );
	address.sin_addr.s_addr = *(u_long *)hostent->h_addr;

	// Connect the socket to the previously setup address
	if ( connect( sock, (struct sockaddr *)&address, sizeof( address ) ) < 0 )
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
	cerr << "Usage: " << binary << "hostname port " << endl;
}

void addRecord( int sock )
{
	record_t newRecord;
	cout << "Enter id (interger):";

	newRecord.command = add_t;

	bool valid = false;
	while( not valid )
	{
		if ( scanf( "%d", &newRecord.id ) == EOF )
		{
			cout << "ID should be a non-zero integer):";
		} 
		else if ( newRecord.id == 0 ) 
		{
			cout << "ID should be a non-zero integer):";
		}
		else
		{
			valid = true;
		}
	}

	cout << "Enter name (up to 32 char):";
	scanf( "%32s", newRecord.name );

	cout << "Enter age (integer):";
	valid = false;
	while( not valid )
	{
		if ( scanf( "%d", &newRecord.age ) == EOF )
		{
			cout << "Age should be a non-zero integer):";
		} 
		else if ( newRecord.age == 0 ) 
		{
			cout << "Age should be a non-zero integer):";
		}
		else
		{
			valid = true;
		}
	}

	// Now do the actual writing of the data out to the socket.
	write( sock, (char*) &newRecord, sizeof(newRecord) );

	record_t resultRec;
	bzero( &resultRec, sizeof( resultRec ) );

	read( sock, (char*) &resultRec, sizeof(resultRec) );

	if ( resultRec.command == ADD_SUCCESS )
	{
		cout << "ID " << newRecord.id << " added successfully" << endl;
	}
	else
	{
		cout << "ID " << newRecord.id << " already exists" << endl;
	}


}

void retriveRecord( int sock )
{
	record_t findRecord;
	findRecord.command = retrive_t;

	cout << "Enter id (interger):";

	bool valid = false;
	while( not valid )
	{
		if ( scanf( "%d", &findRecord.id ) == EOF )
		{
			cout << "ID should be a non-zero integer):";
		} 
		else if ( findRecord.id == 0 ) 
		{
			cout << "ID should be a non-zero integer):";
		}
		else
		{
			valid = true;
		}
	}

	// Now do the actual writing of the data out to the socket.
	write( sock, (char*) &findRecord, sizeof(findRecord) );

	record_t resultRec;
	bzero( &resultRec, sizeof( resultRec ) );

	read( sock, (char*) &resultRec, sizeof(resultRec) );

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

		// Validate the port number chosen
		if ( port < PORT_MIN || port > PORT_MAX )
		{
			cerr << port << ": invalid port number" << endl;
			exit( EXIT_FAILURE );
		}
		
		int sock = setupSocket( hostname, port );

		bool shouldQuit = false;
	  while ( not shouldQuit )	
		{

			cout << "Enter command (" << add_t << " for Add, " << retrive_t 
					 << " for Retrive, " << quit_t << " to quit):"; 

			int cmd; 
			scanf( "%d", &cmd );
			
			switch ( cmd )
			{
				case add_t:
					addRecord( sock );
					break;
				case retrive_t:
					retriveRecord( sock );
					break;
				case quit_t:
					close( sock );
					shouldQuit = true;
					break;
				default:
					break;
			}
		}

		return EXIT_SUCCESS;
	}
}
