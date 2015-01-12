#include <ctype.h>
#include "ef_sock.h"
#include "ef_utility.h"

#ifndef _WIN32____
//#include <net/if_arp.h>

#ifdef CC_TARGET_PLATFORM
#if (CC_TARGET_PLATFORM != CC_PLATFORM_IOS)
#include <sys/epoll.h>
#define LINUX_EPOLL
#endif
#endif

#include <fcntl.h>

#define _PATH_PROCNET_DEV		"/proc/net/dev"

#endif /*_WIN32*/

//#define SOCK_FUNC_TEST
#include <errno.h>
#include <cassert>
#include <string.h>
#include "ef_btype.h"

using namespace ef;

SOCKET tcp_bind_connect(const char * dsthost, int dstport, const char * localip, int localport)
{
	SOCKET fd;
	int ret;
	struct sockaddr_in cli; 
	struct sockaddr_in lcl; 
	int err;

	int reuseaddr = 1;
	int keepalive = 1;


	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (fd == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}

	lcl.sin_family = AF_INET;
	lcl.sin_addr = sock_get_hostip(localip);
	lcl.sin_port = htons(localport);	

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));

	cli.sin_family = AF_INET;
	cli.sin_addr = sock_get_hostip(dsthost);
	cli.sin_port = htons(dstport);
	if(bind(fd, (struct sockaddr *)&lcl, sizeof(struct sockaddr)) < 0){
		closesocket(fd);
		return INVALID_SOCKET;
	}

	ret = connect(fd, (struct sockaddr *)&cli, sizeof(struct sockaddr));

	if (ret != 0) {
		err = sock_errno;
		if (err != SOCK_EINPROGRESS && err != SOCK_EWOULDBLOCK) {
			closesocket(fd);
			return INVALID_SOCKET;
		}
	}

	return fd;
}

SOCKET tcp_connect(const char * dsthost, int dstport, const char * localip, int localport)
{
	SOCKET fd;
	int ret;
	struct sockaddr_in cli; 
	int err;

	int reuseaddr = 1;
	int keepalive = 1;
	//int sendbuf = 300 * 1024;
	//int recvbuf = 300 * 1024;


	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (fd == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));


	cli.sin_family = AF_INET;
	cli.sin_addr = sock_get_hostip(dsthost);
	cli.sin_port = htons(dstport);

	ret = connect(fd, (struct sockaddr *)&cli, sizeof(struct sockaddr));

	if (ret != 0) {
		err = sock_errno;
		if (err != SOCK_EINPROGRESS && err != SOCK_EWOULDBLOCK)
		{
			closesocket(fd);
			return INVALID_SOCKET;
		}
	}

	return fd;
}
/*
* consucc 取两个值 -1 失败； 0 连接成功, 连接成功包括阻塞
* 如果 返回值为 INVALID_SOKCET 表示彻底失败
*/
SOCKET tcp_nb_connect(const char * dsthost, int dstport, const char * localip, int localport, int * consucc)
{
	SOCKET fd;
	int ret;
	struct sockaddr_in cli; 
	int err;

	int reuseaddr = 1;
	int keepalive = 1;
	//int sendbuf = 300 * 1024;
	//int recvbuf = 300 * 1024;

	if (consucc != NULL) {
		*consucc = 0;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (fd == INVALID_SOCKET) {
		if (consucc != NULL) {
			*consucc = -1;
		}

		return INVALID_SOCKET;
	}

	set_socket_nonblocking(fd);

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));


	cli.sin_family = AF_INET;
	cli.sin_addr = sock_get_hostip(dsthost);
	cli.sin_port = htons(dstport);

	ret = connect(fd, (struct sockaddr *)&cli, sizeof(struct sockaddr));

	if (ret != 0) {
		err = sock_errno;
		if (err != SOCK_EINPROGRESS && err != SOCK_EWOULDBLOCK) {
			if (consucc != NULL) {
				*consucc = -1;
			}

			closesocket(fd);

			return INVALID_SOCKET;
		}
	}

	return fd;
}

