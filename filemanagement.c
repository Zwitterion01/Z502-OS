/********************************************************************************
This filemanagement cares the disk format, file/directory read and write.
The lock here is used as MEMORY_INTERLOCK_BASE + 22 and MEMORY_INTERLOCK_BASE + 23.
********************************************************************************/
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "myglobal.h"
diskForm Disk[100];
namedir named[100];
int diskcount=0;
int INODE=0;
long findnextfreesec(long);

//given a int find its correspond MSB
byte findMSB(int number)
{
	return (number & MSBMASK)>>8;
}
//given a int find its correspond LSB
byte findLSB(int number)
{
	return (number & LSBMASK);
}
//given a diskid find its index in the disk array
int getindex(long diskid)
{
	int k=0;
//printf("\n %d\n",diskcount);
  for(;k < diskcount;k++)
  { 
	  if(Disk[k]->diskID==diskid)
	  { //aprintf("\n Now ID:%d\n",k);
        break;		
	  }
  }	 
return k;  
}

//Generate a byte array for block0 of a disk. 
byte* setupblock0(long diskid)
{  INT32 LockResult;
   READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
   Disk[diskcount]=(diskForm)malloc(sizeof(diskF));
   Disk[diskcount]->diskID=diskid;
   Disk[diskcount]->sectorID=16-2+Swap_size;   // set sector offset.
   diskcount++; 
   READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
  byte* block0=(byte*)malloc(16*sizeof(byte));
  //set the byte as slides show
  block0[0]=diskid; block0[1]=Bitmap_size; block0[2]=Root_size; block0[3]=Swap_size; 
  block0[4]=findLSB(2048);block0[5]=findMSB(2048);// A disk has 2048 sectors.
  block0[6]=findLSB(RootheaderLoc+Root_size);block0[7]=findMSB(RootheaderLoc+Root_size); 
  block0[8]=findLSB(RootheaderLoc+Root_size+Bitmap_size*4);block0[9]=findMSB(RootheaderLoc+Root_size+Bitmap_size*4);
  block0[10]=findLSB(RootheaderLoc);block0[11]=findMSB(RootheaderLoc);
  block0[12]=0;block0[13]=0; block0[14]=0; block0[15]=1;
  return block0;
}
//Set up a directory given name, time, disk, and its upper directory Inode.
byte* setupdir(char* dirname, long time,long diskID, int last_inode)
{   INT32 LockResult;
	byte* res=(byte*)malloc(16*sizeof(byte));
	//Set INODE
	res[0]=INODE;
	//Set name, 7 byte
	int length=strlen(dirname);
	int loc=0;
	for(int i=1;i<=7;i++)
	{
		if(loc<length)
		{
			res[i]=*(dirname+loc);
			loc++;
		}
		else
		{
			res[i]=0;
		}
	}
	//Set creation time, 3 byte
	 res[8] = findMSB(time);
	  res[9] = 0;
	 res[10] = findLSB(time);
	// set file discription
	byte filedis=0;
	if(strcmp(dirname,"root")==0)
	{
		filedis=1+filedis;
		filedis=1*2+filedis;
		filedis=filedis+8*31; // 31 for the root inode
	}
	else
	{
		filedis=1+filedis;
		filedis=1*2+filedis;
		filedis=filedis+8*last_inode;
	}
	res[11]=filedis;
	//set Index loc, find next free loc
	long Sector_num;
	if(strcmp(dirname,"root")==0)
	 Sector_num=2; 
    else
	 Sector_num=findnextfreesec(diskID)+1;// findnextfreesec(diskID) is the header location
	res[13]=findMSB(Sector_num);res[12]=findLSB(Sector_num);
	res[14]=res[15]=0;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	if(strcmp(dirname,"root")!=0)
	Disk[getindex(diskID)]->sectorID++; // if not root, then it will occupy a sector.
    READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 23, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
    named[INODE]=(namedir)malloc(sizeof(DIR));
	for(int i=0;i<7;i++)  // get the file header in the memory.
	named[INODE]->name[i]=dirname[i];
    named[INODE]->headerloc=Sector_num-1;
	named[INODE]->ford=1;
	named[INODE]->time=time;
	named[INODE]->size=0;
	INODE++;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 23, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return res;
}
//Get headerlocation givern filename.
long getheaderloc(char* fname)
{
  for(int i=0;i<INODE;i++)
  {
	  if(strcmp(named[i]->name,fname)==0)
		  return named[i]->headerloc;
  }	  
  return -1;
}
//Set up a file given name, time, disk, and its upper directory Inode.
byte* setupfile(char* filename, long time,long diskID, int last_inode)
{   INT32 LockResult;
	byte* res=(byte*)malloc(16*sizeof(byte));
	//Set INODE
	res[0]=INODE;
	//Set name
	int length=strlen(filename);
	int loc=0;
	for(int i=1;i<=7;i++)
	{
		if(loc<length)
		{
			res[i]=*(filename+loc);
			loc++;
		}
		else
		{
			res[i]=0;
		}
	}
	//Set creation time
	 res[8] = findMSB(time);
	  res[9] = 0;
	 res[10] = findLSB(time);
	// set file discription
	byte filedis=0;
		filedis=0+filedis;
		filedis=1*2+filedis;
		filedis=filedis+8*last_inode;
	res[11]=filedis;
	//set Index loc, find next free loc
	long Sector_num;
	 Sector_num=findnextfreesec(diskID)+1;// findnextfreesec(diskID) is the header location
	res[13]=findMSB(Sector_num);res[12]=findLSB(Sector_num);
	res[14]=res[15]=0;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	Disk[getindex(diskID)]->sectorID++;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 23, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	named[INODE]=(namedir)malloc(sizeof(DIR));
	//Get the file header information saved in the memory
	for(int i=0;i<7;i++)
	named[INODE]->name[i]=filename[i];
    named[INODE]->headerloc=Sector_num-1;
	named[INODE]->ford=0;
	named[INODE]->time=time;
	named[INODE]->size=0;
	INODE++;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 23, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	return res;
}

