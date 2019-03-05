#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "myglobal.h"
/***********************************************************************************
This framemanagement cares the frame fault to set the virtual memory to the physical
memory.
***********************************************************************************/
int physicpage=0;
PCBPointer getpcb(int);
int getfreepage();
pagetab pagetabel[64];
pagetab swaparea[1024];
byte datame[13][1024][16];
int swapn=0;
int entries=0;
int findswapn(int ,int );
   INT32 LockResult;
// Fault handler, given virtual memory and pid, fix the memory problem.
void faulthandler(int virtualloc,int pid)
{   
    if(findswapn(virtualloc,pid)!=-1) // check if the vpn is in the swaparea
	{  
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	  int swapnumber=findswapn(virtualloc,pid);
	  int phl=getLRUpage();//swaparea[swapnumber]->physicloc; get victim by LRU
	  byte readb[16];
	  Z502ReadPhysicalMemory(phl, readb); //read out the victim
	  invalidvpage(pagetabel[phl]->pid,pagetabel[phl]->virloc);  //set that page invalid
	  pagetabel[phl]->swapFlag=1;
      //if the victim is not in the swaparea, then add it to the swaoarea
      //firstly, we save the page information. 	  
	  if(findswapn(pagetabel[phl]->virloc,pagetabel[phl]->pid)==-1)
	  {
		int tempswap=swapn;
		swaparea[tempswap]=(pagetab)malloc(sizeof(pages));
	    swaparea[tempswap]->physicloc=phl;
	    swaparea[tempswap]->virloc=pagetabel[phl]->virloc;
	    swaparea[tempswap]->page=pagetabel[phl]->page;
	    swaparea[tempswap]->pid=pagetabel[phl]->pid;
		swaparea[tempswap]->swapFlag=1; 
		swapn++;
	  }
	  //read back the swaparea data
	  byte readd[16];
	  READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	  //readinDisk(readd,swaparea[swapnumber]->pid,SwapLoc+ swaparea[swapnumber]->virloc);
	  READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	  for(int i=0;i<16;i++)
		  readd[i]=datame[swaparea[swapnumber]->pid][swaparea[swapnumber]->virloc][i];
	  Z502WritePhysicalMemory(phl,readd);
      // write the victim in the swaparea 	  
	  for(int i=0;i<16;i++)
		  datame[pagetabel[phl]->pid][pagetabel[phl]->virloc][i]=readb[i];
	  READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	 // writeinDisk(readb,pagetabel[phl]->pid,SwapLoc+ pagetabel[phl]->virloc);
	  READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	  setpagetable(pid,virtualloc,phl);  //give the phisical memory the vpn
	  //set pagetabel
	  pagetabel[phl]->physicloc=phl;
	  pagetabel[phl]->virloc=virtualloc;
	  pagetabel[phl]->pid=pid;
	  READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	}
	else    //We didn't find it in the swaparea.
    {int phispage=getfreepage(); //find the next available phisical frame
	if(phispage==-1)             //Find we have no available phisical frame, so we use LRU algorithm to find the victim.
	{    READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
        int temps=swapn;
		phispage=getLRUpage();  //find the victim
		invalidvpage(pagetabel[phispage]->pid,pagetabel[phispage]->virloc); //invalid the origin vpn on that page
		//save that virtual in the swaparea.
		pagetabel[phispage]->swapFlag=1;
		swaparea[temps]=(pagetab)malloc(sizeof(pages));
	    swaparea[temps]->physicloc=phispage;
	    swaparea[temps]->virloc=pagetabel[phispage]->virloc;
	    swaparea[temps]->page=pagetabel[phispage]->page;
	    swaparea[temps]->pid=pagetabel[phispage]->pid;
		swaparea[temps]->swapFlag=1;
		swapn++;
		byte readb[16];
		//modify the page information 
		setpagetable(pid,virtualloc,phispage);
	    pagetabel[phispage]->physicloc=phispage;
     	pagetabel[phispage]->virloc=virtualloc;
	    pagetabel[phispage]->page=getpage(pid);
	    pagetabel[phispage]->pid=pid;
     	pagetabel[phispage]->swapFlag=0;
		Z502ReadPhysicalMemory(phispage, readb);
		//save the memory data
		for(int i=0;i<16;i++)
		datame[swaparea[temps]->pid][swaparea[swapn-1]->virloc][i]=readb[i];
	 READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		//writeinDisk(readb, swaparea[temps]->pid,SwapLoc + swaparea[swapn-1]->virloc);
	}
	else //found phisical memory available
	{ READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		pagetabel[phispage]=(pagetab)malloc(sizeof(pages));
    //aprintf("Use %d physical page!\n",phispage);
	//set the page information
	setpagetable(pid,virtualloc,phispage);
	pagetabel[phispage]->physicloc=phispage;
	pagetabel[phispage]->virloc=virtualloc;
	pagetabel[phispage]->page=getpage(pid);
	pagetabel[phispage]->pid=pid;
	pagetabel[phispage]->swapFlag=0;
	 READ_MODIFY(MEMORY_INTERLOCK_BASE + 24, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);}
	}
	printmp();
	/*for(int i=0;i<swapn;i++)
	{
		aprintf("physic:%d ,virtual: %d, pid: %d\n",swaparea[i]->physicloc,swaparea[i]->virloc,swaparea[i]->pid);
	}
	aprintf("\n");*/
}
//find a free phisical page
int getfreepage()
{
	int res=physicpage;
	 if(res>63)
		return -1; 
	physicpage++;
	return res;
	
}
//find a victim page by LRU algorithm
int getLRUpage()
{   
    int victimp=-1;  // set default victim number as an invalid number 
	while(victimp==-1)
	{int i=0;
	for(;i<64;i++)//loop until we find a victim
	{
        short *tempPage = pagetabel[i]->page; //get the pagetable of the physiacal page
        int tempvloc = pagetabel[i]->virloc;  // get the vpn of that phisical location
        if ((tempPage[tempvloc] & (UINT16)PTBL_REFERENCED_BIT) == 0){  //if it is not refrenced, the use this victim
			victimp=i;
            break;
        } else {
            tempPage[tempvloc] = tempPage[tempvloc] - (UINT16)PTBL_REFERENCED_BIT; // if it is not refrenced, set the refrenced bit as 0.
        }
		//aprintf("%d",i);
	}
	//aprintf("\nvictim is %d\n",victimp);
	}
	
	return victimp;
}
//given vpn and pid, find the swap information table.
int findswapn(int virtualloc,int pid)
{   int res=-1;
	for(int i=0;i<swapn;i++)
	{
		if(swaparea[i]->pid==pid&&swaparea[i]->virloc==virtualloc)
		{res= i;
		break;
		}
	}
	return res;
}
//given vpn, find the physical loc.
int searchvloc(int query)
{
	for(int i=0;i<physicpage;i++)
	{
		if(pagetabel[i]->virloc==query)
		{
			return i;
		}
	}
	return -1;
}