/*
* consucc 取两个值，-1 失败； 0 连接成功, 连接成功包括阻塞
* 如果返回值为INVALID_SOCKET, 表示彻底失败
*/
SOCKET tcp_nb_connect2(struct in_addr dstip, int dstport, const char * localip, int localport, int *consucc)
{
	SOCKET fd;
	int ret;
	int err;
	struct sockaddr_in cli; 

	int reuseaddr = 1;
	int keepalive = 1;

	assert(dstport >= 0 && dstport <= 65535);

	if (consucc != NULL) {
		*consucc = 0;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0) {
		if (consucc != NULL) {
			*consucc = -1;
		}

		return INVALID_SOCKET;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));

	set_socket_nonblocking(fd);

	cli.sin_family = AF_INET;
	cli.sin_addr = dstip;
	cli.sin_port = htons(dstport);

	ret = connect(fd, (struct sockaddr *) &cli, sizeof(cli));
	if (ret != 0) {
		err = sock_errno;
		if (err != SOCK_EINPROGRESS && err != SOCK_EWOULDBLOCK) {
			if (consucc != NULL) {
				*consucc = -1;	//连接失败
			}
			closesocket(fd);
			return INVALID_SOCKET;
		}
	}

	return fd;
}


/*
*函数作用:
*			 以阻塞方式 向套接字发送指定长度数据
*			 发送成功或超时时退出
*
*
*输入参数:
*			  fd 套接字
*			  sendbuf 发送缓冲区
*			  towrite 发送字节数
*			  waitms 超时时间，waitms <= 0表示一直阻塞直到发送完成。
*
*输出参数:
*			  actsndnum 为实际发送的字节数
*
*
* 返回值 ：
*			  > 0 成功，返回实际发送的字节数。
*			  < 0 失败，套接字关闭或其它错误。
*			  = 0 失败，发送数据超时
*/
int tcp_send(SOCKET fd, const char *sendbuf, int towrite, long waitms, int *actsndnum)
{
	int ret = 0;
	int sendLen = 0;
	int errcode = 0;
	int bytes = 0;
	int restms = 0;
	int sval = 0;
	int usval = 0;
	int cast_ms = 0;
	
	struct timeval tick0;
	struct timeval tick1;

#ifdef LINUX_EPOLL
	int ep = 0;
	struct epoll_event ee;
	struct epoll_event ev_list[16];
#else
	struct timeval tv;
	struct timeval *ptv = NULL;
	fd_set wFDs;
#endif
	
#ifdef SOCK_FUNC_TEST

#ifdef LINUX_EPOLL
	char *backend = "epoll";
#else
	char *backend = "select";
#endif

	int wait_times = 0;
	struct timeval t1, t2;
#endif

	if (actsndnum != NULL) {
		*actsndnum = 0;
	}

	if (fd == INVALID_SOCKET) {
		return -1;
	}

	if (sendbuf == NULL || towrite <= 0) {
		return -1;
	}

	if (waitms > 0) {
		gettimeofday(&tick0, NULL);
		restms = waitms;
	}

	sendLen = towrite;
	do {
		bytes = send(fd, sendbuf, sendLen, 0);
		if (bytes > 0) {
			sendLen -= bytes;
			sendbuf += bytes;
			if (actsndnum != NULL) {
				*actsndnum += bytes;
			}

		}
		else if (bytes == -1) {
			
			errcode = sock_errno;

			if (errcode ==  SOCK_EINTR) {

				continue;
			}
			else if (errcode == SOCK_EAGAIN 
				|| errcode == SOCK_EWOULDBLOCK) {
				
				if (waitms > 0) {
					/*check time out time*/
					gettimeofday(&tick1, NULL);
					usval = tick1.tv_usec - tick0.tv_usec;
					sval = tick1.tv_sec - tick0.tv_sec;

					if (usval < 0) {
						sval -= 1;
						usval += 1000000;
					}


					cast_ms = sval * 1000 + usval / 1000;
					restms = waitms - cast_ms;
					
					if (restms <= 0) {
						/*time out*/
						ret = 0;
						break; /*break from do {} while*/
					}

				}
				else if (waitms == 0) {
					/*restms must be -1 when waitms <= 0*/
					restms = -1;
				}
				else {
					restms = -1;
				}
				

#ifdef LINUX_EPOLL
				if (ep == 0) {
					ep = epoll_create(16);
					if (ep < 0) {
						return -1;
					}
					
					memset(&ee, 0, sizeof(struct epoll_event));
					ee.events = EPOLLOUT;
						ee.data.ptr = NULL;
					if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ee) == -1) {
						close(ep);
						return -1;
					}
				}

				ret = epoll_wait(ep, ev_list, (int)128, restms);
#else				
				if (restms > 0) {
					tv.tv_sec = restms / 1000;
					tv.tv_usec = (restms % 1000) * 1000;
					ptv = &tv;
				}
				else if (restms == 0) {
					tv.tv_sec = 0;
					tv.tv_usec = 0;
					ptv = &tv;
				}
				else {
					/*infinite*/
					//fprintf(stdout, "[TRACE] tcp_send : wait ms <= 0\n");
					ptv = NULL;
				}

				FD_ZERO(&wFDs);
				FD_SET(fd, &wFDs);
				ret = select(fd + 1, NULL, &wFDs, NULL, ptv);
#endif /*LINUX_EPOLL*/
			

				if (ret == -1) {
					errcode = sock_errno;
					if (errcode == SOCK_EINTR) {
						continue;
					}
					else {
						ret = -1;
						break; /*break from do {} while*/
					}
				}
				else if (ret == 0) {
					/*time out*/
					ret = 0;
					break; /*break from do {} while*/
				}
				else {
#ifdef LINUX_EPOLL
					if (ev_list[0].events & EPOLLOUT) {
						/*fd can be write*/
						//fprintf(stdout, "[TRACE] tcp_send : fd is writable with epoll_wait\n");
						continue;
					}
					else {
						ret = -1;
						break;
					}
#else
					if (FD_ISSET(fd, &wFDs)) {
						/*fd can be write*/
						continue;
					}
					else {
						ret = -1;
						break;
					}
#endif /*LINUX_EPOLL*/					
				}

			}
			else {
				/*Unexpected error*/
				ret = -1;
				break; /*break from do {} while*/
			}
		}
		else {
			/*bytes == 0, socket has closed*/
			ret = -1;
			break;
		}
	}while (sendLen > 0);


