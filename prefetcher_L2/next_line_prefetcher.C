/*
 *
 * File: prefetcher.C
 * Author: Sat Garcia (sat@cs)
 * Description: This simple prefetcher waits until there is a D-cache miss then 
 * requests location (addr + 16), where addr is the address that just missed 
 * in the cache.
 *
 */

#include "prefetcher.h"
#include <stdio.h>
#include <iostream>

Prefetcher::Prefetcher() { _ready = false;}//pref_req=0; cpu_req=0;}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
//if(_ready)
//{
//printf("here %d\n",pref_req);
//pref_req++;
//}
return _ready; 
}

Request Prefetcher::getRequest(u_int32_t cycle) { 
return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) {// _ready = false;
    if(_req_left == 0){
    _ready = false; 
  }else{
    _req_left--;
    _nextReq.addr = _nextReq.addr + L2_BLOCK_SIZE;
  }
 }

void Prefetcher::cpuRequest(Request req) { 
//		cpu_req++;
	//if(!_ready && !req.HitL1) {
	if(!_ready && !req.HitL2) {
//		cpu_req++;

		_nextReq.addr = req.addr + L2_BLOCK_SIZE;
//std::cout << "63 demand : " << req.addr << " block : "<< (req.addr>>6) << " Prefetch: " << _nextReq.addr << " block addr : " << (_nextReq.addr>>6) << std::endl;
//std::cout << "64 demand : " << req.addr <<" block : "<< (req.addr>>6) << " Prefetch: " << _nextReq.addr+1 << " block addr : " << (_nextReq.addr+1>>6) << std::endl;
		_ready = true;
	  _req_left = NUM_REQS_PER_MISS - 1;
	}
}
