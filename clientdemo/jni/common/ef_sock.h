#ifndef EF_SOCK_H
#define EF_SOCK_H

#include <stdio.h>
#include "ef_btype.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _WIN32


#include <stddef.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <arpa/inet.h>

#include <sys/ioctl.h>

/* in Windows Socket, nonblocking operation results are identified by WSAEWOULDBLOCK */
#define SOCK_EINPROGRESS		EINPROGRESS      
#define SOCK_EWOULDBLOCK		EWOULDBLOCK
#define SOCK_EAGAIN			EAGAIN
#define SOCK_EINTR			EINTR
#define SOCK_EBADF			EBADF
#define SOCK_EINVAL			EINVAL
#define SOCK_ENOTSOCK			ENOTSOCK

#define sock_errno			errno

#define sock_close			close

#define closesocket			close

#define SOCKET_ERROR			(-1)

typedef	int	SOCKET;

/* Check if socket is in blocking or non-blocking mode.  Return -1 for
 * error, 0 for nonblocking, 1 for blocking. */
int sock_query_blocking(int socket);

int sock_is_valid (SOCKET fd);


#else/*_WIN32*/

#include <Iphlpapi.h>

 /* in Windows Socket, nonblocking operation results are identified by WSAEWOULDBLOCK */
#define SOCK_EINPROGRESS		WSAEWOULDBLOCK     
#define SOCK_EWOULDBLOCK		WSAEWOULDBLOCK
#define SOCK_EAGAIN			WSAEWOULDBLOCK
#define SOCK_EINTR			WSAEINTR
#define SOCK_EBADF			WSAEBADF
#define SOCK_EINVAL			WSAEINVAL
#define SOCK_ENOTSOCK			WSAENOTSOCK

#define sock_errno			WSAGetLastError()

#define sock_close			closesocket

#endif /*if WINDOWS*/


#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif


#define ADDR_TYPE_UNKNOWN   0
#define ADDR_TYPE_ETHERNET  1
#define ADDR_TYPE_TOKENRING 2
#define ADDR_TYPE_FDDI      3
#define ADDR_TYPE_PPP       4
#define ADDR_TYPE_LOOPBACK  5
#define ADDR_TYPE_SLIP      6


/* Set socket to blocking or non-blocking mode.  Return -1 for error,
 * 0 for success. */
int sock_set_blocking(SOCKET socket, int blocking);

int set_socket_nonblocking(int fd);

struct in_addr sock_get_hostip (const char * host);

/* get the peer ip address from the socket FD */
struct in_addr sock_get_peer_ip(SOCKET s);

/* determine socket is open or not */
//int sock_is_open (SOCKET fd);

/* get the number of the pending bytes from the socket FD */
int sock_get_pending (SOCKET fd);

/* determine if the socket fd is read already, the 2nd para is milliseconds waiting for */
int sock_read_ready (SOCKET fd, int ms);
int sock_write_ready (SOCKET fd, int ms);


/* Open a server socket. Return -1 for error, >= 0 socket number for OK.*/
SOCKET tcp_server_create (const char * local_ip, int port);

/* Open a client socket with binding a local address. */
SOCKET tcp_connect_with_timeout (const char * dsthost, int dstport, int timeout_ms);
SOCKET tcp_connect (const char * dsthost, int dstport, const char * localip, int localport);
SOCKET tcp_bind_connect (const char * dsthost, int dstport, const char * localip, int localport);

/* As above, but given a destination ip address */
SOCKET tcp_connect2 (struct in_addr dstip, int dstport, const char * localip, int localport);


SOCKET tcp_nb_connect (const char * dsthost, int dstport, 
                       const char * localip, int localport, int * consucc);
SOCKET tcp_nb_connect2 (struct in_addr dstip, int dstport, 
                        const char * localip, int localport, int * consucc);

SOCKET udp_server_create (struct in_addr * localip, int port);

int tcp_receive    (SOCKET fd, char * rcvbuf, int toread, long waitms, int * actrcvnum);
int tcp_nb_receive (SOCKET fd, char * rcvbuf, int bufsize, int * actrcvnum);
int tcp_send       (SOCKET fd, const char * sendbuf, int towrite, int * actsndnum);


int make_tcp_socket_pair(SOCKET fd[2]);
#ifdef __cplusplus
}
#endif

#endif