#ifdef LINUX_EPOLL
	if (ep != 0) {
		close(ep);
	}
#endif

	if (sendLen != 0) {
		return ret;
	}

	return towrite;
}

/*
*函数作用:
*			 以阻塞方式 从套接字读取指定长度数据
*			 读取成功或超时时退出
*
*
*输入参数:
*			  fd 套接字
*			  rcvbuf 发送缓冲区
*			  toread 发送字节数
*			  waitms 超时时间，waitms <= 0表示一直阻塞直到读取完成。
*
*输出参数:
*			  actrcvnum 为实际发送的字节数
*
*
* 返回值 ：
*			  > 0 成功，返回实际读取的字节数。
*			  < 0 失败，套接字关闭或其它错误。
*			  = 0 失败，读取数据超时
*/
int tcp_receive(SOCKET fd, char * rcvbuf, int toread, long waitms, int *actrcvnum)
{
	int ret = 0;
	int readLen = 0;
	int errcode = 0;
	int bytes = 0;
	int restms = 0;
	int sval = 0;
	int usval = 0;
	int cast_ms = 0;

	struct timeval tick0;
	struct timeval tick1;

#ifdef LINUX_EPOLL
	int ep = 0;
	struct epoll_event ee;
	struct epoll_event ev_list[16];
#else
	struct timeval tv;
	struct timeval *ptv = NULL;
	fd_set rFDs;
#endif	
	
#ifdef SOCK_FUNC_TEST

#ifdef LINUX_EPOLL
	char *backend = "epoll";
#else
	char *backend = "select";
#endif

	int wait_times = 0;
	struct timeval t1, t2;
#endif

	if (actrcvnum != NULL) {
		*actrcvnum = 0;
	}

	if (fd == INVALID_SOCKET) {
		return -1;
	}

	if (rcvbuf == NULL || toread <= 0) {
		return -1;
	}

	if (waitms > 0) {
		gettimeofday(&tick0, NULL);
		restms = waitms;
	}
	set_socket_nonblocking(fd);

	readLen = toread;
	do {
		bytes = recv(fd, rcvbuf, readLen, 0);
		if (bytes > 0) {
			readLen -= bytes;
			rcvbuf += bytes;
			if (actrcvnum != NULL) {
				*actrcvnum += bytes;
			}

		}
		else if (bytes == -1) {
			
			errcode = sock_errno;

			if (errcode == SOCK_EAGAIN 
				|| errcode == SOCK_EWOULDBLOCK || errcode ==  SOCK_EINTR) {
				
				if (waitms > 0) {
					/*check time out time*/
					gettimeofday(&tick1, NULL);
					usval = tick1.tv_usec - tick0.tv_usec;
					sval = tick1.tv_sec - tick0.tv_sec;

					if (usval < 0) {
						sval -= 1;
						usval += 1000000;
					}

					cast_ms = sval * 1000 + usval / 1000;
					restms = waitms - cast_ms;
					
					if (restms <= 0) {
						/*time out*/					
						ret = 0;
						break; /*break from do {} while*/
					}
					
				}
				else if (waitms == 0) {
					/*restms must be -1 when waitms <= 0*/
					restms = -1;
				}
				else {
					restms = -1;
				}
		

#ifdef LINUX_EPOLL
				if (ep == 0) {
					ep = epoll_create(16);
					if (ep < 0) {
						return -1;
					}
					
					memset(&ee, 0, sizeof(struct epoll_event));
					ee.events = EPOLLIN;
						ee.data.ptr = NULL;
					if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ee) == -1) {
						close(ep);
						return -1;
					}
				}

				ret = epoll_wait(ep, ev_list, (int)128, restms);
#else
				if (restms > 0) {
					tv.tv_sec = restms / 1000;
					tv.tv_usec = (restms % 1000) * 1000;
					ptv = &tv;
				}
				else if (restms == 0) {
					tv.tv_sec = 0;
					tv.tv_usec = 0;
					ptv = &tv;				
				}
				else {
					/*infinite*/
					//fprintf(stdout, "[TRACE] tcp_receive : wait ms <= 0\n");
					ptv = NULL;
				}
			
				FD_ZERO(&rFDs);
				FD_SET(fd, &rFDs);
				ret = select(fd + 1, &rFDs, NULL, NULL, ptv);
#endif /*LINUX_EPOLL*/
			

				if (ret == -1) {
					errcode = sock_errno;
					if (errcode == SOCK_EINTR) {
						continue;
					}
					else {
						ret = -1;
						break; /*break from do {} while*/
					}
				}
				else if (ret == 0) {
					/*time out*/

#ifdef SOCK_FUNC_TEST
	if (wait_times > 1) {
		fprintf(stdout, "[DEBUG] tcp_receive : wait times = %d\n", wait_times);
	}
#endif				
					ret = 0;
					break; /*break from do {} while*/
				}
				else {
#ifdef LINUX_EPOLL					
					if (ev_list[0].events & EPOLLIN) {
						/*fd can be write*/
						continue;
					}
					else {
						ret = -1;
						break;
					}
#else
					if (FD_ISSET(fd, &rFDs)) {
						/*fd can be write*/
						continue;
					}
					else {
						ret = -1;
						break;
					}
#endif
				}

			}
			else {
				/*Unexpected error*/
				ret = -1;
				break; /*break from do {} while*/
			}
		}
		else {
			/*bytes == 0, socket has closed*/
			ret = -1;
			break;
		}
	}while (readLen > 0);


