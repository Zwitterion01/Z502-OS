/************************************************************************
READY QUEUE MANAGER:
CONTAIN ONE QUEUE, FUNCTIONS ARE USED TO DEQUEUE, REMOVE, INSERT, ETC
LOCK FOR READYQ IS BASED ON THE DISK ID MEMORY_INTERLOCK_BASE + 11.
 ************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h>
#include    <limits.h>
#include    "global.h"
#include    "protos.h"
#include    "myglobal.h"
int readyqn=0;
void CreateReadyQueue()
{
	char* Ready_Queue="Ready_Queue";
	QCreate(Ready_Queue);
}
int ReadyqueeisEmpty()
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	if(QNextItemInfo(1)==(void *)-1)  //check the q is empty, if so, return 1.
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return 1;}
	else 
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return -1;}
	
}
void ReadyQueueInsert(int order, PCBPointer p)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		QInsert(1,order,p);
		readyqn++;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void *ReadyDeQueue()
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			void *ReturnPointer=QRemoveHead(1);
			readyqn--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *ReadyInfo()
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QNextItemInfo(1);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *ReadyExist(PCBPointer p)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QItemExists(1,p);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *ReadyRemove(PCBPointer p)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QRemoveItem(1,p);
		readyqn--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
int getReadyQN()
{INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+11, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	int r=readyqn;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 11, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return r;
}