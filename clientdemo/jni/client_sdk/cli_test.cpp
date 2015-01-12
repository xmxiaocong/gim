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
 #include <ctype.h>
#include <netdb.h>
#include <stdio.h>

using namespace std;

struct in_addr sock_get_hostip(const char *host)
{
	struct hostent *remoteHost;
	struct in_addr addr;

	addr.s_addr = INADDR_NONE;

	if (host == NULL) 
	{
		return addr;
	}

	if (isalpha(host[0]))
	{
		remoteHost = gethostbyname(host);
		if (remoteHost == NULL) {
			return addr;
		}

		addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
	}
	else 
	{
		addr.s_addr = inet_addr(host);
	}

	return addr;
}

int connect_server(const char * local_ip, int port)
{
	int fd;
	int ret;
	struct sockaddr_in cli;
	int err;

	int reuseaddr = 1;
	int keepalive = 1;


	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (fd == -1) 
	{
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(reuseaddr));
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));


	cli.sin_family = AF_INET;
	cli.sin_addr = sock_get_hostip(local_ip);
	cli.sin_port = htons(port);

	ret = connect(fd, (struct sockaddr *)&cli, sizeof(struct sockaddr));

	if (ret != 0)
	{
		if (err != EINPROGRESS && err != EWOULDBLOCK)
		{
			close(fd);
			return -1;
		}
	}

	return fd;
}

int main()
{
	int ret;
	const char* ip = "127.0.0.1";
	int port = 14701;
	const int count = 300;
	int fds[count];
	for (int n = 0; n < count; n++)
	{
		fds[n] = connect_server(ip, port);
	}
	while (1)
	{
		string cmd;
		cin >> cmd;
		if(cmd != "send")
		{
			continue;
		}
		for (int n = 0; n < count; n++)
		{
			if (fds[n] != -1)
			{
				char buf[100] = { 0 };
				sprintf(buf, "this is %d", fds[n]);
				int len = 100;
				ret = send(fds[n], buf, len, 0);
				if (ret <= 0)
				{
					close(fds[n]);
					fds[n] = -1;
				}

			}
		}
		
			
	}
	return 0;
}