#ifdef LINUX_EPOLL
	if (ep != 0) {
		close(ep);
	}
#endif

	if (readLen != 0) {
		return ret;
	}

	return toread;
}

struct in_addr sock_get_hostip (const char *host)
{
	struct hostent *remoteHost;
	struct in_addr addr;

	addr.s_addr = INADDR_NONE;

	if (host == NULL) {
		return addr;
	}

	if (isalpha(host[0])) {
		remoteHost = gethostbyname(host);
		if (remoteHost == NULL) {
			return addr;
		}

		addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
	}
	else {
		addr.s_addr = inet_addr(host);
	}
	
	return addr;
}

int sock_get_pending (SOCKET fd)
{

	int ret;

#ifndef _WIN32
	int nread;
	ret = ioctl(fd, FIONREAD, &nread);
#else
	ulong nread;
	ret = ioctlsocket(fd, FIONREAD, &nread);
#endif

	if (ret != 0) {
		return 0;
	}

	return nread;
}

/*
*函数作用:
*			  判断套接字是否可读
*
*输入参数:
*			  fd 套接字
*			  ms 超时时间
*
* 返回值 ：
*			  = 1 成功，套接字可读
*			  = 0 失败，套接字关闭或其它错误。
*/

int sock_read_ready(SOCKET fd, int ms)
{
	int ret;

#ifdef LINUX_EPOLL
	int ep = 0;
	uint32 revents;
	struct epoll_event ee;
	struct epoll_event ev_list[16];

	ep = epoll_create(16);
	if (ep < 0) {
		return 0;
	}

	memset(&ee, 0, sizeof(struct epoll_event));
	ee.events = EPOLLIN;
	ee.data.ptr = NULL;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ee) == -1) {
		close(ep);
		return 0;
	}

	ret = epoll_wait(ep, ev_list, (int)128, ms);
	if (ret != 1) {
		close(ep);
		return 0;
	}

	revents = ev_list[0].events;
	if (revents & EPOLLIN) {
		close(ep);
		return 1;
	}

	close(ep);

	return 0;
	
