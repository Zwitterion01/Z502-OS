#define NEWPCB 0
#define RUNPCB 1
#define TERMPCB 2
#define DELAYPCB 3
#define SUSPENDPCB 4
#define WRITINGPCB 5
#define READINGPCB 6
#define         DO_LOCK                     1
#define         DO_UNLOCK                   0
#define         SUSPEND_UNTIL_LOCKED        TRUE
#define         DO_NOT_SUSPEND              FALSE
#define Disk_len   8
#define Swap_size  1024
#define Root_size  2
#define Bitmap_size  3
#define RootheaderLoc 1
#define SwapLoc 15
#define MSBMASK  65280
#define LSBMASK  255
typedef unsigned char byte;
typedef struct{
	int PID;
	char* processname;
	long startaddress;
	long contextid;
	int state; // 0 for a new pcb, 1 for running pcb, 2 for teminated pcb, 3 for sleep pcb,4 for suspend pcb.
	long waketime;
	int priority;
	long diskID;
	short *pageTable;
	byte dirheader[16];
	byte dirIndexsec[16];
	byte fileheader[16];
	byte fileIndexsec[16];
}PCB;
typedef struct{
	long diskID;
	long sectorID;
}diskF;
typedef struct{
	char name[7];
	long headerloc;
	int ford;
	long time;
	long size;
}DIR;
typedef struct{
	int physicloc;
	int virloc;
	short *page;
	int pid;
	int swapFlag;
}pages;
typedef pages *pagetab;
typedef DIR *namedir;
typedef diskF *diskForm;
typedef PCB *PCBPointer;
