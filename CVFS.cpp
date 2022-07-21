#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<iostream>

#define MAXINODE 50             // This is the maximum number of INODES inside the DILB i.e Disk Inode List Block

#define READ 1                  // This is the permission for Reading the file
#define WRITE 2                 // This is the permission for Reading the file
                                // 3 -> This is the permission for Reading & Writing the file

#define MAXFILESIZE 1024        // Limitation of the project that the max file size can be of 1kb

#define REGULAR 1               // This is for regular file
#define SPECIAL 2               // This is for special file for future enhancement

#define START 0                 // This is for the cursor to be at start of the file
#define CURRENT 1               // This is for the cursor to be at current position in the file
#define END 2                   // This is for the cursor to be at end of the file

typedef struct superblock
{
    int TotalInodes;
    int FreeInode;
}SUPERBLOCK,*PSUPERBLOCK;

typedef struct inode
{
    char FileName[MAXINODE];
    int InodeNumber;
    int FileSize;
    int FileActualSize;
    int FileType;
    char *Buffer;               // Pointing to Data Block
    int LinkCount;              // Always it is 1 : It is the count of the shortcut/duplicate file in our System.  There are two types of links Softlinks(shortcuts) and Hardlinks(They cannot be made actually)
    int ReferenceCount;         // The number of references refering to the entity is called ReferenceCount . Eg - You are watching TV ReferenceCount is 1, your brother came he also sits and is watching TV now ReferenceCount is 2 and vice-versa, if the ReferenceCount is zero then as no one is referring it so it should be closed.
    int permission; // 1 2 3
    struct inode *next;         // Points to the next inode
}INODE,*PINODE,**PPINODE;       // **PPINODE is not used anywhere

typedef struct filetable
{
    int readoffset;
    int writeoffset;
    int count;                  // Always 1
    int mode; // 1 2 3
    PINODE ptrinode;            // Pointing to struct inode (Line 28)
}FILETABLE,*PFILETABLE;

typedef struct ufdt
{
    PFILETABLE ptrfiletable;    // Pointing to struct filetable (Line 42)
}UFDT;

UFDT UFDTArr[MAXINODE];
SUPERBLOCK SUPERBLOCKobj;
PINODE head = NULL;

void man(char *name)
{
    if(name == NULL) return;
    if(strcmp(name, "create") == 0)
    {
        printf("Description : Used to create new regular file\n");
        printf("Usage : create File_name Permission\n");
    }
    else if(strcmp(name, "read")== 0)
    {
        printf("Description : Used to read data from regular file\n");
        printf("Usage : read File_name No_Of_Bytes_To_Read\n");
    }
    else if(strcmp(name, "write" )== 0)
    {
        printf("Description : Used to write into regular file\n");
        printf("Usage : write File_name\n After this enter the data that we want to write \n ");
    }
    else if(strcmp(name, "ls")== 0)
    {
        printf("Description : Used to list all information of file\n");
        printf("Usage : ls\n");
    }
    else if(strcmp(name,"stat") == 0)
    {
        printf("Description : Used to Display information of file\n");
        printf("Usage : stat File_name\n");
    }
    else if(strcmp(name,"fstat")== 0)
    {
        printf("Description : Used to display information of file\n");
        printf("Usage : stat File_Descriptor\n");
    }
    else if(strcmp(name,"truncate")== 0)
    {
        printf("Description : Used to remove data from file\n");
        printf("Usage : truncate File_name\n");
    }
    else if(strcmp(name,"open")== 0)
    {
        printf("Description : Used to open existing file \n");
        printf("Usage : open File_name mode\n");
    }
    else if(strcmp(name,"close")==0)
    {
        printf("Description : Used to close opened file \n");
        printf("Usage : close File_name mode \n");
    }
    else if(strcmp(name,"closeall")== 0)
    {
        printf("Description : Used to close all opened file \n");
        printf("Usage : closeall\n");
    }
    else if(strcmp(name,"lseek")==0)
    {
        printf("Description : Used to change file offset");
        printf("Usage : lseek File_Name changeInOffset StartPoint\n");
    }
    else if(strcmp(name,"rm")== 0)
    {
        printf("Description : Used to delete the file \n");
        printf("Usage : rm File_Name\n");
    }
    else
    {
        printf("ERROR : No manual entry available \n");
    }
}

