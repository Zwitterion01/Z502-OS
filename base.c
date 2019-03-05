/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/
/************************************************************************
LOCK FOR RUNNING PCB IS MEMORY_INTERLOCK_BASE + 13.
 ************************************************************************/
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "myglobal.h"
//int pid=0;
//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];
void createafile(char* );
char *call_names[] = {       "MemRead  ", "MemWrite ", "ReadMod  ", "GetTime  ",
		"Sleep    ", "GetPid   ", "Create   ", "TermProc ", "Suspend  ",
		"Resume   ", "ChPrior  ", "Send     ", "Receive  ", "PhyDskRd ",
		"PhyDskWrt", "DefShArea", "Format   ", "CheckDisk", "OpenDir  ",
		"OpenFile ", "CreaDir  ", "CreaFile ", "ReadFile ", "WriteFile",
		"CloseFile", "DirContnt", "DelDirect", "DelFile  " };
void OSCreateProcess(char*,long,long,int);
void dispatcher();
void Wastetime();
int pcbnum();
int  how_many_interrupt_entrie_fault=0;
int how_many_write_s=0;
int multiflag=0;
PCBPointer getpcb(int);
void changePCB(int ,char*,int,int);
void writeinDisk(byte* ,long ,long );
long transferbyte(byte,byte );
PCBPointer Running_PCB;
INT32 LockResultR;
int suspendN=0; int termiN=0; int schedulepr=0; int scheduleflag=0;
/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the Operating System.
 NOTE WELL:  Just because the timer or the disk has interrupted, and
             therefore this code is executing, it does NOT mean the 
	     action you requested was successful.
	     For instance, if you give the timer a NEGATIVE time - it 
	     doesn't know what to do.  It can only cause an interrupt
	     here with an error.
	     If you try to read a sector from a disk but that sector
	     hasn't been written to, that disk will interrupt - the
	     data isn't valid and it's telling you it was confused.
	     YOU MUST READ THE ERROR STATUS ON THE INTERRUPT
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID;
	INT32 Status;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL  remove_this_from_your_interrupt_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0;              /** TEMP **/
 
	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;
	if (mmio.Field4 != ERR_SUCCESS) {
		aprintf( "The InterruptDevice call in the InterruptHandler has failed.\n");
		aprintf("The DeviceId and Status that were returned are not valid.\n");
	}
	// HAVE YOU CHECKED THAT THE INTERRUPTING DEVICE FINISHED WITHOUT ERROR?
   if(DeviceID== TIMER_INTERRUPT&&Status!=1)
   {   
     TimerInterruptHandler();  //Go to Timer InterruptHandler
   } 
    else   
    if((DeviceID==DISK_INTERRUPT_DISK0||DISK_INTERRUPT_DISK1||DISK_INTERRUPT_DISK2||
	   DISK_INTERRUPT_DISK3||DISK_INTERRUPT_DISK4||DISK_INTERRUPT_DISK5||
	   DISK_INTERRUPT_DISK6||DISK_INTERRUPT_DISK7)&&Status!=1)
	{     
	   PCBPointer p;
	   p=(PCBPointer)DiskDeQueue(DeviceID-5);
	   changePCB(p->PID,p->processname,p->PID,NEWPCB);
	   ReadyQueueInsert(p->priority, p);
	   printschedule(p->PID);//aprintf("disk schedule\n");
	   /*MEMORY_MAPPED_IO mmio1; 
	    mmio1.Mode = Z502ReturnValue;
		 mmio1.Field1 = mmio1.Field2 =mmio1.Field3 =0;
		 MEM_READ(Z502Clock, &mmio1);*/
	   //aprintf("Disk Dequeue Successfully : PID %d Time:%ld \n",p->PID,mmio1.Field1);
	}
	/** REMOVE THESE SIX LINES **/
	how_many_interrupt_entries++; /** TEMP **/
	if (remove_this_from_your_interrupt_code && (how_many_interrupt_entries < 50)) {
		aprintf("InterruptHandler: Found device ID %d with status %d\n",
				(int) mmio.Field1, (int) mmio.Field2);
	}
}         

  // End of InterruptHandler