#else
	
	fd_set fdset;
	struct timeval tv;

	tv.tv_sec = ms / 1000;
	tv.tv_usec = ms % 1000 * 1000;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	ret = select(fd + 1, &fdset, NULL, NULL, &tv);
	if (ret > 0) {
		if (FD_ISSET(fd, &fdset)) {
			return 1;
		}
	}

#endif

	return 0;
}
	

/*
*函数作用:
*			  判断套接字是否可写
*
*输入参数:
*			  fd 套接字
*			  ms 超时时间
*
* 返回值 ：
*			  = 1 成功，套接字可写
*			  = 0 失败，套接字关闭或其它错误。
*/

int sock_write_ready(SOCKET fd, int ms)
{
	int ret;

#ifdef LINUX_EPOLL
	int ep = 0;
	uint32 revents;
	struct epoll_event ee;
	struct epoll_event ev_list[16];

	ep = epoll_create(16);
	if (ep < 0) {
		return 0;
	}

	memset(&ee, 0, sizeof(struct epoll_event));
	ee.events = EPOLLOUT;
	ee.data.ptr = NULL;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ee) == -1) {
		close(ep);
		return 0;
	}

	ret = epoll_wait(ep, ev_list, (int)128, ms);
	if (ret != 1) {
		close(ep);
		return 0;
	}

	revents = ev_list[0].events;
	if (revents & EPOLLOUT) {
		close(ep);
		return 1;
	}

	close(ep);

	return 0;
	
#else

	fd_set fdset;
	struct timeval tv;

	tv.tv_sec = ms / 1000;
	tv.tv_usec = ms % 1000 * 1000;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	ret = select(fd + 1, NULL, &fdset, NULL, &tv);
	if (ret > 0) {
		if (FD_ISSET(fd, &fdset)) {
			return 1;
		}
	}
#endif	

	return 0;
}

