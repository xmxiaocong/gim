#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cassert>
#include "base/ef_loop_buf.h"
#include "net/ef_sock.h"

using namespace ef;


int main(int argc, const char** argv){
	
#define BUF_SIZE 5000
	LoopBuf b;
	for(int i = 0; i < 10; ++i){
		uint8 buf[BUF_SIZE];
		b.autoResizeWrite(buf, sizeof(buf));
		uint8 rcvbuf[BUF_SIZE];
		int ret = b.read(rcvbuf, sizeof(rcvbuf));
		assert(ret == BUF_SIZE);
		assert(memcmp(buf, rcvbuf, BUF_SIZE) == 0);
		uint8 buf1[BUF_SIZE * 2];
		b.autoResizeWrite(buf1, sizeof(buf1));
		uint8 rcvbuf1[BUF_SIZE * 2];
		int ret1 = b.read(rcvbuf, sizeof(rcvbuf1));
		assert(ret1 == BUF_SIZE * 2);
		assert(memcmp(buf1, rcvbuf1, BUF_SIZE * 2) == 0);
	}

	const int sz = 16;
	in_addr_t addrs[sz];
	int ret = getLocalhostIps(addrs, sz);
	
	struct in_addr addr;
	for(int j = 0; j < ret; ++j){
		addr.s_addr = addrs[j];
		std::cout << "ip[" << j << "]:"
			<< inet_ntoa(addr) << std::endl;
	}

	std::cout << "test success!\n";
	
	return 0;
}
