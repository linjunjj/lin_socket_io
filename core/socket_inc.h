
#ifndef _NET_SOCKINC_H_INCLUDE__
#define _NET_SOCKINC_H_INCLUDE__

#include <assert.h>
#include <sys/types.h>

#include "utils/int_types.h"

#ifdef WIN32

// XXX before include winsock2.h
//#define FD_SETSIZE 1024

#include <winsock2.h>
typedef int socklen_t;

#define snprintf _snprintf

#else

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <net/if.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

// see: setnodelay()
#include <netinet/tcp.h>

#endif

#endif // _SNOX_SOCKINC_H_INCLUDE__