void DisplayHelp()
{
    printf("ls : To list out files \n");
    printf("clear : To clear console \n");
    printf("open : To open the file \n");
    printf("close : To close the file \n");
    printf("closeall : To close all opened files \n");
    printf("read : To Read the contents from file \n");
    printf("write : To Write contents into file \n");
    printf("exit : To terminate file System \n");
    printf("stat : To Display information of file using name \n");
    printf("fstat : To Display information of file using name \n");
    printf("truncate : To Remove all data from file \n");
    printf("rm : To Delete the file \n");
}

int GetFDFromName(char *name)
{
    int i = 0;
    while(i<MAXINODE)
    {
        if((UFDTArr[i].ptrfiletable!=NULL) && (UFDTArr[i].ptrfiletable->ptrinode->FileType != 0))
            if(strcmp((UFDTArr[i].ptrfiletable->ptrinode->FileName),name)== 0)
            break;
        i++;
    }
    if(i == MAXINODE) return -1;
    else return i;
}

PINODE Get_Inode(char *name)
{
    PINODE temp = head;
    int i=0;
    if(name == NULL)
    return NULL;
    while(temp!=NULL)
    {
        if(strcmp(name,temp->FileName) == 0)
        break;
        temp = temp->next;
    }
    return temp;
}

void CreateDILB()
{
    int i=0;
    PINODE newn = NULL;
    PINODE temp = head;
    while(i<= MAXINODE)
    {
        newn = (PINODE)malloc(sizeof(INODE));
        newn->LinkCount =0;
        newn->ReferenceCount =0;
        newn->FileType =0;
        newn->FileSize =0;
        newn->Buffer=NULL;
        newn->next=NULL;
        newn->InodeNumber =i;
        if(temp == NULL)
        {
            head = newn;
            temp = head;
        }
        else
        {
            temp->next=newn;
            temp=temp->next;
        }
        i++;
    }
    printf("DILB Created Successfully \n");
}

void InitialiseSuperBlock()
{
    int i = 0;              // If you want to reserve 1st three inodes then initialize i = 3
    while(i<MAXINODE)
    {
        UFDTArr[i].ptrfiletable = NULL;
        i++;
    }

    SUPERBLOCKobj.TotalInodes = MAXINODE;       // TotalInodes = MAXINODE
    SUPERBLOCKobj.FreeInode = MAXINODE;         // FreeInodes out of MAXINODE Inodes
}

int CreateFile(char *name,int permission)
{
    int i=0;
    PINODE temp = head;
    if((name == NULL)||(permission == 0)||(permission > 3))
    return -1;
    if (SUPERBLOCKobj.FreeInode == 0)
    return -2;
    (SUPERBLOCKobj.FreeInode)--;
    if(Get_Inode(name)!= NULL)
    return -3;
    while(temp!=NULL)
    {
        if(temp->FileType == 0)
        break;
        temp=temp->next;
    }
    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable == NULL)
        break;
        i++;
    }
    UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));
    UFDTArr[i].ptrfiletable->count = 1;
    UFDTArr[i].ptrfiletable->mode = permission;
    UFDTArr[i].ptrfiletable->readoffset = 0;
    UFDTArr[i].ptrfiletable->writeoffset = 0;
    UFDTArr[i].ptrfiletable->ptrinode = temp;
    strcpy(UFDTArr[i].ptrfiletable->ptrinode->FileName,name);
    UFDTArr[i].ptrfiletable->ptrinode->FileType=REGULAR;
    UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount=1;
    UFDTArr[i].ptrfiletable->ptrinode->LinkCount=1;
    UFDTArr[i].ptrfiletable->ptrinode->FileSize=MAXFILESIZE;
    UFDTArr[i].ptrfiletable->ptrinode->FileActualSize=0;
    UFDTArr[i].ptrfiletable->ptrinode->permission=permission;
    UFDTArr[i].ptrfiletable->ptrinode->Buffer = (char*)malloc(MAXFILESIZE);
    return i;
}

