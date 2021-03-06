/*
 *
 * File: main.C
 * Author: Saturnino Garcia (sat@cs)
 * Description: Cache simulator for 2 level cache
 *
 */
//#define PREFETCH_H "next_line_prefetch.h"

#include "cache.h"
#include "CPU.h"
#include "mem-sim.h"
#include "memQueue.h"
#include <stdlib.h>
#include <string.h>
#include "prefetcher.h"
#include <unordered_map>
//#include <algorithm.h>

int main(int argc, char* argv[]) {
	if(argc != 4) {
		printf("Usage: %s [trace file]\n",argv[0]);
		return 1;
	}

	int hitTimeL1 = 1; 
	int accessTimeL2 = atoi(argv[2]); 
	int accessTimeMem = atoi(argv[3]); 
//	int accessTimeL2 = 20; 
//	int accessTimeMem = 50; 
	printf("%d, %d\n",accessTimeL2,accessTimeMem);	
	int lineSizeL1 = 64;
	int assocL1 = 4; 
	int totalSizeL1 = 64; 

	int lineSizeL2 = 64; 
	int assocL2 = 8; 
	int totalSizeL2 = 256; 

	int numSetsL1, numSetsL2;


	u_int32_t addr, cycles;
	u_int32_t runtime = 0; // number of cycles in runtime
	u_int32_t nonmemrt = 0; // number of cycles in runtime spent with non-mem instructions

	FILE *fp; // to write out the final stats

	// calc number of sets (if assoc == 0 then it's fully assoc so there is only 1 set)
	if(assocL1 != 0) numSetsL1 = totalSizeL1 * 1024 / (assocL1 * lineSizeL1);
	else {
		numSetsL1 = 1;
		assocL1 = totalSizeL1 * 1024 / lineSizeL1;
	}

	if(assocL2 != 0) numSetsL2 = totalSizeL2 * 1024 / (assocL2 * lineSizeL2);
	else {
		numSetsL2 = 1;
		assocL2 = totalSizeL2 * 1024 / lineSizeL2;
	}

	// D-cache is write through with no-write-alloc, LRU replacement
	Cache DCache(numSetsL1,assocL1,lineSizeL1,false,false,true);

	// L2 cache is writeback with write-alloc, LRU replacement
	Cache L2Cache(numSetsL2,assocL2,lineSizeL2,false,true,false);

	CPU cpu(argv[1]);

	Prefetcher pf;

	memQueue writeBuffer(2,&DCache,accessTimeL2,true,true,'a');
	memQueue queueL2(8,&DCache,accessTimeL2,true,false,'b');
	memQueue queueMem(8,&L2Cache,accessTimeMem,false,false,'c');


	// statistical stuff
	u_int64_t nRequestsL2 = 0; // number of requests sent out to L2 (both CPU and prefetcher requests)
	u_int64_t nRequestsL2Load = 0; // number of requests sent out to L2 (both CPU and prefetcher requests)
	u_int64_t nRequestsL2Store = 0; // number of requests sent out to L2 (both CPU and prefetcher requests)
	u_int64_t memCycles = 0; // number of cycles that main memory is being accessed
	u_int64_t memQsize = 0; // used for calculating average queue length

	u_int64_t curr_cycle = 1;
	Request req;
	bool isHit;
 //mine
	//std::unordered_map<u_int32_t,bool> pref_table;
        bool is_pre; //checking whether this entry is due to prefetcher
 	uint L2_req_from_pref=0;	
 	uint L2_req_from_pref_is_hit=0;	
 	uint L2_req_from_pref_is_miss=0;	
	uint L1_miss=0;
	uint L1_pf_used=0;
	uint L2_miss=0;
	uint L2_pf_used=0;
	uint pref_req=0;
	uint pref_complete=0;
	uint pref_dropped=0;
	uint L1_hit=0;
        uint total_L2_req=0;
	uint mem_req_from_L2=0;
	uint L2_hit=0;
	uint L2_acc=0;
	uint L2_write_hit=0;
	uint l2qtomemq=0;
	uint waste_req=0;	
//int pref_hit=0;
	//u_int32_t temp_addr=0;
	//u_int32_t cpu_addr=0;

	while(!cpu.isDone()) {

		isHit = false;
//mine
		is_pre=false;
		cpuState cpu_status = cpu.getStatus(curr_cycle);

//		printf("%u: %u\n",curr_cycle,cpu_status);

		if(cpu_status == READY) { // request is ready
				
			req = cpu.issueRequest(curr_cycle);

			// check for L1 hit
			isHit = DCache.check(req.addr,req.load);
			if(isHit){L1_hit++;}
	//mine
			is_pre= DCache.is_prefetch(req.addr);
			if(is_pre){L1_pf_used++;}
	
			cpu.hitL1(isHit);
			req.HitL1 = isHit;
			
			//mine
			if (isHit==0){L1_miss++;}
//mine insert the pref.request in pref_table
	/*		cpu_addr=req.addr >>4;
			std::unordered_map<u_int32_t,bool>::const_iterator got = pref_table.find(cpu_addr);
                        if(got != pref_table.end()){pref_hit++;}
			
			L1_miss++;
			}*/

			// notify the prefetcher of what just happened with this memory op
			pf.cpuRequest(req);

			if(isHit) {
				DCache.access(req.addr,req.load, req.fromCPU);
				cpu.completeRequest(curr_cycle);
			}
			else if(req.load) {
				nRequestsL2++;
				nRequestsL2Load++;
				if(queueL2.add(req,curr_cycle)) 
					cpu.setStatus(WAITING); // CPU is now "waiting" for response from L2/mem
				else 
					cpu.setStatus(STALLED_L2); // no room in l2 queue so we are "stalled" on this request
			}
			else { 
				nRequestsL2++;
				nRequestsL2Store++;

				if (writeBuffer.add(req,curr_cycle)) cpu.completeRequest(curr_cycle); 
				else { // need to stall for an entry in the write buffer to open up
					cpu.setStatus(STALLED_WB);
				}
			}
		}
		// PF can do some work if we are just waiting or idle OR if we had a hit in the D-cache so the D-to-L2 bus isn't needed
		else if(cpu_status == WAITING || cpu_status == IDLE || cpu_status == STALLED_WB || isHit) { // either waiting for lower mem levels or idle so PF can do something
			if(pf.hasRequest(curr_cycle)) { 
//mine				
				pref_req++;
				nRequestsL2++;

				req = pf.getRequest(curr_cycle);
//mine insert the pref.request in pref_table
			//	temp_addr= req.addr >> 4;	
			//	pref_table.insert(std::pair<u_int32_t,bool>(temp_addr,true));
				req.fromCPU = false;
				req.load = true;
				if(queueL2.add(req,curr_cycle)) {
					pf.completeRequest(curr_cycle); // if added to queue then the request is "complete"
					pref_complete++;
				} else {
					pref_dropped++;
				}
					 
			}

			if(cpu_status == STALLED_WB) { // attempt to put it in the write buffer
				req = cpu.getRequest(); // get the request we want

				if (writeBuffer.add(req,curr_cycle)) cpu.completeRequest(curr_cycle); // if added, we can move on
			}
		}
		else if(cpu_status == STALLED_L2) { // stalled b/c of L2 queue so let us just try this right away
			req = cpu.getRequest();
			if(queueL2.add(req,curr_cycle)) cpu.setStatus(WAITING); // l2 queue is free now so we can go into waiting state
		}

		// service the L2 queue
		if(queueL2.frontReady(curr_cycle)) { // check to see if the front element in the queue is ready
			//printf("servicing the l2 queue on cycle %u\n",curr_cycle);
			req = queueL2.getFront();
			isHit = L2Cache.check(req.addr,req.load);
                        L2_acc++;
			cpu.loadHitL2(isHit);
			is_pre= L2Cache.is_prefetch(req.addr);
//mine stats
//		if(!DCache.check(req.addr,req.load)) //mine: If the current request is not in L1 cache, send current request to L2
//		{
		//	printf("L1 MISS SO L2 REQ\n"); 
			//total_L2_req++;			
			//if (req.fromCPU==false){L2_req_from_pref++;}

	//mine
			//is_pre= L2Cache.is_prefetch(req.addr);
			//if(is_pre){L2_pf_used++;}
	//mine
			//if (isHit){L2_hit++;}
			//if (isHit==0){L2_miss++;}
//		}
		//	printf("REMOVING WASTE REQ\n"); 
			if(isHit) {
			total_L2_req++;
			L2_hit++;			
			if(is_pre && req.fromCPU==true){L2_pf_used++;}

			if ( req.fromCPU==false){L2_req_from_pref++;}
			if (is_pre && req.fromCPU==false){L2_req_from_pref_is_hit++;}

				DCache.access(req.addr,req.load,req.fromCPU); // update D cache
				if(req.fromCPU) cpu.completeRequest(curr_cycle); // this request was from the CPU so update state to show we are done
				queueL2.remove(); // remove this request from the queue
                                //here
			}
			else {
				if(queueMem.add(req,curr_cycle)) {
			total_L2_req++;			
			L2_miss++;
			if (req.fromCPU==false){L2_req_from_pref++;L2_req_from_pref_is_miss++;}

					queueL2.remove(); // succesfully added to memory queue so we can remove it from L2 queue
        				//here
		//		queueMem.add(req,curr_cycle); //mine: If L2 Miss then just add the entry into Memqueue and don't remove the front entry from L2 Queue.
					l2qtomemq++;
			     }
			}
	
/*		else {
			printf("REMOVING WASTE REQ\n"); 
			waste_req++;
			queueL2.remove(); //remove the front end of the queue if current request is already in L1 and update it as waste request to L2
		     }*/
	}

		// service the memory queue
		if(queueMem.frontReady(curr_cycle)) {
			//printf("servicing the mem queue on cycle %u\n",curr_cycle);
			req = queueMem.getFront();
			mem_req_from_L2++;
			queueMem.remove();

			// update both L2 and D cache
			L2Cache.access(req.addr,req.load,req.fromCPU);
		if(req.load) DCache.access(req.addr,req.load,req.fromCPU); // only update if this is a load
	////MODIFIED	
		//if(req.load && req.fromCPU==true) DCache.access(req.addr,req.load,req.fromCPU); // only update if this is a load

			if(req.fromCPU && req.load) cpu.completeRequest(curr_cycle);
		}

		// check to see if we are utilizing memory BW during this cycle
		if(queueMem.getSize() > 0) memCycles++;

		// used to find the average size of the memory queue
		memQsize += queueMem.getSize();

		// service the write buffer
		if(writeBuffer.frontReady(curr_cycle)) {
			req = writeBuffer.getFront();

			isHit = L2Cache.check(req.addr,req.load);
			cpu.storeHitL2(isHit);

			if(isHit) { // store hit in L2 so just save it and we are done
				L2_write_hit++;
				L2Cache.access(req.addr,req.load,req.fromCPU);
				writeBuffer.remove();
			}
			else { // L2 is write-allocate so we need to load data from memory first
				if(queueMem.add(req,curr_cycle)) writeBuffer.remove(); // we can keep adding to the queue because we check for duplicates as part of add()
			}
		}

		curr_cycle++; // next cycle
	}

	curr_cycle--; // just for stats sake

	double avgMemQ = (double)memQsize / (double)curr_cycle;
	double L2BW = (double)nRequestsL2 / (double)curr_cycle;
	double memBW = (double)memCycles / (double)curr_cycle;

	/*
	fprintf("total run time: %u\n",curr_cycle);
	fprintf("D-cache total hit rate: %f\n",cpu.getHitRateL1());
	fprintf("L2 cache total hit rate: %f\n",cpu.getHitRateL2());
	fprintf("AMAT: %f\n",cpu.getAMAT());
	fprintf("Average Memory Queue Size: %f\n",avgMemQ);
	fprintf("L2 BW Utilization: %f\n",L2BW);
	fprintf("Memory BW Utilization: %f\n",memBW);
	*/


	// create output file name based on trace file name
/*	char* outfile = (char *)malloc(sizeof(char)*(strlen(argv[1])+5));
	strcpy(outfile,argv[1]);
	strcat(outfile,".out");

	fp = fopen(outfile,"w"); // open outfile for writing

	free(outfile);

	fprintf(fp,"%u\n",curr_cycle);
	fprintf(fp,"%.4f\n",cpu.getHitRateL1());
	fprintf(fp,"%.4f\n",cpu.getHitRateL2());
	fprintf(fp,"%.4f\n",cpu.getAMAT());
	fprintf(fp,"%.4f\n",avgMemQ);
	fprintf(fp,"%.4f\n",L2BW);
	fprintf(fp,"%.4f\n",memBW);

	fclose(fp);*/
	printf("TOTAL_ACCESS_TIME	%ld\n",cpu.Total_access_time());
	printf("TOTAL_Inst	%ld\n",cpu.Total_inst());
	printf("CPU_CYCLES	%ld\n",curr_cycle);
	printf("MEM_CYCLES	%ld\n",memCycles);
	printf("MEM_OPS 	%ld\n",cpu.getNrequest());
	printf("L1_HIT 		%ld\n",L1_hit);
	printf("L1_HIT_DUE_TO_PREFETCH 	%ld\n", L1_pf_used);
	printf("L1_MISS 	%ld\n",L1_miss);
	printf("PREFETCH_REQ 	%ld\n",pref_req);
	printf("PREFETCH_COMP 	%ld\n",pref_complete);
	printf("PREFETCH_DROP 	%ld\n",pref_dropped);
	printf("L2_REQ 		%ld\n",nRequestsL2);
	printf("L2_REQ_LD 		%ld\n",nRequestsL2Load);
	printf("L2_REQ_DT 		%ld\n",nRequestsL2Store);
	printf("L2_REQ_L2Q      %ld\n",total_L2_req);
	printf("L2_REQ_L2Q_PF   %ld \n",L2_req_from_pref);
	printf("L2_REQ_L2Q_PF_HIT   %ld \n",L2_req_from_pref_is_hit);
	printf("L2_REQ_L2Q_PF_MISS   %ld \n",L2_req_from_pref_is_miss);
//	printf("Total CPU request %d\n",pf.cpu_req);
	printf("PF_REQ_REPLACE_BY_CPU_L2Q %ld\n",queueL2.cpu_dup_replace);
	printf("PF_REQ_REPLACE_BY_CPU_MEM %ld\n",queueMem.cpu_dup_replace);
	printf("DUPLICATE_L2Q %ld\n",queueL2.duplicate_found);
	printf("DUPLICATE_MEMQ %ld\n",queueMem.duplicate_found);
	printf("DUPLICATE_L2Q_PF %ld\n",queueL2.duplicate_found_pf);
	printf("DUPLICATE_L2Q_PF_OLDPF %ld\n",queueL2.duplicate_found_pf_oldpf);
	printf("DUPLICATE_L2Q_PF_OLDDEM %ld\n",queueL2.duplicate_found_pf_olddem);
	printf("WASTE_L2_REQ_L2Q %d\n",waste_req);
	printf("L2_HIT 		%ld\n",L2_hit);
	printf("L2_MISS 	%ld\n",L2_miss);
	printf("L2HIT_DUE_TO_PREFETCH 	 %ld\n", L2_pf_used);
//	printf("L1_miss  prefetch hit %d\n",pref_hit);
	printf("MQ_L2Q		%d\n",l2qtomemq);
	printf("MEM_REQ_MEMQ    %d\n",mem_req_from_L2);
        printf("L2_WRITE_HIT    %d\n",L2_write_hit);
	printf("PREFETCH_ACCURACY	 %f\n",float(L1_pf_used)/L2_req_from_pref);
	printf("PREFETCH_COVERAGE	 %f\n",float(L1_pf_used)/(L1_miss+L1_pf_used));
	printf("PREFETCH_BW	 %f\n",memBW);
	printf("IPC 		 %f\n",float(cpu.Total_inst())/curr_cycle);
	printf("PREFETCH_ACCURACY_2	 %f\n",float(L1_pf_used)/total_L2_req);
	return 0;
}

