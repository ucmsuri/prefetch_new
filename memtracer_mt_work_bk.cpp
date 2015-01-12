
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "pin.H"
#include "portability.H"

PIN_LOCK lock;

/*
 * Thread specific data
 */
TLS_KEY mlog_key;

// function to access thread-specific data

INT32 numThreads = 0;
const INT32 MaxNumThreads = 4;

//static UINT32 icount = 0;
//UINT32 icount_lastmem;
UINT32 sincelast=0;
UINT32 memrefs; // number of memory accesses so far
#define MAX_REFS 200000000 // we want to limit the size of the log files so we will limit trace files to 2M lines

/*
 * Name of the output file
 */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "buffer.out", "output file");


class MLOG
{
  public:
    MLOG(THREADID tid);
    ~MLOG();
    UINT64 _icount;
    UINT64 _icount_lastmem;
    UINT64 _total;
   // VOID DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid );
 // private:
    //ofstream _ofile;
    FILE* _ofile;
  //  FILE * trace[2];
};


MLOG::MLOG(THREADID tid)
{
_icount=0;        
_icount_lastmem=0;
_total=0; 
 string filename = "xz -2 -c > " + KnobOutputFile.Value() +  "." + decstr(tid);
    _ofile = popen(filename.c_str(),"w");
    if ( ! _ofile )
    {
        cerr << "Error: could not open output file." << endl;
        exit(1);
    }
}

MLOG::~MLOG()
{
  //  _ofile.close();
}

MLOG* get_tls(THREADID tid)
{
MLOG * mlog = static_cast<MLOG*>( PIN_GetThreadData( mlog_key, tid ) );
    return mlog;
}

// This function is called before every instruction is executed
//VOID docount() { icount++; }
VOID docount(THREADID tid) { 
MLOG* mlog = get_tls(tid);
mlog->_icount++; 
}
//FILE * trace;
// This function is called before every instruction is executed
//VOID docount() { icount++; }

VOID RecordMemRead(VOID * ip, VOID * addr, THREADID tid)
{
	MLOG* mlog = get_tls(tid);
	int _sincelast = mlog->_icount - mlog->_icount_lastmem - 1;
	mlog->_icount_lastmem = mlog->_icount;
		//mlog->_total+=mlog->_icount - mlog->_icount_lastmem;
	//assert (mlog->_total==mlog->_icount);
        fprintf(mlog->_ofile,"l %p %p %u\n", ip, addr, _sincelast);
	//sincelast=0;	

   // 	fclose(trace);
	
	
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, THREADID tid)
{
	MLOG* mlog = get_tls(tid);
	int _sincelast = mlog->_icount - mlog->_icount_lastmem - 1;
	mlog->_icount_lastmem = mlog->_icount;
    	fprintf(mlog->_ofile,"s %p %p %u\n", ip, addr, _sincelast);
	//sincelast=0;	

    	//fclose(trace);
	
}

// update lastmem info and write to file (load)
/*VOID RecordMemRead(VOID * ip, VOID * addr, THREADID tid)
{
	MLOG* mlog = get_tls(tid);
	if(mlog->_icount == mlog->_icount_lastmem) 
	{
	sincelast = 0;
	mlog->_total+=sincelast;
	}
	else {
		sincelast = mlog->_icount - mlog->_icount_lastmem - 1;
		mlog->_icount_lastmem = mlog->_icount;
		//mlog->_total+=mlog->_icount - mlog->_icount_lastmem;
		mlog->_total+=sincelast+1;;
	}
//	cout << "TID : " << tid << " READ " << mlog->_icount << "  " << mlog->_total << endl;
	assert (mlog->_total==mlog->_icount);
    fprintf(mlog->_ofile,"l %p %p %u\n", ip, addr, sincelast);
	sincelast=0;	

   // 	fclose(trace);
	
	
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, THREADID tid)
{
	MLOG* mlog = get_tls(tid);
	if(mlog->_icount == mlog->_icount_lastmem)
	{
	sincelast = 0;
	mlog->_total+=sincelast;
	}
	else {
		sincelast = mlog->_icount - mlog->_icount_lastmem - 1;
		mlog->_icount_lastmem = mlog->_icount;
		//mlog->_total+=mlog->_icount - mlog->_icount_lastmem;
		mlog->_total+=sincelast+1;;
	}
//	cout << "TID : " << tid << " WRITE " << mlog->_icount << "  " << mlog->_total << endl;
	assert (mlog->_total==mlog->_icount);
    	fprintf(mlog->_ofile,"s %p %p %u\n", ip, addr, sincelast);
	sincelast=0;	

    	//fclose(trace);
	
}
*/


// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
  // Insert a call to docount before every instruction, no arguments are passed
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,IARG_THREAD_ID, IARG_END);

		// instruments loads using a predicated call, i.e.
		// the call happens iff the load will be actually executed
		// (this does not matter for ia32 but arm and ipf have predicated instructions)
		if (INS_IsMemoryRead(ins))
		{
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR,
				IARG_MEMORYREAD_EA,
				IARG_THREAD_ID,
				IARG_END);
		}

		// instruments stores using a predicated call, i.e.
		// the call happens iff the store will be actually executed
		if (INS_IsMemoryWrite(ins))
		{
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR,
				IARG_MEMORYWRITE_EA,
				IARG_THREAD_ID,
				IARG_END);
		}
}


/*
 * Note that opening a file in a callback is only supported on Linux systems.
 * See buffer-win.cpp for how to work around this issue on Windows.
 */
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // There is a new MLOG for every thread.  Opens the output file.
    MLOG * mlog = new MLOG(tid);

    // A thread will need to look up its MLOG, so save pointer in TLS
    PIN_SetThreadData(mlog_key, mlog, tid);

}


VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    MLOG * mlog = static_cast<MLOG*>(PIN_GetThreadData(mlog_key, tid));

    cout << "Count[" << decstr(tid) << "]= " << mlog->_icount << endl;
    cout << "Total count[" << decstr(tid) << "]= " << mlog->_total << endl;
    delete mlog;

    PIN_SetThreadData(mlog_key, 0, tid);
}

/*VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
}*/




/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}




/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    InitLock(&lock);
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    //OutFile.open(KnobOutputFile.Value().c_str());
    mlog_key = PIN_CreateThreadDataKey(0);
    //trace = fopen("mem.trace", "w");

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
 //   PIN_AddFiniFunction(Fini, 0);
    
 // add callbacks
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
