#include <unistd.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <iostream>
#include "cache.h"
#include "process_info.h"

#define CONTAINER_CAPACITY 100000

namespace DNSCache {
  struct CacheItem{
    bool DNSStatus;
    time_t UpdateTime;
  };

} // namespace DNSCache


int main(int argc, char *argv[]){

  //cache test  
  common::Cache<std::string, DNSCache::CacheItem, utility::single_thread, common::lru_cache> cacheContainer(CONTAINER_CAPACITY);
  for(int i=0; i< CONTAINER_CAPACITY; ++i){
      cacheContainer.set("parameter:" + std::to_string(i),{i%2?true:false, CONTAINER_CAPACITY+100-i});
  }
	
	DNSCache::CacheItem temp;
	
  for(int i=4; i< CONTAINER_CAPACITY; i+=3){
    bool status = cacheContainer.get("parameter:" + std::to_string(i), temp);
    //std::cout << "The size now is: " << temp.UpdateTime << std::endl;
  }


  //get process info
  if(argc > 3){
		printf("Usage:test [process_name] [user]\n");
		return 1;
	}
  unsigned int pid=0;
  if(argc == 1){
    pid = getpid();
  }
	
  if(argc == 2){
		pid = get_pid(argv[1]);
	}

	if(argc == 3){
		pid = get_pid(argv[1],argv[2]);
	}
	
  printf("=====CONTAINER_CAPACITY:%d=====\n",CONTAINER_CAPACITY);
	printf("pid=%d\n",pid);
	printf("pcpu=%f\n",get_proc_cpu(pid));
	printf("procmem=%d\n",get_proc_mem(pid));
	printf("virtualmem=%d\n",get_proc_virtualmem(pid));	

  //sleep(100);
}