/*
*函数作用:
*			  从非阻塞套接字中读取数据，语义类似recv，只不过recv是
*			  阻塞操作，而tcp_nb_receive遇到EAGAIN时，不阻塞返回。
*
*输入参数:
*			  fd 套接字
*			  rcvbuf 接收缓冲区
*			  bufsize 接收缓冲区大小
*
*输出参数:
*			  actrcvnum为实际读取的字节数
*
*
* 返回值 ：
*			  > 0 成功，返回实际读取的字节数。
*			  = 0 成功，返回0是可能的，表示套接字没有数据可读。
*			  < 0 失败，套接字关闭或其它错误。
*/
int tcp_nb_receive (SOCKET fd, char *rcvbuf, int bufsize, int *actrcvnum)
{
	int ret = 0;
	int readLen = 0;
	int errcode = 0;

	if (actrcvnum != NULL) {
		*actrcvnum = 0;
	}

	if (fd == INVALID_SOCKET 
		|| rcvbuf == NULL 
		|| bufsize <= 0) {
		return -1;
	}

	set_socket_nonblocking(fd);
	readLen = 0;

	do {
		ret = recv(fd, rcvbuf, bufsize, 0);
		if (ret > 0) {
			readLen += ret;
			if (actrcvnum != NULL) {
				*actrcvnum += ret;
			}

			rcvbuf += ret;
			bufsize -= ret;
		}
		else if (ret == 0) {
			/*Socket closed*/
			return -1;
		}
		else if (ret == SOCKET_ERROR) {
			
			errcode = sock_errno;
			
			if (errcode == SOCK_EINTR) {
				continue;
			}
			else if (errcode == SOCK_EAGAIN 
				|| errcode == SOCK_EWOULDBLOCK) {
				/*No data can be read*/
				break;
			}
			else {
				/*Unexpected error*/
				return -1;
			}
		}
	}while(bufsize > 0);

	return readLen;
}

int tcp_receive (SOCKET fd, char *rcvbuf, int bufsize, int *actrcvnum)
{
	int ret = 0;
	int readLen = 0;
	int errcode = 0;

	if (actrcvnum != NULL) {
		*actrcvnum = 0;
	}

	if (fd == INVALID_SOCKET 
		|| rcvbuf == NULL 
		|| bufsize <= 0) {
		return -1;
	}

	readLen = 0;

	do {
		ret = recv(fd, rcvbuf + readLen, bufsize, 0);
		if (ret > 0) {
			readLen += ret;
			if (actrcvnum != NULL) {
				*actrcvnum += ret;
			}

			rcvbuf += ret;
			bufsize -= ret;
		}
		else if (ret == 0) {
			/*Socket closed*/
			return -1;
		}
		else if (ret == SOCKET_ERROR) {
			
			errcode = sock_errno;
			
			if (errcode == SOCK_EINTR) {
				continue;
			}
			else if (errcode == SOCK_EAGAIN 
				|| errcode == SOCK_EWOULDBLOCK) {
				/*No data can be read*/
				break;
			}
			else {
				/*Unexpected error*/
				return -1;
			}
		}
	}while(bufsize > 0);

	return readLen;
}


int tcp_send (SOCKET fd, const char *rcvbuf, int bufsize, int *actrcvnum)
{
        int ret = 0;
        int readLen = 0;
        int errcode = 0;
                
        if (actrcvnum != NULL) {
                *actrcvnum = 0;
        }

        if (fd == INVALID_SOCKET
                || rcvbuf == NULL
                || bufsize <= 0) {
                return -1;
        }

        readLen = 0;

        do {
                ret = send(fd, rcvbuf + readLen, bufsize, 0);
                if (ret > 0) {
                        readLen += ret;
                        if (actrcvnum != NULL) {
                                *actrcvnum += ret;
                        }

                        rcvbuf += ret;
                        bufsize -= ret;
                }
                else {

                        errcode = sock_errno;

                        if (errcode == SOCK_EINTR) {
                                continue;
                        }
                        else if (errcode == SOCK_EAGAIN
                                || errcode == SOCK_EWOULDBLOCK) {
                                /*No data can be read*/
                                break;
                        }
                        else {
                                /*Unexpected error*/
                                return -1;
                        }
                }
        }while(bufsize > 0);

        return readLen;
}

