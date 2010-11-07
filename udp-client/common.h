/**
 * Author: Brian Gianforcaro ( bjg1955@cs.rit.edu )
 *
 * Description: A header of constants which are used
 * in the projects client application.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#define	PORT_MIN 1024
#define	PORT_MAX 65535

#define HOSTNAME 1
#define PORTNUM 2

#define ADD_SUCCESS 0
#define ADD_FAILURE 1

#define RET_SUCCESS 0
#define RET_FAILURE 1

#define MAX_LEN 32

/**
 * A struct defining the data storage
 * and nework communcation data format 
 */
typedef struct
{ 
  int command; 
  int id; 
  char name[MAX_LEN]; 
  int age;
} record_t;


/* Enum defining all actions the user can initiate */
typedef enum {
  add_t = 0,
  retrive_t = 1,
  quit_t = 2
} actions_t;

#endif // _COMMON_H
