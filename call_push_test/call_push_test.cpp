#include <stdlib.h>
#include <string>
#include <iostream>
#include "net/ef_sock.h"
#include <json/json.h>
#include "base/ef_btype.h"
#include <errno.h>
#include "base/ef_thread.h"
#include "base/ef_utility.h"

using namespace std;
using namespace ef;

struct head{
	int magic;
	int len; 
	int cmd;			
};

ef::SOCKET  fd = EF_INVALID_SOCKET;
bool run = true;
int g_send_count = 1;

void* recv_thread(void* args){
	int recvcount=0;
	while (recvcount<g_send_count)
	{
		if (fd == EF_INVALID_SOCKET){
			sleep(1);
			continue;
		}

		head h;
		int len=0;
		ef::tcpReceive(fd, (char*)&h, sizeof(h), 0, &len);
		h.cmd = ntohl(h.cmd);
		h.magic=ntohl(h.magic);
		h.len = ntohl(h.len);

		cout << "h.len="<<h.len << ", h.cmd=" << h.cmd << endl;

		string buf;
		buf.resize(h.len-sizeof(h));
		ef::tcpReceive(fd, (char*)buf.data(), buf.size(), 0, &len);

		cout << "receive:" << buf << endl;
		recvcount++;
	}

	return NULL;
}

int main(int argc, char* const* argv)
{
	
	if (argc > 1){
		g_send_count = atoi(argv[1]);
	}

	fd = tcpConnect("127.0.0.1", 11147, "0.0.0.0", 0);
	if(fd == EF_INVALID_SOCKET){
		cout << "connect error:"<< sock_errno << endl;
	}
	ef::THREADHANDLE thrd;
	run = true;
	ef::threadCreate(&thrd, NULL, recv_thread, NULL);

	int count=0;
	while (count++ < g_send_count)
	{
		cout << "this is test no:" << count << endl;
		Json::Value v;
		v["cmd"] = 1;
		v["type"] = 101;
		v["time"] = "20151010";
		v["to"] = "test0001";
		v["from"] = "test0002";
		v["data"] = ef::itostr(count);
		v["sn"] = ef::itostr(count);
		Json::FastWriter w;
		string jsonstr = w.write(v);

		int len=0;
		head h;
		h.cmd = ntohl(200);
		h.magic=ntohl(0xffff);
		h.len = ntohl(sizeof(h)+jsonstr.size());

		string sendbuf;
		sendbuf.reserve(h.len);
		sendbuf.append((char*)&h, sizeof(h));
		sendbuf.append(jsonstr);

		if (ef::tcpSend(fd, sendbuf.c_str(), sendbuf.size(), 0, &len) < 0){
			cout << "tcpsend faild:" << sock_errno << endl;
		}
	}
	threadJoin(&thrd);
	return 0;
}
