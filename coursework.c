#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include<math.h>
#include<string.h>
#include<ctype.h>
#include<stdint.h>

typedef struct __attribute__((__packed__)) {
    uint8_t     BS_jmpBoot[ 3 ];    
    uint8_t     BS_OEMName[ 8 ]; 
    uint16_t    BPB_BytsPerSec; 
    uint8_t     BPB_SecPerClus;
    uint16_t    BPB_RsvdSecCnt;
    uint8_t     BPB_NumFATs;
    uint16_t    BPB_RootEntCnt;
    uint16_t    BPB_TotSec16;
    uint8_t     BPB_Media;
    uint16_t    BPB_FATSz16; 
    uint16_t    BPB_SecPerTrk;
    uint16_t    BPB_NumHeads;
    uint32_t    BPB_HiddSec;
    uint32_t    BPB_TotSec32;
    uint8_t     BS_DrvNum;
    uint8_t     BS_Reserved1;
    uint8_t     BS_BootSig;
    uint32_t    BS_VolID;
    uint8_t     BS_VolLab[ 11 ];
    uint8_t     BS_FilSysType[ 8 ];
} BootSector;

typedef struct{
    uint8_t     DIR_Name[ 11 ];
    uint8_t     DIR_Attr;
    uint8_t     DIR_NTRes;
    uint8_t     DIR_CrtTimeTenth;
    uint16_t    DIR_CrtTime;
    uint16_t    DIR_CrtDate;
    uint16_t    DIR_LstAccDate;
    uint16_t    DIR_FstClusHI;
    uint16_t    DIR_WrtTime;
    uint16_t    DIR_WrtDate;
    uint16_t    DIR_FstClusLO;
    uint32_t    DIR_FileSize;
} DirectoryEntry;

typedef struct{
	uint8_t		LDIR_Ord;		// Order/ position in sequence
	uint8_t		LDIR_Name1[ 10 ];	// First 5 UNICODE characters
	uint8_t		LDIR_Attr;		// = ATTR_LONG_NAME
	uint8_t		LDIR_Type;		// Should = 0
	uint8_t		LDIR_Chksum;		// Checksum of short name
	uint8_t		LDIR_Name2[ 12 ];	// Middle 6 UNICODE characters
	uint16_t	LDIR_FstClusLO;		// MUST be zero
	uint8_t		LDIR_Name3[ 4 ];	// Last 2 UNICODE characters
} LongDirectoryEntry;

int file_desc;
BootSector* bootSecPointer;
uint16_t* FATtable;
DirectoryEntry* mainDirectoryEntPointer;
int mainOffset;

//function to capitalize a string
char* capitalise(char* string){
    for(int x=0;x<strlen(string);x++){
        string[x]=toupper(string[x]);
    }
    return string;
}

//function to count number of clusters in a cluster chain given first cluster
int clusterCount(uint16_t input_cluster){
    int cluster_count=0;
    while(input_cluster<65024){
        input_cluster=FATtable[input_cluster];
        cluster_count++;
    }
    return cluster_count;
}


//function to check if directory entry contains a long filename
int if_longDirectory(DirectoryEntry* directoryEntPointer, int y){
    if(y==0)
        return 0;
    int ReadOnly=(((directoryEntPointer[y]).DIR_Attr)&1);   
    int Hidden=((((directoryEntPointer[y]).DIR_Attr)&2)>>1);    
    int System=((((directoryEntPointer[y]).DIR_Attr)&4)>>2);
    int VolumeName=((((directoryEntPointer[y]).DIR_Attr)&8)>>3);
    int Directory=((((directoryEntPointer[y]).DIR_Attr)&16)>>4);
    int Archive=((((directoryEntPointer[y]).DIR_Attr)&32)>>5); 
    if(!(ReadOnly==1 && Hidden==1 && System==1 && VolumeName==1))
        return 0;
    else
        return 1;
}


