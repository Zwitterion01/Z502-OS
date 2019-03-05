/************************************************************************
TIMER QUEUE MANAGER:
CONTAIN ONE QUEUE, FUNCTIONS ARE USED TO DEQUEUE, REMOVE, INSERT, ETC
LOCK FOR READYQ IS BASED ON THE DISK ID MEMORY_INTERLOCK_BASE + 10.
 ************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h>
#include    <limits.h>
#include    "global.h"
#include    "protos.h"
#include    "myglobal.h"
int timerqn=0;
void CreateTimerQueue()
{
	char* Timerqueue="Timer_Queue";
	QCreate(Timerqueue);
}
int TimerqueeisEmpty()
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	if(QNextItemInfo(0)==(void *)-1)  //check the q is empty, if so, return 1.
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return 1;}
	else 
	{READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return -1;}
	
}
void TimerQueueInsert(int order, PCBPointer p)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		QInsert(0,order,p);
		if(p->state!=SUSPENDPCB)
		timerqn++;
 READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void *TimerDeQueue()
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			void *ReturnPointer=QRemoveHead(0);
			PCBPointer P=(PCBPointer)ReturnPointer;
			if(P->state!=SUSPENDPCB)
			timerqn--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
		
}
void *TimerExist(PCBPointer p)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QItemExists(0,p);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *TimerRemove(PCBPointer p)
{
	INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QRemoveItem(0,p);
		timerqn--;
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return(ReturnPointer);
}
void *TimerInfo()
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		void *ReturnPointer=QNextItemInfo(0);
		READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
		return(ReturnPointer);
}
int getTimerQN()
{INT32 LockResult; 
	READ_MODIFY(MEMORY_INTERLOCK_BASE+10, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	int r=timerqn;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 10, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
			return r;
}