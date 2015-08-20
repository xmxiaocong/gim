#include <iostream>
#include <sys/epoll.h>
#include <sstream>
#include <fstream>
#include <string>
#include "net/ef_sock.h"
#include "proto/msg_head.h"
#include "proto/connect_server.pb.h"
#include "base/ef_utility.h"
#include "base/ef_log.h"

using namespace ef;
using namespace gim;
using namespace std;

int main(int argc, char **argv)
{
	LoginRequest lgreq;
	RedirectResponse rdresp;
	string lgreqbuf;
	SOCKET sockfd;

	lgreq.set_id("131aseasdlkle");
	lgreq.set_type(0);
	lgreq.set_version("1.0.1");
	lgreq.set_token("slakjflejejr");
	lgreq.SerializeToString(&lgreqbuf);

	sockfd = tcpConnect("127.0.0.1", 13000, NULL, 0);
	if (sockfd < 0) {
		std::cout << "connect server fail" << std::endl;
		return -1;
	}

	head rh;
	rh.cmd = htonl(LOGIN_REQ);
	rh.magic = 20150624;
	rh.len = htonl(sizeof(head) + lgreqbuf.size());
	string respbuf;
	respbuf.append((char *)&rh, sizeof(rh));
	respbuf.append(lgreqbuf);
	tcpSend(sockfd, respbuf.data(), respbuf.size(), 100, NULL);
	char buf[1024];
	tcpReceive(sockfd, buf, 1024, 100, NULL);
	head h1 = *(head*)buf;
	h1.cmd = htonl(h1.cmd);
	h1.len = htonl(h1.len);
	rdresp.ParseFromArray(buf + sizeof(h1), h1.len - sizeof(h1));
	for (int i = 0; i < rdresp.addrs_size(); i++) {
		std::cout << rdresp.addrs(i).ip() << ":" << rdresp.addrs(i).port() << std::endl;
	}	
	return 0;
}