//function to print long filename of a directory entry(TASK 6)
void print_long_filename(DirectoryEntry* directoryEntPointer,int y){
    if(if_longDirectory(directoryEntPointer,y-1)){
                LongDirectoryEntry *LongDirectoryEntryPointer = (LongDirectoryEntry*)malloc(sizeof(LongDirectoryEntry));
                LongDirectoryEntryPointer=(LongDirectoryEntry *)(&(directoryEntPointer[y-1]));
                for(int x=0;x<10;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name1[x]!=255){
                        printf("%c",LongDirectoryEntryPointer->LDIR_Name1[x]);

                    }
                }
                for(int x=0;x<12;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name2[x]!=255){
                        printf("%c",LongDirectoryEntryPointer->LDIR_Name2[x]);

                    }
                }
                for(int x=0;x<4;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name3[x]!=255){
                        printf("%c",LongDirectoryEntryPointer->LDIR_Name3[x]);
                      
                    }
                }                                
                if(if_longDirectory(directoryEntPointer,y-2))
                    print_long_filename(directoryEntPointer,y-1);
    }
}

//function which returns long filename of a directory entry
char* get_long_filename(DirectoryEntry* directoryEntPointer,int y,char* longNameBuffer,int offset){
    int offset2 = offset;
    if(if_longDirectory(directoryEntPointer,y-1)){
                LongDirectoryEntry *LongDirectoryEntryPointer = (LongDirectoryEntry*)malloc(sizeof(LongDirectoryEntry));
                LongDirectoryEntryPointer=(LongDirectoryEntry *)(&(directoryEntPointer[y-1]));
                for(int x=0;x<10;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name1[x]!=255){
                        sprintf(longNameBuffer+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name1[x]);
                        offset2+=1;
                    }
                }
                for(int x=0;x<12;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name2[x]!=255){
                        sprintf(longNameBuffer+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name2[x]);
                        offset2+=1;
                    }
                }
                for(int x=0;x<4;x++){
                    if(LongDirectoryEntryPointer->LDIR_Name3[x]!=255){
                        sprintf(longNameBuffer+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name3[x]);
                        offset2+=1;                      
                    }
                }                                
                if(if_longDirectory(directoryEntPointer,y-2))
                    get_long_filename(directoryEntPointer,y-1,longNameBuffer,offset2);
    }
    return longNameBuffer;
}

//function which returns size of a file in bytes given its starting cluster number and parent directory pointer
int sizeOfFile(uint16_t starting_cluster,DirectoryEntry* directoryEntPointer){
    int y=0;
    while((directoryEntPointer[y].DIR_Name[0])!=0){    
        if((int)((directoryEntPointer[y]).DIR_FstClusLO)==(int)starting_cluster){
            return (int)(directoryEntPointer[y]).DIR_FileSize;
        }
        y++;
    }
    return 0;
}

//function to print directory table given its pointer
void printDirectoryTable(DirectoryEntry* directoryEntPointer){
        int y=0;
        while((directoryEntPointer[y].DIR_Name[0])!=0){
            int ReadOnly=(((directoryEntPointer[y]).DIR_Attr)&1);   
            int Hidden=((((directoryEntPointer[y]).DIR_Attr)&2)>>1);    
            int System=((((directoryEntPointer[y]).DIR_Attr)&4)>>2);
            int VolumeName=((((directoryEntPointer[y]).DIR_Attr)&8)>>3);
            int Directory=((((directoryEntPointer[y]).DIR_Attr)&16)>>4);
            int Archive=((((directoryEntPointer[y]).DIR_Attr)&32)>>5);  

            int year=1980+((((directoryEntPointer[y]).DIR_WrtDate)&65024)>>9);
            int month=((((directoryEntPointer[y]).DIR_WrtDate)&480)>>5);
            int day=((((directoryEntPointer[y]).DIR_WrtDate)&31));

            int hour=((((directoryEntPointer[y]).DIR_WrtTime)&63488)>>11);
            int minute=((((directoryEntPointer[y]).DIR_WrtTime)&2016)>>5);
            int second=2*((((directoryEntPointer[y]).DIR_WrtTime)&31));

            if(!(ReadOnly==1 && Hidden==1 && System==1 && VolumeName==1)){
                printf("First_cluseter = %-5hu",(directoryEntPointer[y]).DIR_FstClusLO);
                printf("Last_modified = %04d-%02d-%02d   %02d-%02d-%02d   ",year,month,day,hour,minute,second);
                printf("R-%d H-%d S-%d V-%d D-%d A-%d    ",ReadOnly,Hidden,System,VolumeName,Directory,Archive);
                printf("File_length = %-10u",(directoryEntPointer[y]).DIR_FileSize);
                if(if_longDirectory(directoryEntPointer,y-1))
                    print_long_filename(directoryEntPointer,y);
                else{
                    for(int x=0;x<11;x++)
                    {
                        if(x==8&&Directory==0&&VolumeName==0)
                            printf(".");
                        if((directoryEntPointer[y]).DIR_Name[x]!=' ')
                            printf("%c",(directoryEntPointer[y]).DIR_Name[x]);
                    }
                }                    
                printf("\n");

            }
            y++;
            
        }
        printf("\n\n");
}