/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_from_your_fault_code = TRUE; /** TEMP **/
	static INT32 how_many_fault_entries = 0; /** TEMP **/

	// Get cause of fault
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;

	INT32 Status;
	Status = mmio.Field2;
	
	// This causes a print of the first few faults - and then stops printing!
	how_many_interrupt_entrie_fault++; 
	if (remove_this_from_your_fault_code && (how_many_interrupt_entrie_fault < 50)) {
		aprintf("FaultHandler: Found device ID %d with status %d\n",
				(int) mmio.Field1, (int) mmio.Field2);
	}
	if (Status >= NUMBER_VIRTUAL_PAGES){
		mmio.Mode = Z502Action;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Halt, &mmio);
		return;
	}
	faulthandler(Status,Running_PCB->PID);
	
} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;
    INT32 Time;
	MEMORY_MAPPED_IO mmio;
	
	call_type = (short) SystemCallData->SystemCallNumber;
	if (do_print > 0) {
		aprintf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			aprintf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
					(unsigned long) SystemCallData->Argument[i],
					(unsigned long) SystemCallData->Argument[i]);
		}
		do_print--;
	}
	switch(call_type)
	{
		case SYSNUM_GET_TIME_OF_DAY:
		{mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
		MEM_READ(Z502Clock, &mmio);
		*(long *)SystemCallData->Argument[0]=mmio.Field1;
		//aprintf(" Time is %ld\n",mmio.Field1);
		break;}
		/**************************************************************************
          SVC:Terminate Process.
          Prameter:SystemCallData->Argument[0]  pid,if -1 teminate itself -2 halt the program
		  Return:  SystemCallData->Argument[1]  success:0 else -1   
          Notice: if -1 is called in SystemCallData->Argument[0], then we will first check
                  readyQ then check TimerQ, to see if something there need to be processed.		  
         **************************************************************************/
		case SYSNUM_TERMINATE_PROCESS:
		{int flag=0; int k=0;
		if(SystemCallData->Argument[0]==-1||SystemCallData->Argument[0]==-2)
		{if(SystemCallData->Argument[0]==-2)     //Halt the program.
			{mmio.Mode = Z502Action;
		    mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
			MEM_WRITE(Z502Halt, &mmio);}
		 if(SystemCallData->Argument[0]==-1)   //Terminate itself.
		 {
			  Running_PCB->state = TERMPCB;			
			  changePCB(Running_PCB->PID,"",Running_PCB->PID,TERMPCB);
			  termiN++;  
			  *(long *)SystemCallData->Argument[1]=0;
			  if(ReadyInfo()!=(void *)-1)  //Check the readyq.
			  {
                  dispatcher();
				  break;
			  }
			  else
			  {if(TimerInfo()!=(void *)-1)   //Check the Timerq.
			 {
				 mmio.Mode = Z502ReturnValue;
		         mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
		         MEM_READ(Z502Clock, &mmio);
	              PCBPointer p=(PCBPointer)TimerInfo();
				  //aprintf("Next Timer is for:%ld\n",p->waketime);
				  long waketimes =0;
				  waketimes=(long)(p->waketime) - (long)(mmio.Field1);
				  mmio.Mode = Z502Start;
                  if(waketimes<0)  // If the time expire, then set the timer to 0.
                  mmio.Field1=0; // You pick the time units
                  else  
                  mmio.Field1=waketimes;					  
	              mmio.Field2 = mmio.Field3 = 0;
	              MEM_WRITE(Z502Timer, &mmio);  
               if(multiflag==1)
		      {mmio.Mode = Z502StartContext;
	           mmio.Field1 = Running_PCB->contextid;
	           mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	           mmio.Field3 = mmio.Field4 = 0;
		       MEM_WRITE(Z502Context, &mmio);
		       }   				  
				 dispatcher();
				  break;
			}			  
			  else
			 dispatcher();   //Let the dispatcher decide which to do next.
		  }		
           
		 }
			}
		else        // other process terminated bt this one.
		{
			for(;k<pcbnum();k++)
			{PCBPointer p=getpcb(k);
				if(p->PID==(int)SystemCallData->Argument[0])
				{//printf("the pcb I want to change is %d\n",(int)SystemCallData->Argument[0]);
				 flag=1;
				 break;
				}
			}
			if(flag==1)
			{
				  changePCB(k,"",k,TERMPCB); //withdraw the processname and set the state to terminated.
				  if(ReadyExist(getpcb(k))!=(void *)-1)// check it is in the readyQ or not.
					  ReadyRemove(getpcb(k));
                  termiN++;		
                   *(long *)SystemCallData->Argument[1]=0;	
                   break;				   
			}
			else
			{
				 *(long *)SystemCallData->Argument[1]=1; 	
			}
		}
		
		break;
		}
		
		/**************************************************************************
          SVC:Get Process ID.
          Prameter:SystemCallData->Argument[0]  process name,if "", return running pid
		  Return:  SystemCallData->Argument[1]  PID  
                   SystemCallData->Argument[2]  success:0 else -1  		  
         **************************************************************************/
		case SYSNUM_GET_PROCESS_ID:
		{
				
		if(strcmp((char*)SystemCallData->Argument[0], "")==0)     // get running pcb id.
		{
			
			
			//aprintf("\n Get ID!\n");
			//aprintf("\n Get id done!!");
			int i=Running_PCB->PID;
			
			
		*(long *)SystemCallData->Argument[1]=i;
		*(long *)SystemCallData->Argument[2]=0; }
		else // get other pcb id.
		{ int k=0;int flag=0;
			for(;k< pcbnum();k++)   
			{   PCBPointer p=getpcb(k);
				if(strcmp(p->processname,(char*)(SystemCallData->Argument[0]))==0)
				{
			     *(long *)SystemCallData->Argument[1]=p->PID;
		         *(long *)SystemCallData->Argument[2]=0; 	
				 flag=1;
				 break;
				}
			}
			if(flag==0)
			{
				*(long *)SystemCallData->Argument[2]=1; 
			}
		}
		
		break;
		}
		
		/**************************************************************************
          SVC:Sleep
          Prameter:SystemCallData->Argument[0]  Sleep time.
		  Notice:We will judge the waketime to the timer already existing then process
                 as scheduling.		  
		 **************************************************************************/
		 
		case  SYSNUM_SLEEP:
		{long rwake=0;
		mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
		MEM_READ(Z502Clock, &mmio);    //get time now.
		
			
		Running_PCB->waketime = (long)mmio.Field1+(long)(SystemCallData->Argument[0]);  //set pcb status and wake time.
		Running_PCB->state = DELAYPCB;    
		rwake = Running_PCB->waketime;
		
			
		changePCB(Running_PCB->PID,Running_PCB->processname,Running_PCB->PID,DELAYPCB);
		setwaketime(Running_PCB->PID,(long)mmio.Field1+(long)(SystemCallData->Argument[0]));
		
		if(TimerInfo()!=(void*)-1)                     //judge the tiimerq is empty or not.
		{
		
		PCBPointer p;
	    p=(PCBPointer)TimerInfo();
		
		if(rwake<p->waketime)        //judge whther the waketime is smaller than the waketime expected now. 
		{
		//aprintf("Timer:new: %ld \n",rwake);
		mmio.Mode = Z502Start;
	    mmio.Field1 =(long)(SystemCallData->Argument[0]);   // You pick the time units
	    mmio.Field2 = mmio.Field3 = 0;
		TimerQueueInsert(Running_PCB->waketime, Running_PCB);
	    MEM_WRITE(Z502Timer, &mmio);      //settimer
			if(multiflag==1)
		{mmio.Mode = Z502StartContext;
	    mmio.Field1 = Running_PCB->contextid;
	    mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	    mmio.Field3 = mmio.Field4 = 0;
		MEM_WRITE(Z502Context, &mmio);
		}   
		}
        else
        {//aprintf("Timer:new: %ld  old: %ld\n",rwake,p->waketime);  //If it is bigger than the waketime expected now, just add it to the timerq, and start the shorter timer. 
		mmio.Mode = Z502Start;
		long times=(long)(p->waketime)-(long)(mmio.Field1);
		if(times<0)        // If the time expire, then set the timer to 0.
	    mmio.Field1 = 0;   // You pick the time units
	    else
			mmio.Field1=times;
		TimerQueueInsert(Running_PCB->waketime, Running_PCB);
	    mmio.Field2 = mmio.Field3 = 0;
	    MEM_WRITE(Z502Timer, &mmio); 
	if(multiflag==1)
		{mmio.Mode = Z502StartContext;
	    mmio.Field1 = Running_PCB->contextid;
	    mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	    mmio.Field3 = mmio.Field4 = 0;
		MEM_WRITE(Z502Context, &mmio);
		}   		
		}			
		}     
		else                                         //Empty timerq, just start the timer.
		{
		//aprintf("Timer:new: %ld \n",rwake);
		mmio.Mode = Z502Start;
	    mmio.Field1 = (long)(SystemCallData->Argument[0]);   // You pick the time units
	    mmio.Field2 = mmio.Field3 = 0;
		TimerQueueInsert(Running_PCB->waketime, Running_PCB);
	    MEM_WRITE(Z502Timer, &mmio);
		}  
		if(multiflag==1)
		{mmio.Mode = Z502StartContext;
	    mmio.Field1 = Running_PCB->contextid;
	    mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	    mmio.Field3 = mmio.Field4 = 0;
		MEM_WRITE(Z502Context, &mmio);
		}   
		dispatcher();
	 	break;	
		}
		/**************************************************************************
          SVC:Disk Write
          Prameter:SystemCallData->Argument[0]  DiskID
		           SystemCallData->Argument[1]  Sector
		           SystemCallData->Argument[2]  Buffer
		  Notice:We will judge whether the device is done then call the dispatcher.
		 **************************************************************************/
		case SYSNUM_PHYSICAL_DISK_WRITE:
		{   long DiskID=SystemCallData->Argument[0]; 
		 mmio.Field2 = DEVICE_IN_USE;
	        while (mmio.Field2 != DEVICE_FREE) {
	        mmio.Mode = Z502Status;
		    mmio.Field1 = DiskID;
		    mmio.Field2 = mmio.Field3 = 0;
		    MEM_READ(Z502Disk, &mmio);
			}	
			mmio.Mode = Z502DiskWrite;
	        mmio.Field1 = SystemCallData->Argument[0];
	        mmio.Field2 = SystemCallData->Argument[1];
	        mmio.Field3 = SystemCallData->Argument[2];
			
			
			Running_PCB->state=WRITINGPCB;
			
			
		    changePCB(Running_PCB->PID,Running_PCB->processname,Running_PCB->PID,WRITINGPCB);
			DiskQueueInsert(10,Running_PCB,(long)(SystemCallData->Argument[0]));
			MEM_WRITE(Z502Disk, &mmio);
	
        // The call to IDLE should indicate that the correct amount of time has occurred for the interrupt to start.
        // That DOESN'T mean the interrupt has finished.  That interrupt handling code is running on a different thread.
        // So we now wait until the interrupt has done its work and the disk is now free.
		
	      dispatcher();
			
	   		break;
		}
		/**************************************************************************
          SVC:Disk Read
          Prameter:SystemCallData->Argument[0]  DiskID
		           SystemCallData->Argument[1]  Sector
		  Return:  SystemCallData->Argument[2]  Buffer
		  Notice:We will judge whether the device is done then call the dispatcher.
		 **************************************************************************/
		case  SYSNUM_PHYSICAL_DISK_READ:
		{   long DiskID=SystemCallData->Argument[0]; 
			
	        mmio.Field2 = DEVICE_IN_USE;
	        while (mmio.Field2 != DEVICE_FREE) {
	        mmio.Mode = Z502Status;
		    mmio.Field1 = DiskID;
		    mmio.Field2 = mmio.Field3 = 0;
		    MEM_READ(Z502Disk, &mmio);
	    }
			mmio.Mode = Z502DiskRead;
	        mmio.Field1 = SystemCallData->Argument[0];
	        mmio.Field2 = SystemCallData->Argument[1];
	        mmio.Field3 = SystemCallData->Argument[2];
			
			
			Running_PCB->state=READINGPCB;
			
			
		    changePCB(Running_PCB->PID,Running_PCB->processname,Running_PCB->PID,READINGPCB);
			DiskQueueInsert(10,Running_PCB,(long)(SystemCallData->Argument[0]));
			MEM_WRITE(Z502Disk, &mmio);
		    dispatcher();
	   		break;
		}
		/**************************************************************************
          SVC:Check Disk
          Prameter:SystemCallData->Argument[0]  DiskID
		  Notice:Unfinished.
		 **************************************************************************/
		case  SYSNUM_CHECK_DISK:
		{  
		if((getpcb(0)->startaddress==(long)test45||getpcb(0)->startaddress==(long)test44||getpcb(0)->startaddress==(long)test43))
			writeswaparea(1);
		if(getpcb(0)->startaddress==(long)test46)
			writeswaparea(8);
		how_many_write_s++;
		mmio.Mode = Z502CheckDisk;
	    mmio.Field1 = SystemCallData->Argument[0];
		mmio.Field2 = 0;mmio.Field3 = 0;mmio.Field4 = 0;
		MEM_WRITE(Z502Disk, &mmio);
		//long disID=SystemCallData->Argument[0];
		*(long *)SystemCallData->Argument[1]=0; 
		break;
		}
		
		/**************************************************************************
          SVC:Create Process
          Prameter:SystemCallData->Argument[0]  Process Name
		           SystemCallData->Argument[1]  StartingAddress
				   SystemCallData->Argument[2]  Initial Priority
		  Return:  SystemCallData->Argument[3]  Pricess ID return
				   SystemCallData->Argument[4]  Error return
		  Notice:When we create pcb, we get the z502 context initialized, not started.
		         And we put it into ReadyQ.
		 **************************************************************************/
		
		case SYSNUM_CREATE_PROCESS:
		{   int k=0;int flag=0;
		    for(;k<pcbnum();k++)
			{    PCBPointer p=getpcb(k);
				if(strcmp(p->processname,(char*)SystemCallData->Argument[0])==0)
				{
					flag=1;
			     break;
			     }
			}
			if(flag==1)   //check the same processname.
			{*(long *)SystemCallData->Argument[4]=ERR_SUCCESS+1;
		}
			else 
			{if(SystemCallData->Argument[2]==-3)  //bad priority
				*(long *)SystemCallData->Argument[4]=ERR_SUCCESS+1;
			else
			{   if(pcbnum()>15)      // expire the pcb number
				{
					*(long *)SystemCallData->Argument[4]=ERR_SUCCESS+1;
			    }
				else 
				{ char* temp;temp = (char *)malloc(16);  
				sprintf(temp,"%s",(char*)SystemCallData->Argument[0]);
				mmio.Mode = Z502InitializeContext;
				void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	            mmio.Field1 = 0;
	            mmio.Field2 =  (long)SystemCallData->Argument[1];
	            mmio.Field3 =  (long)PageTable;	 mmio.Field4 = 0;	
	            MEM_WRITE(Z502Context, &mmio);   //register the context.
				OSCreateProcess(temp,SystemCallData->Argument[1],mmio.Field1,NEWPCB); //register the pcb.
		        initpage(pcbnum()-1,(short*)PageTable);
				setpriority(pcbnum()-1,(int)SystemCallData->Argument[2]);  //set the priority.
				 PCBPointer p = getpcb(pcbnum()-1);
				 ReadyQueueInsert((int)(SystemCallData->Argument[2]), p);// put it into the readyq.
				 printschedule(p->PID);//aprintf("create schedule");
			  *(long *)SystemCallData->Argument[3]=p->PID;     //return pid.
				*(long *)SystemCallData->Argument[4]=ERR_SUCCESS;
				}
				
			}}
			break;
		}
		
		/**************************************************************************
          SVC:Suspend Process
          Prameter:SystemCallData->Argument[0]  Process ID
		  Return:  SystemCallData->Argument[1]  Error return
		  Notice:We will judge the position of the PCB, in ReadyQ, we'll dequeue it
		         in the TimerQ, we'll change the state. 
		 **************************************************************************/
		
		case  SYSNUM_SUSPEND_PROCESS:
		{   int index =(int)SystemCallData->Argument[0];
			//aprintf("SUSPEND PID: %d\n",index);
			if(index==-1)   //bad pid.
			*(long *)SystemCallData->Argument[1]=ERR_SUCCESS+1;
	        int k=0;int flag=0;
		    for(;k<pcbnum();k++)
			{    PCBPointer p=getpcb(k);
				if(p->PID==index&&p->state!=SUSPENDPCB) 
				{
				 flag=1;
			     break;
			     }
			}
			if(flag==0)
				*(long *)SystemCallData->Argument[1]=ERR_SUCCESS+1; //no such pid.
			else
			{
				PCBPointer p=getpcb(k);
				if(ReadyExist(p)!=(void *)-1)// check whether it is in the readyq. 
				{ReadyRemove(p);        //move it from the readyq.
			     changePCB(index,p->processname,index,SUSPENDPCB); //set it to suspending state.
				
				 }
			    else
					if(TimerExist(p)!=(void *)-1) //check whether it is in the timerq.
				    {p=TimerRemove(p);   
				      changePCB(index,p->processname,index,SUSPENDPCB); //first dequeue from timerq, change state 
					  TimerQueueInsert(p->waketime,p);                  //then put it back.
					  
				    } 
					suspendN++;
					 printschedule(p->PID);//aprintf("suspend schedule");
				*(long *)SystemCallData->Argument[1]=ERR_SUCCESS;
				
			}
			break;
		}
		/**************************************************************************
          SVC:Resume Process
          Prameter:SystemCallData->Argument[0]  Process ID
		  Return:  SystemCallData->Argument[1]  Error return
		  Notice:We will judge the position of the PCB, in TimerQ, we'll change the state
		         else we'll put it back to ReayQ. 
		 **************************************************************************/
		case  SYSNUM_RESUME_PROCESS:
		{   int index =(int)SystemCallData->Argument[0];
			//aprintf("RESUME PID: %d\n",index);
	        int k=0;int flag=0;
		    for(;k<pcbnum();k++)
			{    PCBPointer p=getpcb(k);
				if(p->PID==index&&p->state==SUSPENDPCB)
				{
				 flag=1;
			     break;
			     }
			}
			if(flag==0)
				*(long *)SystemCallData->Argument[1]=ERR_SUCCESS+1;
			else
			{
				PCBPointer p=getpcb(k);
				if(TimerExist(p)!=(void *)-1)
				{p=TimerRemove(p);   
				      changePCB(index,p->processname,index,DELAYPCB); //first dequeue from timerq, change state 
					  TimerQueueInsert(p->waketime,p);
				 }
				else
				{changePCB(index,p->processname,index,NEWPCB);
				ReadyQueueInsert(p->priority, p);
				}
				suspendN--;	printschedule(p->PID);//aprintf("resume schedule");
				*(long *)SystemCallData->Argument[1]=ERR_SUCCESS;
			}
			break;
		}
			/**************************************************************************
          SVC:Change Priority of a Process
          Prameter:SystemCallData->Argument[0]  Process ID if -1,change running PCB
		           SystemCallData->Argument[1]  New priority
		  Return:  SystemCallData->Argument[2]  Error return
		  Notice:We will judge the position of the PCB, in TimerQ, we'll change the state
		         in ReadyQ we'll dequeue it change the priority and put it back to ReayQ. 
		 **************************************************************************/
		case SYSNUM_CHANGE_PRIORITY:
		{if((int)(SystemCallData->Argument[0])==-1)    //change the priority for the running pcb.
			{
				Running_PCB->priority=(int)(SystemCallData->Argument[1]);
				 //PCBPointer p=getpcb(Running_PCB->PID);
				 setpriority(Running_PCB->PID,(int)(SystemCallData->Argument[1]));
				 break;
			}
			 int k=0;int flag=0;
		    for(;k<pcbnum();k++)
			{    PCBPointer p=getpcb(k);
				if(p->PID==(int)SystemCallData->Argument[0]&&(int)SystemCallData->Argument[1]>=1)
				{
				 flag=1;
			     break;
			     }
			}
			if(flag==0)   // pid doesn't exist.error.
			*(long *)SystemCallData->Argument[2]=ERR_SUCCESS+1;
		else
		{ 
			PCBPointer p=getpcb(k);
		    if(ReadyExist(p)!=(void *)-1)  // in the reaadyq. dequeue, change state, put it back.
			{   
		         ReadyRemove(p);
				setpriority(k,(int)SystemCallData->Argument[1]);
				//aprintf("Now %dPCB has priority %d!\n",p->PID,p->priority);
				ReadyQueueInsert(p->priority,p);
				printschedule(p->PID);//aprintf("priority schedule");
			}
			else
				if(TimerExist(p)!=(void *)-1) // in the timerq. dequeue, change state, put it back.
				{     TimerRemove(p);
				      setpriority(k,(int)SystemCallData->Argument[1]);
					  //aprintf("Now %dPCB has priority %d!\n",p->PID,p->priority);
					  TimerQueueInsert(p->waketime,p);
				}
			*(long *)SystemCallData->Argument[2]=ERR_SUCCESS;
	    }
			
			break;
		}
		case SYSNUM_FORMAT:
		{   long DiskID=SystemCallData->Argument[0]; 
		   formatdisk(DiskID);
			 *(long *)SystemCallData->Argument[1]=ERR_SUCCESS;
			break;
		}
		case SYSNUM_OPEN_DIR:
		{ //aprintf("\n open dir done!");
		  long DiskID=SystemCallData->Argument[0]; 
		   
		             
		   if(DiskID!=-1)
		   Running_PCB->diskID=DiskID;
	       else
			  DiskID = Running_PCB->diskID;
		   
			
			setdisk(Running_PCB->PID,DiskID);
		if(strcmp((char*)SystemCallData->Argument[1],"root")==0)
		{       
		byte temp[16];byte temp2[16];
		readinDisk(&temp,DiskID,RootheaderLoc); 
		for(int i=0;i<16;i++)
			Running_PCB->dirheader[i]=temp[i];
        readinDisk(&temp2,DiskID,RootheaderLoc+1);
		for(int i=0;i<16;i++)
			Running_PCB->dirIndexsec[i]=temp2[i];
		setdirheader(Running_PCB->PID,temp,temp2);
		 *(long *)SystemCallData->Argument[2]=ERR_SUCCESS;
		}
		else
		{
			long headloc=getheaderloc((char*)SystemCallData->Argument[1]);
			
		             
		byte temp[16];byte temp2[16];
		readinDisk(&temp,DiskID,headloc); 
		for(int i=0;i<16;i++)
			Running_PCB->dirheader[i]=temp[i];
        readinDisk(&temp2,DiskID,headloc+1);
		for(int i=0;i<16;i++)
			Running_PCB->dirIndexsec[i]=temp2[i];
			setdirheader(Running_PCB->PID,temp,temp2);
		 *(long *)SystemCallData->Argument[2]=ERR_SUCCESS;
		}
		
	    
				break;
		}
		
		case SYSNUM_CREATE_DIR:
		{ 
     		mmio.Mode = Z502ReturnValue;
		    mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
		    MEM_READ(Z502Clock, &mmio);
			int timenow=(int)mmio.Field1;
			long headerloc=findnextfreesec(Running_PCB->diskID);
			byte *dirheader=setupdir((char*)SystemCallData->Argument[0], timenow,Running_PCB->diskID,(int)Running_PCB->dirheader[0]);
			long secloc=findnextfreesec(Running_PCB->diskID);
			byte *headerseccontain=Initblock();
			pushsec(Running_PCB->diskID);
			writeinDisk(dirheader,Running_PCB->diskID,headerloc);
			writeinDisk(headerseccontain,Running_PCB->diskID,secloc);
			
		    
		                
			int index=0;
			for(;index<16;index=index+2)
			 {
				 if(Running_PCB->dirIndexsec[index]==0&&Running_PCB->dirIndexsec[index+1]==0)
			        break;
		     }
			   Running_PCB->dirIndexsec[index]=findLSB(headerloc);
			   Running_PCB->dirIndexsec[index+1]=findMSB(headerloc);
		    
			 
			setdirheader(Running_PCB->PID,Running_PCB->dirheader,Running_PCB->dirIndexsec);
			long dirsecloc=transferbyte(Running_PCB->dirheader[12],Running_PCB->dirheader[13]);
			writeinDisk(Running_PCB->dirIndexsec,Running_PCB->diskID,dirsecloc);
			generatebitmap(Running_PCB->diskID);
			break;
		}
		
		case SYSNUM_CREATE_FILE:
		{
			createafile((char*)SystemCallData->Argument[0]);
			break;
		}
		case SYSNUM_OPEN_FILE:
		{
			if(getheaderloc((char*)SystemCallData->Argument[0])==-1)
			{
				createafile((char*)SystemCallData->Argument[0]);
				//*(long *)SystemCallData->Argument[1]=getInode((char*)SystemCallData->Argument[0]);
			}
			long headloc=getheaderloc((char*)SystemCallData->Argument[0]);
			
		             
		    byte temp[16];byte temp2[16];
		    readinDisk(&temp,Running_PCB->diskID,headloc); 
		    for(int i=0;i<16;i++)
			Running_PCB->fileheader[i]=temp[i];
            readinDisk(&temp2,Running_PCB->diskID,headloc+1);
		    for(int i=0;i<16;i++)
		    	Running_PCB->fileIndexsec[i]=temp2[i];
		    
			
			setfileheader(Running_PCB->PID,temp,temp2);
			*(long *)SystemCallData->Argument[1]=getInode((char*)SystemCallData->Argument[0]);
		    *(long *)SystemCallData->Argument[2]=ERR_SUCCESS;
			/*for(int i=0;i<16;i++)
		aprintf("%x ",Running_PCB->dirheader[i]);aprintf("\n");
	    for(int i=0;i<16;i++)
		aprintf("%x ",Running_PCB->dirIndexsec[i]);aprintf("\n");
	    for(int i=0;i<16;i++)
		aprintf("%x ",Running_PCB->fileheader[i]);aprintf("\n");
	    for(int i=0;i<16;i++)
		aprintf("%x ",Running_PCB->fileIndexsec[i]);aprintf("\n");*/
			break;
		}
		case SYSNUM_WRITE_FILE:
		{
		    if((int)SystemCallData->Argument[1]>7||(int)SystemCallData->Argument[1]<0)
			{
				*(long *)SystemCallData->Argument[3]=ERR_SUCCESS+1;
				break;
			}
			long blockloc=findnextfreesec(Running_PCB->diskID);	
			pushsec(Running_PCB->diskID);
			int blockindex=(int)SystemCallData->Argument[1];
			byte lsbblockloc=findLSB(blockloc);
			byte msbblockloc=findMSB(blockloc);     	 
			Running_PCB->fileIndexsec[blockindex*2]=lsbblockloc;Running_PCB->fileIndexsec[blockindex*2+1]=msbblockloc;
			setdirheader(Running_PCB->PID,Running_PCB->dirheader,Running_PCB->dirIndexsec);
			long filesecloc=transferbyte(Running_PCB->fileheader[12],Running_PCB->fileheader[13]);
			writeinDisk(Running_PCB->fileIndexsec,Running_PCB->diskID,filesecloc);

			writeinDisk(SystemCallData->Argument[2],Running_PCB->diskID,blockloc);
			filesizeincrease((int)Running_PCB->fileheader[0]);
            /*     
		    Running_PCB->fileheader[14]=findLSB(getsize((int) Running_PCB->fileheader[0]));Running_PCB->fileheader[15]=findMSB(getsize((int) Running_PCB->fileheader[0]));
			setfileheader(Running_PCB->PID,Running_PCB->fileheader,Running_PCB->fileIndexsec);
            long hloc=getheaderloc_inode((int)Running_PCB->fileheader[0]);	
			writeinDisk(Running_PCB->fileheader,Running_PCB->diskID,hloc);*/	
			generatebitmap(Running_PCB->diskID);
			break;
		}
		case SYSNUM_READ_FILE:
		{   
		     if((int)SystemCallData->Argument[1]>7||(int)SystemCallData->Argument[1]<0)
			{
				*(long *)SystemCallData->Argument[3]=ERR_SUCCESS+1;
				break;
			}
			int blockindex=(int)SystemCallData->Argument[1];
			byte lsbblockloc=Running_PCB->fileIndexsec[blockindex*2];
			byte msbblockloc=Running_PCB->fileIndexsec[blockindex*2+1];
			long datablockloc = transferbyte(lsbblockloc,msbblockloc);
			readinDisk(SystemCallData->Argument[2],Running_PCB->diskID,datablockloc);
			break;
		}
		case SYSNUM_CLOSE_FILE:
		{   
		    
		             
		    Running_PCB->fileheader[14]=findLSB(getsize((int) Running_PCB->fileheader[0]));Running_PCB->fileheader[15]=findMSB(getsize((int) Running_PCB->fileheader[0]));
			 
			
			setfileheader(Running_PCB->PID,Running_PCB->fileheader,Running_PCB->fileIndexsec);
            long hloc=getheaderloc_inode((int)Running_PCB->fileheader[0]);	
			writeinDisk(Running_PCB->fileheader,Running_PCB->diskID,hloc);	 
			generatebitmap(Running_PCB->diskID);
			*(long *)SystemCallData->Argument[1]=ERR_SUCCESS;
			break;
		}
		case SYSNUM_DIR_CONTENTS:
		{   aprintf("Inode  Filename  D/F   Creation Time  File Size\n");
		    int index=0;
			for(;index<16;index=index+2)
			 {
				 if(Running_PCB->dirIndexsec[index]==0&&Running_PCB->dirIndexsec[index+1]==0)
			        break;
				else
				{   long loc=transferbyte(Running_PCB->dirIndexsec[index],Running_PCB->dirIndexsec[index+1]);
					int inode=getInodeLoc(loc);
					char* name=getname(inode);
					int fod=getfileordir(inode);
					long time=gettime(inode);
					long size=getsize(inode);
					if(fod==0)
					aprintf("%ld      %s    F      %ld         %ld\n",inode,name,time,size);
				    else
					aprintf("%ld      %s     D      %ld         %ld\n",inode,name,time,size);
				}
		     }
			break;
		}
	    default:
		printf("ERROR! Call_type not recogniozed!\n");
	    printf("Call_type is - %i\n", call_type);
	}
	
}                                               // End of svc
void createafile(char* file)
{          
            MEMORY_MAPPED_IO mmio;
	        mmio.Mode = Z502ReturnValue;
		    mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
		    MEM_READ(Z502Clock, &mmio);
			int timenow=(int)mmio.Field1;
			long headerloc=findnextfreesec(Running_PCB->diskID);
			byte *dirheader=setupfile(file, timenow,Running_PCB->diskID,(int)Running_PCB->dirheader[0]);
			long secloc=findnextfreesec(Running_PCB->diskID);
			byte *headerseccontain=Initblock();
			pushsec(Running_PCB->diskID);
			writeinDisk(dirheader,Running_PCB->diskID,headerloc);
			writeinDisk(headerseccontain,Running_PCB->diskID,secloc);
		    
		                
			int index=0;
			for(;index<16;index=index+2)
			 {
				 if(Running_PCB->dirIndexsec[index]==0&&Running_PCB->dirIndexsec[index+1]==0)
			        break;
		     }
			   Running_PCB->dirIndexsec[index]=findLSB(headerloc);
			   Running_PCB->dirIndexsec[index+1]=findMSB(headerloc);
			long dirsecloc=transferbyte(Running_PCB->dirheader[12],Running_PCB->dirheader[13]);
			writeinDisk(Running_PCB->dirIndexsec,Running_PCB->diskID,dirsecloc);
			generatebitmap(Running_PCB->diskID);
}
long transferbyte(byte lsb,byte msb)
{
	long res=lsb+256*msb;//((lsb/10)*16+(lsb%10)*1)+((msb/10)*16+(lsb%10))*256;
	return res;
}
void writeinDisk(byte* str,long diskid,long secid)
{           MEMORY_MAPPED_IO mmio;
			mmio.Field2 = DEVICE_IN_USE;
	        while (mmio.Field2 != DEVICE_FREE) {
	        mmio.Mode = Z502Status;
		    mmio.Field1 = diskid;
		    mmio.Field2 = mmio.Field3 = 0;
		    MEM_READ(Z502Disk, &mmio);
			}	
			mmio.Mode = Z502DiskWrite;
	        mmio.Field1 =diskid;
	        mmio.Field2 = secid;
	        mmio.Field3 = str;
			
			
			Running_PCB->state=WRITINGPCB;
			
			
		    changePCB(Running_PCB->PID,Running_PCB->processname,Running_PCB->PID,WRITINGPCB);
			DiskQueueInsert(10,Running_PCB,diskid);
			MEM_WRITE(Z502Disk, &mmio);
	       dispatcher();
}
void readinDisk(byte* res,long diskid,long secid)
{         
            MEMORY_MAPPED_IO mmio;
	        mmio.Field2 = DEVICE_IN_USE;
	        while (mmio.Field2 != DEVICE_FREE) {
	        mmio.Mode = Z502Status;
		    mmio.Field1 = diskid;
		    mmio.Field2 = mmio.Field3 = 0;
		    MEM_READ(Z502Disk, &mmio);
	    }
			mmio.Mode = Z502DiskRead;
	        mmio.Field1 = diskid;
	        mmio.Field2 = secid;
	        mmio.Field3 = (long)res;
			
			
			Running_PCB->state=READINGPCB;
			
			
		    changePCB(Running_PCB->PID,Running_PCB->processname,Running_PCB->PID,READINGPCB);
			DiskQueueInsert(10,Running_PCB,diskid);
			MEM_WRITE(Z502Disk, &mmio);
		dispatcher();
}
void printschedule(int targetid)
{
	SP_INPUT_DATA *schedule;
	schedule = (SP_INPUT_DATA *)calloc(1, sizeof(SP_INPUT_DATA));
	schedule->CurrentlyRunningPID = Running_PCB->PID;
	schedule->TargetPID = targetid;
	//schedule->NumberOfRunningProcesses=1;
	schedule->NumberOfReadyProcesses=getReadyQN();
	schedule->NumberOfProcSuspendedProcesses=suspendN;
	schedule->NumberOfTimerSuspendedProcesses=getTimerQN();
	schedule->NumberOfTerminatedProcesses=termiN;
	schedule->NumberOfDiskSuspendedProcesses=getDiskQN();
	int i=0;
	int temptimeri=0;int tempsuspendi=0;int tempreadyi=0;int temptermini=0;int tempdiski=0;
	for(;i<pcbnum();i++)
	{PCBPointer p=getpcb(i);
		/*if(p->state==RUNPCB)
		{schedule->RunningProcessPIDs[0]=p->PID;
		continue;}
		else*/
			if(p->state==DELAYPCB)
			{schedule->TimerSuspendedProcessPIDs[temptimeri]=p->PID;
		     temptimeri++;
			continue;}
			else if(p->state==TERMPCB)
			{
             schedule->TerminatedProcessPIDs[temptermini]=p->PID;
			 temptermini++;
			 continue;
			}
			else if(p->state==NEWPCB)
			{
				 schedule->ReadyProcessPIDs[tempreadyi]=p->PID;
				 tempreadyi++;
				 continue;
			}
			else if(p->state==SUSPENDPCB)
			{
				 schedule->ProcSuspendedProcessPIDs[tempsuspendi]=p->PID;
				 tempsuspendi++;
				 continue;
			}
            else if(p->state==WRITINGPCB||p->state==READINGPCB)
			{
                 schedule->DiskSuspendedProcessPIDs[tempdiski]=p->PID;
            		tempdiski++;		 
				continue;
			}				
	}
    if(getpcb(0)->startaddress==(long)test23||(getpcb(0)->startaddress==(long)test24&&schedulepr<50)||(getpcb(0)->startaddress==(long)test25&&schedulepr<50))
	SPPrintLine(schedule);
	schedulepr++;
}
void TimerInterruptHandler()
{
	MEMORY_MAPPED_IO mmio1; 
       PCBPointer p;
	   p=(PCBPointer)TimerDeQueue();
	if(p->state!=SUSPENDPCB)
	   { 
	   changePCB(p->PID,p->processname,p->PID,NEWPCB);
	   ReadyQueueInsert(p->priority, p);
	   printschedule(p->PID);//aprintf("timer interrupt schedule");
	  // aprintf("Timer Dequeue Successfully : PID %d\n",p->PID);
	   }
       else
	   { if(ReadyInfo()!=(void *)-1)
		dispatcher();
         else 
         {//aprintf("Interupt with suspend!\n");
		 MEMORY_MAPPED_IO mmio1;
         PCBPointer p1;
	     p1=(PCBPointer)TimerInfo();
		 mmio1.Mode = Z502ReturnValue;
		 mmio1.Field1 = mmio1.Field2 =mmio1.Field3 =0;
		 MEM_READ(Z502Clock, &mmio1);
		 mmio1.Mode = Z502Start;
		 long stime=(long)(p1->waketime)-(long)(mmio1.Field1);
		 if(stime<0)
	     mmio1.Field1 = 0;   // You pick the time units
	     else
			 mmio1.Field1=stime;
	     mmio1.Field2 = mmio1.Field3 = 0;
		  //aprintf("Timer set Successfully : PID %d instead of %d\n",p1->PID,p->PID);
	     MEM_WRITE(Z502Timer, &mmio1);
         }	 
	   }		   
}