int set_socket_nonblocking(int fd)
{
#ifdef _WIN32
	{
		unsigned long nonblocking = 1;
		ioctlsocket(fd, FIONBIO, (unsigned long*) &nonblocking);
	}

#else

	{
		unsigned long nonblocking = 1;
		ioctl(fd, FIONBIO, (void *)&nonblocking);
	}
#endif
	return 0;
}


SOCKET tcp_server_create (const char * local_ip, int port)
{

	int fd;
	struct sockaddr_in srv; 

	int reuseaddr = 1;
	int keepalive = 1;

	assert(port >= 0 && port <= 65535);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd <= 0) {
		return INVALID_SOCKET;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));

	srv.sin_family = AF_INET;
	if (strcmp(local_ip, "0.0.0.0") == 0) {
		srv.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		srv.sin_addr.s_addr = inet_addr(local_ip);
	}
	srv.sin_port = htons(port);

	if (bind( fd, (struct sockaddr *) &srv, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		closesocket(fd);
		return SOCKET_ERROR;
	}

	if (listen( fd, 1024 ) == SOCKET_ERROR) {
		closesocket(fd);
		return SOCKET_ERROR;
	}

	return fd;
}

SOCKET udp_server_create (struct in_addr * localip, int port)
{
	SOCKET	fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		return INVALID_SOCKET;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	if (localip != NULL) {
		addr.sin_addr = *localip;
	}
	else {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);	
	}
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) != 0) {
		sock_close(fd);
		return INVALID_SOCKET;
	}


	return fd;
}

int sock_set_blocking(SOCKET socket, int blocking)
{
	unsigned long nonblocking;

	if (blocking == 0) {
		nonblocking = 1;
	}
	else {
		nonblocking = 0;
	}
	
#ifndef _WIN32
	ioctl(socket, FIONBIO, (void *)&nonblocking);
#else
	ioctlsocket(socket, FIONBIO, (unsigned long*) &nonblocking);
#endif

	return 0;
}

#ifdef UNIX
/*
return value:
	0 socket is invalid
	1 socket is valid
*/
int sock_is_valid(SOCKET fd)
{

#ifdef LINUX_EPOLL
	int  ret;
	int ep = 0;
	uint32 revents;
	struct epoll_event ee;
	struct epoll_event ev_list[16];

	ep = epoll_create(16);
	if (ep < 0) {
		return 0;
	}

	memset(&ee, 0, sizeof(struct epoll_event));
	ee.events = EPOLLIN;
	ee.data.ptr = NULL;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ee) == -1) {
		close(ep);
		return 0;
	}

	ret = epoll_wait(ep, ev_list, (int)128, 0);

	close(ep);
	
	if (ret == -1) {
		return 0;
	}
	else 	if (ret == 0) {
		return 1;
	}
	else if (ret > 0) {
		revents = ev_list[0].events;
		if (revents & EPOLLIN) {
			return 1;
		}
		else {
			return 0;
		}
	}

	return 0;
	
#else

	fd_set rfds;
	int retval = 0, len = 0;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;	

	retval = select(fd + 1, &rfds, NULL , NULL, &tv);
	if (retval == -1) 
		return 0;
	
	if (retval == 0) 
		return 1;
	
	if (!FD_ISSET(fd, &rfds)) 
		return 1;

#ifndef _WIN32
	retval= ioctl (fd,FIONREAD,&len);
#else
	retval= ioctlsocket (fd,FIONREAD,&len);
#endif

	if ( retval < 0) 
		return 0;

	if (len<=0)
		return 0;
	else
		return 1;

#endif /*LINUX_EPOLL*/

}

int make_tcp_socket_pair(SOCKET fd[2]){
	int start_port = 2000;
	int cnt = 0;
	fd[0] = INVALID_SOCKET;
		
	int i = rand() % 60000;
	while(cnt < 50){
		fd[0] = tcp_server_create(start_port + i);
	}
}

#endif