//function to find starting cluster of a file given its filename and parent directory pointer
int clusterFromName(char* inpFilename,DirectoryEntry* directoryEntPointer){
        int y=0;
        while((directoryEntPointer[y].DIR_Name[0])!=0){
            int ReadOnly=(((directoryEntPointer[y]).DIR_Attr)&1);   
            int Hidden=((((directoryEntPointer[y]).DIR_Attr)&2)>>1);    
            int System=((((directoryEntPointer[y]).DIR_Attr)&4)>>2);
            int VolumeName=((((directoryEntPointer[y]).DIR_Attr)&8)>>3);
            int Directory=((((directoryEntPointer[y]).DIR_Attr)&16)>>4);


            if(!(ReadOnly==1 && Hidden==1 && System==1 && VolumeName==1)){

                if(if_longDirectory(directoryEntPointer,y-1)){
                    char* longName=(char*)malloc(256);
                    int offset2 = 0;
                    int ys=y;
                    while(if_longDirectory(directoryEntPointer,ys-1)){
                        LongDirectoryEntry *LongDirectoryEntryPointer = (LongDirectoryEntry*)malloc(sizeof(LongDirectoryEntry));
                        LongDirectoryEntryPointer=(LongDirectoryEntry *)(&(directoryEntPointer[ys-1]));
                        for(int x=0;x<10;x++){
                            if((LongDirectoryEntryPointer->LDIR_Name1[x]!=255)&&(LongDirectoryEntryPointer->LDIR_Name1[x]!=0)){
                                sprintf(longName+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name1[x]);
                                offset2+=1;
                            }
                        }
                        for(int x=0;x<12;x++){
                            if((LongDirectoryEntryPointer->LDIR_Name2[x]!=255)&&(LongDirectoryEntryPointer->LDIR_Name2[x]!=0)){
                                sprintf(longName+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name2[x]);
                                offset2+=1;
                            }
                        }
                        for(int x=0;x<4;x++){
                            if(LongDirectoryEntryPointer->LDIR_Name3[x]!=255&&(LongDirectoryEntryPointer->LDIR_Name3[x]!=0)){
                                sprintf(longName+offset2,"%c",LongDirectoryEntryPointer->LDIR_Name3[x]);
                                offset2+=1;                      
                            }
                        }                                
                        if(if_longDirectory(directoryEntPointer,ys-2)){
                            ys=ys-1;
                        }
                        else{
                            break;
                        }
                    }
                    longName[offset2]='\0';

                    if(strcmp(capitalise(longName),capitalise(inpFilename))==0){
                        free(longName);
                        return directoryEntPointer[y].DIR_FstClusLO;
                    }
                    free(longName);

                }

                if(strlen(inpFilename)<13){
                    char*fname=malloc(sizeof(char)*13);
                    int z=0;                  
                    for(int x=0;x<13;x++)
                    {

                        if(x==7&&Directory==0&&VolumeName==0){
                            if((x!=11)&&(directoryEntPointer[y]).DIR_Name[x]!=' '){
                                sprintf(fname+z,"%c",(directoryEntPointer[y]).DIR_Name[x]);
                                z++;
                            }
                            sprintf(fname+z,"%c",'.');
                            z++;
                        }                          
                        else if((x!=11)&&(directoryEntPointer[y]).DIR_Name[x]!=' '){
                            sprintf(fname+z,"%c",(directoryEntPointer[y]).DIR_Name[x]);
                            z++;
                        }
                      
                        fname[x]=toupper(fname[x]);
                        inpFilename[x]=toupper(inpFilename[x]);
                    }
                    for(int x=0;x<13;x++){
                        if(fname[x]==' ')
                        {
                            fname[x]='\0';
                            break;
                        }
                    }
                    if(strcmp(fname,inpFilename)==0){
                        return directoryEntPointer[y].DIR_FstClusLO;
                    }
                    free(fname);
                }
            }
            y++;
        }
        return -1;
}