void WasteTime(){
//aprintf("haha");
}

void dispatcher()
{MEMORY_MAPPED_IO mmio;
 if(Running_PCB->state==RUNPCB)
 {
	 return NULL;
 }	 
  if(anythingelse()==-1)
  {
	  mmio.Mode = Z502Action;
      mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
	  MEM_WRITE(Z502Halt, &mmio);
	  
  }
  while(ReadyqueeisEmpty()==1)
  { 
	  CALL(WasteTime());
  }	 
 
  PCBPointer p;//=(PCBPointer)malloc(sizeof(PCB));
  p=(PCBPointer)(ReadyDeQueue());
  changePCB(p->PID,p->processname,p->PID,RUNPCB);
			Running_PCB=p;
	//aprintf("DISPATHCER: NOW %s IS RUNNING!\n",Running_PCB->processname);
	printschedule(p->PID);//aprintf("dispather schedule\n");
	mmio.Mode = Z502StartContext;
	mmio.Field1 = Running_PCB->contextid;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	if(multiflag==1)
		mmio.Field2 = START_NEW_CONTEXT_ONLY;
	mmio.Field3 = mmio.Field4 = 0;
	MEM_WRITE(Z502Context, &mmio);   
}

void starttest(long testid)
{       MEMORY_MAPPED_IO mmio;
        void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	    char* Timer_Queue="Timer_Queue";
		QCreate(Timer_Queue);
		char* Ready_Queue="Ready_Queue";
		QCreate(Ready_Queue);
		CreateDiskQueue();
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = testid;
		mmio.Field3 = (long) PageTable;
		MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
		OSCreateProcess("",testid,mmio.Field1,NEWPCB);
		setpriority(0,10);
		//Running_PCB=(PCBPointer)malloc(sizeof(PCB));
		Running_PCB=getpcb(0);
		Running_PCB->pageTable=(short *)PageTable;
		//ReadyQueueInsert(Running_PCB->priority, Running_PCB);
		//dispatcher();
	    mmio.Mode = Z502StartContext;
	    // Field1 contains the value of the context returned in the last call
	    // Suspends this current thread
		mmio.Field1 = Running_PCB->contextid;
	    mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		//if(multiflag==1)
		//mmio.Field2 = START_NEW_CONTEXT_ONLY;
	
	    MEM_WRITE(Z502Context, &mmio);
}


