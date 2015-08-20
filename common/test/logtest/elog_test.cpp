#include "log_init.h"
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include "base/ef_base64.h"
#include "base/ef_utility.h"

bool g_run = true;

using namespace ef;
using namespace gim;

int loop = 0;

void* test_thread(void* p){
       
   std::string s;

   int cnt = 0;
   while(cnt < loop){
       char c = 'c';
       int i = 10;
       long l = 1234567890;
       long long ll = 1234567890123456;
       double d = 12345.6789;
       void* p = &d;
    
       logDebug("file_logger") << "file test line:"
            << "c:" << c << ",i:" << i << ",l:" << l << ",ll:" << ll
            << ",d" << d << ",p:" << p << ",s:" << s;
	++cnt;

   }
   return 0; 
}

int main(int argc, const char** argv){
   if(argc < 3){
      std::cout << "elog_test <threadcnt> <loop>\n";
      exit(0);
   }
   int pcnt = atoi(argv[1]);
   loop = atoi(argv[2]);
   logInit("log.conf");
               
   std::string s1 = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
       

   logDebug("console_logger") << "sleep_ms(" << 3600 << ")";
   sleep_ms(3600);
   logDebug("console_logger") << "sleep_ms(" << 3600 << ") finish";

   pthread_t* pids = new pthread_t[pcnt];
   for(int i = 0; i < pcnt; ++i){
       pthread_create(&pids[i], NULL, test_thread, NULL); 
   }   
   
   g_run = false;
   for(int j = 0; j < pcnt; ++j){
       void* rt;
       pthread_join(pids[j], &rt);
   }   

   return 0;   
}