//function which prints a file given its starting cluster and parent directory pointer(TASK 5)
DirectoryEntry* openFile(uint16_t starting_cluster,DirectoryEntry* directoryEntPointer){

    int cluster_count=clusterCount(starting_cluster+2);

    int file_size = sizeOfFile(starting_cluster+2,directoryEntPointer);
    
    char* fileBuffer = malloc((cluster_count)*(bootSecPointer->BPB_BytsPerSec)*(bootSecPointer->BPB_SecPerClus));

    char* fileBufferPointer= fileBuffer;
    memset(fileBufferPointer, 0, 1);

    int if_directory=0;
    int y=0;
    while((directoryEntPointer[y].DIR_Name[0])!=0){
        if((int)(directoryEntPointer[y]).DIR_FstClusLO==(int)starting_cluster+2){
            if_directory=((((directoryEntPointer[y]).DIR_Attr)&16)>>4);
        }
        y++;
    }

    if(if_directory==0){
        uint16_t FAT_starting_cluster=starting_cluster+2;
        while(FAT_starting_cluster<65024){
            lseek(file_desc,mainOffset+((FAT_starting_cluster-2)*(bootSecPointer->BPB_SecPerClus)*(bootSecPointer->BPB_BytsPerSec)),SEEK_SET);
            read(file_desc,fileBufferPointer,(bootSecPointer->BPB_SecPerClus)*(bootSecPointer->BPB_BytsPerSec));
            fileBufferPointer+=(bootSecPointer->BPB_SecPerClus)*(bootSecPointer->BPB_BytsPerSec);
            FAT_starting_cluster=FATtable[FAT_starting_cluster];
        }
        for(int x=0;x<file_size;x++){
            printf("%c",fileBuffer[x]);
        }
        printf("\n");
        return 0;
    }
    else
    {
        int y=1;
        DirectoryEntry* subDirectoryEntPointer  = (DirectoryEntry*)malloc(sizeof(DirectoryEntry)*(64)*1);
        while(starting_cluster<65004){
                lseek(file_desc,mainOffset+((starting_cluster)*(bootSecPointer->BPB_SecPerClus)*(bootSecPointer->BPB_BytsPerSec)),SEEK_SET);  
                subDirectoryEntPointer  = (DirectoryEntry*)realloc(subDirectoryEntPointer,sizeof(DirectoryEntry)*(64*y));
                read(file_desc,subDirectoryEntPointer+((y-1)*64), sizeof(DirectoryEntry)*(64));
                y++;
            starting_cluster=FATtable[starting_cluster+2]-2;
        }
            printDirectoryTable(subDirectoryEntPointer);
            return subDirectoryEntPointer;
    }
    return 0;
}

