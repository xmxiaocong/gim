#include  <sys/epoll.h>
#include <string>
#include <iostream>
#include <set>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
using namespace std;

int start_serve(const char * local_ip, int port)
{
	int ret = 0;
	int eplfd = -1;

	eplfd = epoll_create(1024);
	if (eplfd < 0)
	{
		cout << "epoll_create fail ret = " << eplfd << endl;
		return -1;
	}
	int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenfd <= 0)
	{
		cout << "socket err listenfd = " << listenfd << endl;
		return -1;
	}
	int reuseaddr = 1;
	int keepalive = 1;

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));

	unsigned long nonblocking = 1;
	ioctl(listenfd, FIONBIO, (void *)&nonblocking);

	struct sockaddr_in srv;
	if (!local_ip)
	{
		srv.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		srv.sin_addr.s_addr = inet_addr(local_ip);
	}
	srv.sin_port = htons(port);
	ret = bind(listenfd, (struct sockaddr*) & srv, sizeof(sockaddr));
	if (ret < 0)
	{
		cout << "bind err " << endl;
		return -1;
	}

	ret = listen(listenfd, 1024);
	if (ret < 0)
	{
		cout << "listen fail ret = " << ret << endl;
		close(listenfd);
		return -1;
	}

	//add listen event
	{
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = listenfd;
		ret = epoll_ctl(eplfd, EPOLL_CTL_ADD, listenfd, &ev);
		if (ret < 0)
		{
			cout << "epoll add listenfd fail ret " << ret << endl;
			close(listenfd);
		}
	}

	set<int> clifds;
	int count = 0;
	while (1)
	{
		const int epoolmax = 1024;
		struct epoll_event evs[epoolmax];
		int fds = epoll_wait(eplfd, evs, epoolmax, -1);
		for (int n = 0; n < epoolmax;n++)
		{
			int fd = evs[n].data.fd;
			if (fd == listenfd)
			{
				int clientfd;
				struct sockaddr_in clientaddr;
				int arrlen = sizeof(clientaddr);
				clientfd = accept(listenfd, (struct sockaddr*)&clientaddr,(socklen_t*) &arrlen);
				if (clientfd == -1)
				{
					cout << "accept fail" << endl;
					continue;
				}
				
				//add client
				{
					cout << "accept" << endl;
					unsigned long nonblocking = 1;
					ioctl(clientfd, FIONBIO, (void *)&nonblocking);

					struct epoll_event ev;
					ev.events = EPOLLIN;
					ev.data.fd = clientfd;
					ret = epoll_ctl(eplfd, EPOLL_CTL_ADD, clientfd, &ev);
					clifds.insert(clientfd);
				}

			}
			else
			{
				if(clifds.find(fd) == clifds.end())
					continue;

				char buf[1024] = { 0 };
				ret = recv(fd, buf, sizeof(buf), 0);
				if (ret > 0)
				{
					cout << "recv [" << buf << "]" << endl;
				}
				else 
				{
					if (ret == 0)
					{
						cout << "socket close" << endl;
					}
					else if(errno == EAGAIN)
					{
						continue;
					}
					else
					{
						cout << "socket error:" << errno << endl;
					}
					close(fd);
					clifds.erase(fd);
					struct epoll_event ev;
					ev.data.fd = fd;
					epoll_ctl(eplfd, EPOLL_CTL_DEL, fd, &ev);
				}
			}
		}
	}
}

int main()
{
	const char* ip = "127.0.0.1";
	int port = 14701;
	start_serve(ip, port);
	return 0;
}
