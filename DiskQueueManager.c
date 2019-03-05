/************************************************************************
DISK QUEUE MANAGER:
CONTAIN 8 QUEUES FOR 8 DIFFERENT DISK, EACH HAVE THESE FUNCTIONS.
LOCK FOR DISK IS BASED ON THE DISK ID MEMORY_INTERLOCK_BASE + 14 +
DISKID.
 ************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h>
#include    <limits.h>
#include    "global.h"
#include    "protos.h"
#include    "myglobal.h"
int disknum=8;
int diskqn[8]={0,0,0,0,0,0,0,0};
void CreateDiskQueue()
{
	QCreate("Disk_Queue0");
	QCreate("Disk_Queue1");
	QCreate("Disk_Queue2");
	QCreate("Disk_Queue3");
	QCreate("Disk_Queue4");
	QCreate("Disk_Queue5");
	QCreate("Disk_Queue6");
	QCreate("Disk_Queue7");
}
int DiskqueeisEmpty(int diskid)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14+diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	if(QNextItemInfo(2+diskid)==(void *)-1)  //check the q is empty, if so, return 1.
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return 1;}
	else 
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return -1;}
	
}
void DiskQueueInsert(int order, PCBPointer p,int diskid)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 14 + diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		QInsert(2+diskid,order,p);
		diskqn[diskid]++;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 14 + diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void *DiskDeQueue(int diskid)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14+diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			void *ReturnPointer=QRemoveHead(2+diskid);
			diskqn[diskid]--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *DiskInfo(int diskid)
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14+diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QNextItemInfo(2+diskid);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *DiskExist(PCBPointer p,int diskid)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14+diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QItemExists(2+diskid,p);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *DiskRemove(PCBPointer p,int diskid)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14+diskid, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QRemoveItem(2+diskid,p);
		diskqn[diskid]--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 14+diskid, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
int getDiskQN()
{INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+14, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	int r=diskqn[0]+diskqn[1]+diskqn[2]+diskqn[3]+diskqn[4]+diskqn[5]+diskqn[6]+diskqn[7];
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 14, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return r;
}