// rm_File("Demo.txt")
int rm_File(char * name)
{
    int fd=0;
    fd= GetFDFromName(name);
    if(fd == -1)
    {
        return -1;
    }
    (UFDTArr[fd].ptrfiletable->ptrinode->LinkCount)--;
    if(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount == 0)
    {
        UFDTArr[fd].ptrfiletable->ptrinode->FileType = 0;
        //free(UFDTArr[fd].ptrfiletable->ptrinode->Buffer);
        free(UFDTArr[fd].ptrfiletable);
    }
    UFDTArr[fd].ptrfiletable = NULL;
    (SUPERBLOCKobj.FreeInode)++;
    
}

int ReadFile(int fd,char *arr,int isize)
{
    int read_size = 0;
    if(UFDTArr[fd].ptrfiletable == NULL) return -1;
    if(UFDTArr[fd].ptrfiletable->mode !=READ && UFDTArr[fd].ptrfiletable->mode !=READ+WRITE) return -2;
    if(UFDTArr[fd].ptrfiletable->ptrinode->permission!= READ &&UFDTArr[fd].ptrfiletable->ptrinode->permission!= READ+WRITE) return -2;
    if(UFDTArr[fd].ptrfiletable->readoffset ==UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) return -3;
    if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
    return -4;
    read_size = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) -(UFDTArr[fd].ptrfiletable->readoffset);
    if(read_size < isize)
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) +(UFDTArr[fd].ptrfiletable->readoffset),read_size);
        UFDTArr[fd].ptrfiletable->readoffset =UFDTArr[fd].ptrfiletable->readoffset + read_size;
    }
    else
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer)+(UFDTArr[fd].ptrfiletable->readoffset), isize);
        (UFDTArr[fd].ptrfiletable->readoffset)=(UFDTArr[fd].ptrfiletable->readoffset) + isize;
    }
    return isize;
}

int WriteFile(int fd,char *arr,int isize)
{
    if(((UFDTArr[fd].ptrfiletable->mode) !=WRITE) &&((UFDTArr[fd].ptrfiletable->mode)!=READ+WRITE)) return -1;
    if(((UFDTArr[fd].ptrfiletable->ptrinode->permission) !=WRITE)&& ((UFDTArr[fd].ptrfiletable->ptrinode->permission) !=READ+WRITE)) return -1;
    
    if((UFDTArr[fd].ptrfiletable->writeoffset) == MAXFILESIZE) return -2;
    
    if((UFDTArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR) return -3;
    
    strncpy((UFDTArr[fd].ptrfiletable->ptrinode->Buffer) +(UFDTArr[fd].ptrfiletable->writeoffset),arr,isize);   // UFDTArr[fd].ptrfiletable->writeoffset is appending the text at the last of the file
    (UFDTArr[fd].ptrfiletable->writeoffset)=(UFDTArr[fd].ptrfiletable->writeoffset) + isize;    
    (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) =(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)+ isize;
    return isize;
}

int OpenFile(char *name,int mode)
{
    int i=0;
    PINODE temp =NULL;
    if(name == NULL || mode<= 0)
    return -1;
    temp=Get_Inode(name);
    if(temp == NULL)
    return -2;
    if(temp->permission< mode)
    return -3;
    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable == NULL)
        break;
        i++;
    }
    UFDTArr[i].ptrfiletable =(PFILETABLE)malloc(sizeof(FILETABLE));
    if(UFDTArr[i].ptrfiletable == NULL) return -1;
    UFDTArr[i].ptrfiletable-> count =1;
    UFDTArr[i].ptrfiletable->mode =mode;
    if(mode == READ + WRITE)
    {
        UFDTArr[i].ptrfiletable->readoffset= 0;
        UFDTArr[i].ptrfiletable->writeoffset =0;
    }
    else if(mode == READ)
    {
        UFDTArr[i].ptrfiletable->readoffset= 0;
    }
    else if(mode == WRITE)
    {
        UFDTArr[i].ptrfiletable->writeoffset= 0;
    }
    UFDTArr[i].ptrfiletable->ptrinode =temp;
    (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)++;
    return i;
}