/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	INT32 i;
	MEMORY_MAPPED_IO mmio;

	// Demonstrates how calling arguments are passed thru to here

	aprintf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		aprintf(" %s", argv[i]);
	aprintf("\n");
	aprintf("Calling with argument 'sample' executes the sample program.\n");

	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode.  Note - sometimes people change
	// around where the "M" should go.  Allow for both possibilities
	if (argc > 2) {
		if ((strcmp(argv[1], "M") ==0) || (strcmp(argv[1], "m")==0)) {
			strcpy(argv[1], argv[2]);
			strcpy(argv[2],"M\0");
		}
		if ((strcmp(argv[2], "M") ==0) || (strcmp(argv[2], "m")==0)) {
			aprintf("Simulation is running as a MultProcessor\n\n");
			mmio.Mode = Z502SetProcessorNumber;
			mmio.Field1 =MAX_NUMBER_OF_PROCESSORS;
			mmio.Field2 = (long) 0;
			mmio.Field3 = (long) 0;
			mmio.Field4 = (long) 0;
			MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
			multiflag=1;
		}
	} else {
		aprintf("Simulation is running as a UniProcessor\n");
		aprintf(
				"Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR ] = (void *) InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR ] = (void *) FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR ] = (void *) svc;

	//  Determine if the switch was set, and if so go to demo routine.

	PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) SampleCode;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context

	} // End of handler for sample code - This routine should never return here
	if(argc > 1)
	{   int length=strlen(argv[1]);
        int a=0;
        if(length==5)
		{
			a=argv[1][4]-'0';
		}
		else if(length==6)
		{
			a=(argv[1][4]-'0')*10+(argv[1][5]-'0');
		}
		switch(a)
		{   case 0:
			{
				starttest((long)test0);
			break;
			}
			case 1:
			{starttest((long)test1);
			break;}
			case 2:
			{starttest((long)test2);
			break;}
			case 3:
			{starttest((long)test3);
			break;}
			case 4:
			{starttest((long)test4);
			break;}
			case 5:
			{starttest((long)test5);
			break;}
			case 6:
			{starttest((long)test6);
			break;}
			case 7:
			{starttest((long)test7);
			break;}
			case 8:
			{starttest((long)test8);
			break;}
			case 9:
			{starttest((long)test9);
			break;}
			case 10:
			{starttest((long)test10);
			
			break;}
			case 11:
			{starttest((long)test11);
			break;}
			case 12:
			{starttest((long)test12);
			break;}
			case 21:
			{starttest((long)test21);
			break;}
			case 22:
			{starttest((long)test22);
			break;}
			case 23:
			{starttest((long)test23);
			break;}
			case 24:
			{starttest((long)test24);
			break;}
			case 25:
			{starttest((long)test25);
			break;}
			case 41:
			{
		    initdatame();
			starttest((long)test41);
			break;
			}
			case 42:
			{initdatame();
			starttest((long)test42);
			break;
			}
			case 43:
			{
		    initdatame();
			starttest((long)test43);
			break;
			}
			case 44:
			{
			initdatame();
			starttest((long)test44);
			break;
			}
			case 45:
			{
			initdatame();
			starttest((long)test45);
			break;
			}
			case 46:
			{
			initdatame();
			starttest((long)test46);
			break;
			}
			case 47:
			{
			initdatame();
			starttest((long)test47);
			break;
			}
		}	
	}
	
	
#ifdef          MyTestingMode
	MyTestingosInit(argc, argv);
#endif

}                                               // End of osInit
