/************************************************************************
PCB MANAGER
CONTAINS TOTAL PCBS IN AN ARRAY. FUNCTIONS IS USED TO CREAT PCB, GET PCB,
CHANGE OR GET PCB ATTRIBUTE.
LOCK FOR PCB IS MEMORY_INTERLOCK_BASE + 12.
************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h>
#include    <limits.h>
#include    "global.h"
#include    "protos.h"
#include    "myglobal.h"
int pid=0;PCBPointer pcb[1000];
void OSCreateProcess(char* processnamet,long stadd,long contid, int stat)
{  pcb[pid]=(PCBPointer)malloc(sizeof(PCB));
	pcb[pid]->PID=pid;
	if(pid!=0)
	pcb[pid]->processname=processnamet;
    else
		pcb[pid]->processname = "testroutine";
	pcb[pid]->startaddress = stadd;
	pcb[pid]->contextid = contid;
	pcb[pid]->state = stat;
   // printf("Your PID is %d\n",pcb[pid]->PID);
	pid++;
	//return pcb;
}
int pcbnum()
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	int id=pid;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return id;
}
PCBPointer getpcb(int index)
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	PCBPointer pcbr=pcb[index];
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return pcbr;
}

void changePCB(int index,char* pn,int id,int sta)
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	pcb[index]->processname=pn;
	pcb[index]->PID=id;
	pcb[index]->state=sta;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);

}
int anythingelse()
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	int i=0;
	for(;i<pid;i++)
	{
		if(pcb[i]->state==TERMPCB||pcb[i]->state==SUSPENDPCB)
			continue;
		else
		{READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		return 1;}
		
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return -1;
}
void setwaketime(int index,long wake)
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	pcb[index]->waketime=wake;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void setpriority(int index,int pri)
{ INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	pcb[index]->priority=pri;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void setdisk(int index,long diskid)
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	pcb[index]->diskID = diskid;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void setdirheader(int index,byte* dirheader,byte* dirsecn)
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	for(int i=0;i<16;i++)
	{pcb[index]->dirheader[i] = dirheader[i];
	pcb[index]->dirIndexsec[i] = dirsecn[i];}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}

void setfileheader(int index,byte* fileheader,byte* filesecn)
{
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	for(int i=0;i<16;i++)
	{pcb[index]->fileheader[i] = fileheader[i];
	pcb[index]->fileIndexsec[i] = filesecn[i];}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 12, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
void setpagetable(int index,int vloc, int ploc)
{
	pcb[index]->pageTable[vloc] = (UINT16) PTBL_VALID_BIT;
    pcb[index]->pageTable[vloc] += ploc;
}
void initpage(int index,short* p)
{
	pcb[index]->pageTable=p;
}
short* getpage(int index)
{
	return pcb[index]->pageTable;
}
void invalidvpage(int index, int vloc)
{  //aprintf("\nOrigin pagetable:%d ",pcb[index]->pageTable[vloc]);
	pcb[index]->pageTable[vloc] -= (UINT16) PTBL_VALID_BIT;
	//aprintf("Success in invalid bit!%d",pcb[index]->pageTable[vloc]);
}