//function to print file given its path
void findFromPath(char path[]){
    char delim[]="/";
    DirectoryEntry* curDirectory=mainDirectoryEntPointer;
    char* pathPart=strtok(path,delim);

    while((curDirectory != 0)&&(pathPart!=NULL)) {
        if(clusterFromName(pathPart,curDirectory)>0){
            curDirectory=openFile((uint16_t)(clusterFromName(pathPart,curDirectory))-2,curDirectory);
        }
        else if(clusterFromName(pathPart,curDirectory)<0){
            printf("Unable to find file %s\n",pathPart);
            break;
        }
        pathPart = strtok(NULL,delim);
    }
}
 

int main(){

    printf("Enter the name of the FAT16 disk image you would like to open \n");         //user inputs disk name to open
    char diskName[255];
    fgets(diskName,255,stdin);
    if ((strlen(diskName) > 0) && (diskName[strlen (diskName) - 1] == '\n'))
        diskName[strlen (diskName) - 1] = '\0';
    file_desc = open(diskName, O_RDONLY);;
    if(file_desc!=-1){
        printf("Disk image successfully opened\n");
        bootSecPointer  = (BootSector*)malloc(sizeof(BootSector));                      //boot sector is printed(TASK 2)
        read(file_desc,bootSecPointer, sizeof(BootSector));
        printf("\n");
        printf("OEM names: %s\n",bootSecPointer->BS_OEMName);
        printf("Bytes per sector : %hu\n",bootSecPointer->BPB_BytsPerSec);
        printf("Sectors per clustor : %hu\n",bootSecPointer->BPB_SecPerClus);
        printf("Reserved sector count : %hu\n",bootSecPointer->BPB_RsvdSecCnt);
        printf("Number of FATs : %hu\n",bootSecPointer->BPB_NumFATs);
        printf("Size of root DIR : %hu\n",bootSecPointer->BPB_RootEntCnt);
        printf("Total sectors : %hu\n",bootSecPointer->BPB_TotSec16);
        printf("(FAT size) Sectors in FAT : %hu\n",bootSecPointer->BPB_FATSz16);
        printf("Sectors if total sectors is 0 : %u\n",bootSecPointer->BPB_TotSec32);
        printf("Non zero terminated string : %s\n",bootSecPointer->BS_VolLab);
        printf("\n\n");


        FATtable = (uint16_t*)malloc((bootSecPointer->BPB_BytsPerSec)*(bootSecPointer->BPB_FATSz16));   //FAT table is loaded loaded into memory(TASK 3)
        lseek(file_desc,(bootSecPointer->BPB_RsvdSecCnt)*(bootSecPointer->BPB_BytsPerSec),SEEK_SET);
        read(file_desc,FATtable, (bootSecPointer->BPB_BytsPerSec)*(bootSecPointer->BPB_FATSz16));
        lseek(file_desc, (bootSecPointer->BPB_BytsPerSec)*(bootSecPointer->BPB_FATSz16),SEEK_CUR);
                                                                                                        //Root directory is printed(TASK 4)

        mainDirectoryEntPointer  = (DirectoryEntry*)malloc(sizeof(DirectoryEntry)*((bootSecPointer->BPB_RootEntCnt)));
        read(file_desc,mainDirectoryEntPointer, sizeof(DirectoryEntry)*(bootSecPointer->BPB_RootEntCnt));      
        printDirectoryTable(mainDirectoryEntPointer);

        mainOffset=(bootSecPointer->BPB_RsvdSecCnt)*(bootSecPointer->BPB_BytsPerSec)+(bootSecPointer->BPB_BytsPerSec)*(bootSecPointer->BPB_FATSz16)*(bootSecPointer->BPB_NumFATs)+(sizeof(DirectoryEntry))*(bootSecPointer->BPB_RootEntCnt); 


    
        char userInputPath[255];
        printf("Enter file path to open \n");                                       //A file is printed given its path(TASK 7)
        fgets(userInputPath,255,stdin);
        if ((strlen(userInputPath) > 0) && (userInputPath[strlen (userInputPath) - 1] == '\n'))
            userInputPath[strlen (userInputPath) - 1] = '\0';
        findFromPath(userInputPath);


        close(file_desc);

    }
    else{
        printf("Couldn't open disk file\n");
    }
}