void CloseFileByName(int fd)
{
    UFDTArr[fd].ptrfiletable->readoffset = 0;
    UFDTArr[fd].ptrfiletable->writeoffset = 0;
    (UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}

int CloseFileByName(char *name)
{
    int i=0;
    i=GetFDFromName(name);
    if(i == -1)
    return -1;
    UFDTArr[i].ptrfiletable->readoffset = 0;
    UFDTArr[i].ptrfiletable->writeoffset =0;
    (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
    return 0;
}

void CloseAllFile()
{
    int i=MAXINODE;
    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable !=NULL)
        {
            UFDTArr[i].ptrfiletable->readoffset = 0;
            UFDTArr[i].ptrfiletable->writeoffset= 0;
            (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
            break;
        }
        i++;
    }
}

int LseekFile(int fd,int size,int from)
{
    if((fd<0)||(from>2)) return -1;
    if(UFDTArr[fd].ptrfiletable == NULL) return -1;
    if((UFDTArr[fd].ptrfiletable-> mode ==READ)||(UFDTArr[fd].ptrfiletable->mode == READ+WRITE))
    {
        if(from == CURRENT)
        {
            if(((UFDTArr[fd].ptrfiletable->readoffset)+size)>UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) return -1;
            if(((UFDTArr[fd].ptrfiletable->readoffset) +size)< 0) return -1;
            (UFDTArr[fd].ptrfiletable->readoffset)=(UFDTArr[fd].ptrfiletable->readoffset) +size;
        }
        else if(from == START)
        {
            if(size>(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)) return -1;
            if(size<0) return -1;
            (UFDTArr[fd].ptrfiletable->readoffset) = size;
        }
        else if(from == END)
        {
            if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE) return -1;
            if(((UFDTArr[fd].ptrfiletable->readoffset+size)<0)) return -1;
            (UFDTArr[fd].ptrfiletable->readoffset) =
            (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) +size;
        }
    }
    else if(UFDTArr[fd].ptrfiletable->mode == WRITE)
    {
        if(from == CURRENT)
        {
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size )>
            MAXFILESIZE) return -1;
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
            return -1;
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) >(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)=(UFDTArr[fd].ptrfiletable->writeoffset) + size;
            (UFDTArr[fd].ptrfiletable->writeoffset)=(UFDTArr[fd].ptrfiletable->writeoffset) + size;
        }
        else if(from== START)
        {
            if(size > MAXFILESIZE) return -1;
            if(size < 0) return -1;
            if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
            (UFDTArr[fd].ptrfiletable -> writeoffset)=size;
        }
        else if(from== END)
        {
        if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE) return -1;
        if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) <0)
        return -1;
        (UFDTArr[fd].ptrfiletable->writeoffset) =(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
        }
    }
    return 0;
}

void ls_file()
{
    int i = 0;
    PINODE temp=head;
    if(SUPERBLOCKobj.FreeInode == MAXINODE)
    {
        printf("ERROR : There is no file \n");
        return ;
    }
    printf("\n File Name\t Inode number\tFile size\t Link Count\n");
    printf("-----------------------------------------------------------------\n");
    while(temp != NULL)
    {
        if(temp->FileType != 0)
        {
            printf("%s\t\t%d\t\t%d\n",temp->FileName,temp->InodeNumber,temp->FileActualSize,temp->LinkCount);
        }
        temp=temp->next;
    }
    printf("-----------------------------------------------------------------\n");
}

int fstat_file(int fd)
{
    PINODE temp = head;
    int i=0;
    if(fd<0) return -1;
    if(UFDTArr[fd].ptrfiletable == NULL) return -2;
    temp=UFDTArr[fd].ptrfiletable->ptrinode;
    printf("\n-------------------Statistical Information About File------------\n");
    printf("File name :%s \n",temp->FileName);
    printf("Inode Number %d \n",temp->InodeNumber);
    printf("File Size : %d \n",temp->FileSize);
    printf("Actual File Size \n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference count : %d\n",temp->ReferenceCount);
    if(temp->permission == 1)
    printf("File Permission : Read only \n");
    else if(temp->permission == 2)
    printf("File permission : Write only \n");
    else if(temp->permission==3)
    printf("File permission : Read & Write\n");
    printf("-----------------------------------------------------------------\n\n");
    return 0;
}