void initdatame()
{
		for(int i=0;i<10;i++)
	     for(int j=0;j<1024;j++)
			 for(int k=0;k<16;k++)
				datame[i][j][k]=0; 
}

void writeswaparea(int pid)
{  for(int j=0;j<1024;j++)
	{   byte temp[16];
        for(int k=0;k<16;k++) 
        {
		temp[k]=datame[pid][j][k];
		//aprintf("%2x ",temp[k]);
		}
		//aprintf("\n");
		writeinDisk(temp,1,j+SwapLoc);
	}
}

/*
Print out the usage of physical memory. 
*/
void printmp()
{//aprintf("here");
	MP_INPUT_DATA input;
	for(int i=0;i<64;i++)
	{
		if(i<physicpage)  //valid physical information
		{
		 input.frames[i].InUse=TRUE;
	     input.frames[i].Pid=pagetabel[i]->pid;
		 input.frames[i].LogicalPage=pagetabel[i]->virloc;
	     input.frames[i].State=PTBL_VALID_BIT;
	    }
		else    // invalid ones
		{
		 input.frames[i].InUse=FALSE;
	     input.frames[i].Pid=-1;
		 input.frames[i].LogicalPage=-1;
	     input.frames[i].State=0;
		}
}
if(getpcb(0)->startaddress==(long)test44||getpcb(0)->startaddress==(long)test45||getpcb(0)->startaddress==(long)test46)	//set limited times
{
	if(entries<50)
	{MPPrintLine(&input);
   entries++;
  }
  }
if(getpcb(0)->startaddress==(long)test41||getpcb(0)->startaddress==(long)test42||getpcb(0)->startaddress==(long)test43) 
	MPPrintLine(&input);
}