// Move forward the sector because of a use.
void pushsec(long diskid)
{INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
	Disk[getindex(diskid)]->sectorID++;
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 22, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
}
//Initiallize a block.
byte* Initblock()
{
	byte* res=(byte*)malloc(16*sizeof(byte));
	for(int i=0;i<16;i++)
		res[i]='\0';
	return res;
}
//set given position 1. 
void bset(byte *want, int pos){
    *want |= (1 << pos);
}
//given disk, find next free sector.
long findnextfreesec(long diskid)
{ int k=0;
//printf("\n %d\n",diskcount);
  for(;k < diskcount;k++)
  { 
	  if(Disk[k]->diskID==diskid)
	  { //aprintf("\n Now ID:%d\n",k);
        break;		
	  }
  }	  
 //  printf("\n%ld\n", Disk[k]->sectorID);
  return Disk[k]->sectorID+1;
}
//generate bitmap in the given disk
void generatebitmap(long diskid)
{     int index=0;
     for(;index<diskcount;index++)
	{
		if(Disk[index]->diskID==diskid)
			break;
	}
	  byte bitmap[4*Bitmap_size][16]; 
	  int seccount=0;
	   for(int i=0;i<Bitmap_size*4;i++)
		  for(int j=0;j<16;j++)
			  bitmap[i][j]=0;
		for(int i=0;i<Bitmap_size*4;i++)
		{   
	        while(seccount<= Disk[index]->sectorID)
			{
				int sec=seccount/128;//find sector to write
				int byteng=seccount%128; //find the sectors remain
				int byten=byteng/8;    //find the byte to write.
				int position=byteng%8;  //find the bit to set.
				//printf("\n%d %d %d",sec,byten,position);				
				bset(&bitmap[sec][byten],7-position);//in opposite direction
				seccount++;
				byteng=seccount%128;
				if(byteng==0)
					break;
			}
			writeinDisk(bitmap[i], diskid,i+3);
		}
}
//given filename, return Inode.
int getInode(char* fname)
{   int index=0;
	for(;index<INODE;index++)
	{
		 if(strcmp(named[index]->name,fname)==0)
		  return index;
	}
}
//given haederlocation, return Inode
int getInodeLoc(long loch)
{   int index=0;
	for(;index<INODE;index++)
	{
		 if(named[index]->headerloc==loch)
		  return index;
	}
}
//given Inode, return name
char* getname(int Inode)
{  
return named[Inode]->name;
}
//given Inode, return boolean judgment for file/dis
int getfileordir(int inod)
{
	return named[inod]->ford;
}
//given Inode, return time.
long gettime(int inod)
{
	return named[inod]->time;
}
//given Inode, return size.
long getsize(int inod)
{
	return named[inod]->size;
}
//given Inode, increase its size.
void filesizeincrease(int inod)
{
	named[inod]->size++;
}
//given Inode, return header sector location.
long getheaderloc_inode(int inod)
{
	return named[inod]->headerloc;
}
//given disk id, format a disk.
void formatdisk(long id)
{
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
    mmio.Field1 = mmio.Field2 =mmio.Field3 =0;
    MEM_READ(Z502Clock, &mmio);
    int timenow=(int)mmio.Field1;//get time.
    byte *block0;
	block0=setupblock0(id); // get block0
	byte *dirheader=setupdir("root", timenow,id,31);//get root directory header
	byte *headerseccontain=Initblock();//get root directory header
	long dirheadersec=transferbyte(dirheader[12],dirheader[13]);// transfer MSB LSB to int
    writeinDisk(block0,id,0);
	writeinDisk(dirheader,id,1);
	generatebitmap(id);
	writeinDisk(headerseccontain,id,dirheadersec);

}