int stat_file(char *name)
{
    PINODE temp= head;
    int i=0;
    if(name==NULL) return -1;
    while(temp != NULL)
    {
        if(strcmp(name,temp->FileName)== 0)
        break;
        temp=temp->next;
    }
    if(temp == NULL) return -2;
    printf("\n-------------------Statistical Information About File------------\n");
    printf("File name : %s \n",temp->FileName);
    printf("Inode Number : %d \n",temp->InodeNumber);
    printf("File Size : %d \n",temp->FileSize);
    printf("Actual File Size : \n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference count : %d\n",temp->ReferenceCount);
    if(temp->permission == 1)
    printf("File Permission : Read only \n");
    else if(temp->permission == 2)
    printf("File permission : Write only \n");
    else if(temp->permission ==3)
    printf("File permission : Read & Write\n");
    printf("-----------------------------------------------------------------\n\n");
    return 0;
}

int truncate_File(char *name)
{
    int fd = GetFDFromName(name);
    if(fd == -1)
    return -1;
    memset(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,0,MAXFILESIZE);
    UFDTArr[fd].ptrfiletable->readoffset = 0;
    UFDTArr[fd].ptrfiletable->writeoffset =0;
    UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize =0;
    return 0;
}

int main()
{
    char *ptr = NULL;
    int ret = 0, fd = 0, count = 0;
    char command[4][80],str[80],arr[MAXFILESIZE];  // 80 here is used because in cmd prompt in one line we can write 80 characters

    InitialiseSuperBlock();     // Auxilary data - The data or preparations that should br done prior the actual CVFS project starts
    CreateDILB();               // Auxilary data - The data or preparations that should br done prior the actual CVFS project starts

    // Shell
    while(1)    // Such a loop is called "Infinite listening loop"
    {
        fflush(stdin);      // cleans the previous keyboard input/buffer
        strcpy(str,"");

        printf("\nMarvellous VFS : > ");
        
        fgets(str,80,stdin); //scanf("%[^'\n']s",str);
        
        count=sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);
        // sscanf means string madhe scanf de

        if(count == 1)
        {
            if(strcmp(command[0],"ls")== 0)
            {
                ls_file();
            }
            else if(strcmp(command[0],"closeall")== 0)
            {
                CloseAllFile();
                printf("All file closed successfully\n");
                continue;
            }
            else if(strcmp(command[0],"clear")== 0)
            {
                system("cls");
                continue;
            }
            else if(strcmp(command[0],"help")== 0)
            {
                DisplayHelp();
                continue;
            }
            else if(strcmp(command[0],"exit")== 0)
            {
                printf("Terminating the Marvellous virtual file System\n");
                break;
            }
            else
            {
            printf("\nERROR : Command not Found!!!\n");
            continue;
            }
        }
        else if(count == 2)
        {
            if(strcmp(command[0],"stat")== 0)
            {
                ret= stat_file(command[1]);
                if(ret == -1)
                printf("ERROR : Incorrect Parameter\n");
                if(ret== -2)
                printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"fstat")== 0)
            {
                ret= stat_file(command[1]);
                if(ret == -1)
                printf("ERROR : Incorrect Parameter\n");
                if(ret== -2)
                printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"close")== 0)
            {
                ret= CloseFileByName(command[1]);
                if(ret== -1)
                printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"rm")== 0)
            {
                ret= rm_File(command[1]);
                if(ret== -1)
                printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"man")==0)
            {
                man(command[1]);
            }
            else if(strcmp(command[0],"write")==0)
            {
                fd=GetFDFromName(command[1]);
                if(fd== -1)
                {
                    printf("Error: incorrect parameter");
                    continue;
                }
                printf("Enter the data : \n");
                scanf("%[^\n]s",arr);
                ret=strlen(arr);
                if(ret == 0)
                {
                    printf("Error : Incorrect parameter");
                    continue;
                }
                ret=WriteFile(fd,arr,ret);
                if(ret== -1)
                printf("ERROR: permission denied \n");
                if(ret == -2)
                printf("ERROR : There is np Sufficient memory to write");
                if(ret == -3)
                printf("ERROR : It is not regular File\n");
            }
            else if(strcmp(command[0],"truncate")== 0)
            {
            ret = truncate_File(command[1]);
            if(ret == -1)
            printf("ERROR : Incorrect parameter");
            }
            else
            {
            printf("\n ERROR : Command not Found!!!\n");
            }
        }
        else if(count== 3)
        {
            if(strcmp(command[0],"create")== 0)
            {
                ret = CreateFile(command[1],atoi(command[2]));      // atoi() is in-built function which means ASCII to Integer
                if(ret>=0)
                printf("File is successfully created with file descriptor:%d\n",ret);
                if(ret == -1)
                printf("ERROR : Incorrect Parameter \n");
                if(ret == -2)
                printf("ERROR : there is no inodes \n");
                if(ret == -3)
                printf("ERROR : File already exit \n");
                if(ret == -4)
                printf("ERROR : Memory Allocation failure\n");
                continue;
            }
            else if(strcmp(command[0],"open") == 0)
            {
                ret=OpenFile(command[1],atoi(command[2]));
                if(ret>0)
                printf("File is successfully opened with file descriptor :%d\n",ret);
                if(ret == -1)
                printf("ERROR : Incorrect Parameter\n");
                if(ret == -2)
                printf("ERROR : file not present\n");
                if(ret == -3)
                printf("ERROR : Permission denied \n");
                continue;
            }
            else if(strcmp(command[0],"read")==0)
            {
                fd=GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("ERROR : Incorrect Parameter");
                    continue;
                }
                ptr=(char*)malloc(sizeof(atoi(command[2]))+1);
                if(ptr == NULL)
                {
                    printf("ERROR : Memory Allocation failure \n");
                    continue;
                }
                ret = ReadFile(fd,ptr,atoi(command[2]));
                if(ret == -1)
                printf("ERROR : File not existing\n");
                if(ret == -2)
                printf("ERROR : Permission denied \n");
                if(ret == -3)
                printf("ERROR : Reached at end of file\n");
                if(ret == -4)
                printf("ERROR : It is not regulgar file \n");
                if(ret == 0)
                printf("ERROR : File Empty\n");
                if(ret>0)
                {
                    write(2,ptr,ret);
                }
                continue;
            }
            else
            {
            printf("\n ERROR : Command not found!!!\n");
            continue;
            }
        }
        else if(count == 4)
        {
            if(strcmp(command[0],"lseek")==0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameter");
                    continue;
                }
                ret = LseekFile(fd,atoi(command[2]),atoi(command[3]));
                if(ret == -1)
                {
                    printf("ERROR : Unable to perform lseek\n");
                }
            }
            else
            {
                printf("\nERROR : Command not found!!!\n");
                continue;
            }
        }
        else
        {
            printf("\nERROR : Command not Found \n");
            continue;
        }
    }// end of while
    return 0;
}

/* Screenshot of Output :

 help And create :
 write And Read :
 ls :
 Stat :

 Explaination of use of below command :
1. help :
help command which is used to show all the commands that are used in this project.

2.man :
man command is used to format and display the man pages.
 
3.create :
This command is used to create new file.

4.write :
This command is used to write the contents of any regular file .

5.read :
Which is used to read the contents of file.

6.rm :
The rm command is used to remove or delete files.

7.stat :
stat is a Unix system call that returns the attributes about an inode. The semantics of stat vary between operating system.

8.ls :
ls which shows all details of file i.e filename, inode number, file size and list count.

9.ls-l :
ls-l (-l is character not one) shows file or directory, size, or folder name and its permission.

10.ls –a:
List all files including hidden file starting with ‘.’.

11.mkdir :
The mkdir command in the Unix, DOS, Microsoft Windows and ReactOS operating system is used to make a new directory.

12.lseek :
This function acc


IMP NOTE :
1) Userdefined macro and typedef are written using Capital Letters
2) How much maximum memory is to be allocated at a time is decided by the memory manager of a particular OS, so it varies with the OS.
3) Inside Inode file name is not there in actual OS, but in this project we have written the filename inside inode for simplicity as we have not implemented/used directory inside which filename is there.

*/