#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define RAM_SIZE 1048576
#define RAM_PAGE_NUMBER 256
#define PAGE_SIZE 4096
#define MAX_PAGE_NUMBER 1048576
#define MAX_PROCESS_SIZE 100
#define MAX_FRAGMENT_NUMBER 100
#define STORAGE_PROGRAMS "tasks.txt"
#define DUMMY_DATA 1
#define TIME_QUANTUM 5

typedef int* PageTable;

typedef struct
{
    int PageNumber;
    int IndexInPage;
}LogicalAdress;

typedef struct
{
    int FrameNumber;
    int IndexInFrame;
}PhysicalAdress;

typedef enum 
{
    New,
    Ready,
    Running,
    Wating,
    Terminated   
}ProcessStates;

typedef struct
{
    int Program_Reference;
    int Program_Size;
}NewProcessBlock;

typedef struct 
{
    int PID;
    ProcessStates Process_States;
    int Program_Counter;
    int Data_Logical_Adress_Base;
    int Data_Logical_Adress_Limit;
    int Text_Segment_Base;
    int Text_Segment_Limit;
    PageTable* PageTable_Pointer;
    int PageTable_Size;
    int TLB_Hit;
    int TLB_Miss;
    int Dispatch_Count;
}ProcessControlBlock;

typedef struct 
{
    int FreeSpaceBase;
    int FreeSpaceLimit;
    int FreePageLength;
}FreeSpaceBlock;

typedef struct 
{
    ProcessControlBlock Pcb;
}Context;

uint32_t* RAM_Instruction_Memory;
uint8_t* RAM_Data_Memory;

PageTable* CPU_Data_TLB;
Context CPU_Context;

NewProcessBlock* OS_NewQueue;
int* OS_ReadyQueue;
int* OS_TerminationQueue;
PageTable* OS_Data_PageTable_List;
ProcessControlBlock* OS_PCB_List;
FreeSpaceBlock* OS_FSB_List;


int OS_NewQueue_Head = 0;
int OS_NewQueue_Tail = 0;
int OS_NewQueue_Size = 0;
int OS_ReadyQueue_Head = 0;
int OS_ReadyQueue_Tail = 0;
int OS_ReadyQueue_Size = 0;
int OS_TerminationQueue_Head = 0;
int OS_TerminationQueue_Tail = 0;
int OS_TerminationQueue_Size = 0;
int PID_Index = 0;
int OS_FSB_List_Index = 0;
int RAM_Instruction_Memory_Index = 0;
int CPU_Timer = 0;

// Utils - File Operation
bool Read_Line_From_File(int lineIndex, char* line, char *fileName)
{   
    bool result = true;
    FILE *fp = fopen(fileName, "r");

    if (fp == NULL)
    {
        printf("Read_Line_From_File: Error: could not open file %s \n", fileName);
        result = false;
    }
    else
    {
        int counter = 0;

        while (counter <= lineIndex)
        {
            fgets(line, 1000, fp);
            counter++;
        }
    }

    fclose(fp);   
    return result;
}

bool Read_Word_From_Line(int wordIndex, char* word, char* line)
{
    bool result = false;
    int i = 0;
    int j = 0;

    while (line[i] != '\0' && wordIndex >= 0)
    {
        if (wordIndex > 0 && line[i] == ' ' )
        {
            wordIndex--;
        }
        else if (wordIndex == 0 && line[i] != ' ' )
        {
            word[j] = line[i];
            j++;
            result = true;
        }
        else if (line[i] == ' ')
        {
            break;
        }

        i++;
    }

    return result;
}

int Get_Length_Of_File(char *fileName)
{
    FILE* fp = fopen(fileName, "r");

    int ch, lineCount = 0;

    if (fp == NULL)
    {
        printf("Get_Length_Of_File: Error: could not open file %s \n", fileName);
    }
    else
    {
        do
        {
            ch = fgetc(fp);
            if(ch == '\n')
                lineCount++;
        }
        while (ch != EOF);

        if(ch != '\n' && lineCount != 0)
            lineCount++;
    }

    fclose(fp);

    return lineCount;
}

// Utils
LogicalAdress GetLogicalAdressStruct(int address)
{
    LogicalAdress logicalAdress;
    logicalAdress.PageNumber = (4294963200 & address) >> 12;
    logicalAdress.IndexInPage = (address & 4095);

    return logicalAdress;
}

//Memory
void Memory_Initialize()
{
    RAM_Data_Memory = malloc(sizeof(uint8_t)*RAM_SIZE);
    RAM_Instruction_Memory = malloc(sizeof(uint32_t)*(RAM_SIZE/4));
}

// CPU
void CPU_Initialize()
{
    CPU_Data_TLB = malloc(sizeof(PageTable)*MAX_PAGE_NUMBER);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        CPU_Data_TLB[i] = NULL;
    }
}

PhysicalAdress CPU_Memory_Management_Unit(int loadInstructionAddressValue)
{ 
    PhysicalAdress physicalAdress;
    PageTable osPageTable = OS_Data_PageTable_List[CPU_Context.Pcb.PID];   
    PageTable cpuTLB = CPU_Data_TLB[CPU_Context.Pcb.PID];  

    LogicalAdress logicalAdress = GetLogicalAdressStruct(loadInstructionAddressValue);
    if(loadInstructionAddressValue < CPU_Context.Pcb.Data_Logical_Adress_Limit)
    {
        if(cpuTLB[logicalAdress.PageNumber] != -1)
        {
            physicalAdress.FrameNumber = cpuTLB[logicalAdress.PageNumber];
            physicalAdress.IndexInFrame = logicalAdress.IndexInPage;
            CPU_Context.Pcb.TLB_Hit++;
        }
        else
        {
            cpuTLB[logicalAdress.PageNumber] = osPageTable[logicalAdress.PageNumber];
            CPU_Context.Pcb.TLB_Miss++;
        }
    }
    else
    {
        physicalAdress.FrameNumber = -1;
        physicalAdress.IndexInFrame = -1;
        printf("PID:%d, %d adresine izinsiz erişim. Segmentation fault ama program devam etsin...\n", CPU_Context.Pcb.PID, loadInstructionAddressValue);
    }
    
    return physicalAdress;
}

void CPU_Instruction_Load()
{
    int loadInstructionAddressValue = RAM_Instruction_Memory[CPU_Context.Pcb.Text_Segment_Base + CPU_Context.Pcb.Program_Counter]; 

    PhysicalAdress physicalAdress = CPU_Memory_Management_Unit(loadInstructionAddressValue);
    //load data at physicalAdress to register
    CPU_Timer++;
}

bool CPU_Instruction_Halt()
{
    OS_TerminationQueue[OS_TerminationQueue_Head] = CPU_Context.Pcb.PID;
    OS_TerminationQueue_Head++;
    OS_TerminationQueue_Size++;

    printf("PID: %d process'i sonlandırıldı. \n", CPU_Context.Pcb.PID);

    return true;
}

void CPU_Timer_Interrupt_Handler(int pid)
{
    OS_ReadyQueue[OS_ReadyQueue_Head] = pid;
    OS_ReadyQueue_Head = (++OS_ReadyQueue_Head) % MAX_PROCESS_SIZE;
    OS_ReadyQueue_Size++;
}

bool CPU_Timer_Interrupt(int pid)
{
    bool interrupt = false;

    if(CPU_Timer >= TIME_QUANTUM)
    {
        interrupt = true;
        CPU_Timer = 0;
        CPU_Timer_Interrupt_Handler(pid);
    }

    return interrupt;
}

int CPU_Executor()
{
    int result = 1;
    bool interrupt = false;
    bool io = false;
    bool exit = false;

    while(!interrupt && !io && !exit)
    {
        CPU_Instruction_Load(CPU_Context.Pcb);  //USER Program
        CPU_Context.Pcb.Program_Counter++;

        if(CPU_Context.Pcb.Program_Counter >= CPU_Context.Pcb.Text_Segment_Limit)
        {       
            result = 0;       
            CPU_Timer = 0;
            exit = true;  
        }

        interrupt = CPU_Timer_Interrupt(CPU_Context.Pcb.PID);      
    }  

    return result;
}

// OS
void OS_Initialize()
{
    OS_NewQueue = malloc(sizeof(NewProcessBlock)*MAX_PROCESS_SIZE);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        OS_NewQueue[i].Program_Reference = -1;
    }

    OS_ReadyQueue = malloc(sizeof(int)*MAX_PROCESS_SIZE);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        OS_ReadyQueue[i] = -1;
    }

     OS_TerminationQueue = malloc(sizeof(int)*MAX_PROCESS_SIZE);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        OS_TerminationQueue[i] = -1;
    }

    OS_Data_PageTable_List = malloc(sizeof(PageTable)*MAX_PROCESS_SIZE);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        OS_Data_PageTable_List[i] = NULL;
    }

    OS_PCB_List = malloc(sizeof(ProcessControlBlock)*MAX_PROCESS_SIZE);
    for(int i = 0; i < MAX_PROCESS_SIZE; i++)
    {
        OS_PCB_List[i].PID = -1;
    }

    OS_FSB_List = malloc(sizeof(PageTable)*MAX_FRAGMENT_NUMBER);
    OS_FSB_List[0].FreePageLength = RAM_PAGE_NUMBER; 
    OS_FSB_List[0].FreeSpaceBase = 0;
    OS_FSB_List[0].FreeSpaceLimit = RAM_SIZE;
    for(int i = 1; i < MAX_PROCESS_SIZE; i++)
    {
        OS_FSB_List[i].FreeSpaceBase = -1;
    }
}

void OS_Context_Switch(int mode, int pid)
{
    if(pid != -1)
    {
        if(mode == 1)
        {
            CPU_Context.Pcb = OS_PCB_List[pid];
            
        }
        else if(mode == 2)
        {
            OS_PCB_List[pid] = CPU_Context.Pcb;
        } 
    }    
}

bool OS_Memory_Manager(NewProcessBlock npb, ProcessControlBlock* pcb)
{
    bool result = false;
    while(OS_FSB_List[OS_FSB_List_Index].FreeSpaceBase != -1 && OS_FSB_List_Index < MAX_FRAGMENT_NUMBER)
    {
        if(OS_FSB_List[OS_FSB_List_Index].FreeSpaceLimit > npb.Program_Size)
        {
            pcb->Data_Logical_Adress_Base = 0;
            pcb->Data_Logical_Adress_Limit = npb.Program_Size;
             
            int programPagesNumber = floor(npb.Program_Size/PAGE_SIZE)+1;        

            // Ram
            for(int i = 0; i < npb.Program_Size; i++)
            {
                // a.txt formati verilmedigi icin Dummy data kullanildi.
                RAM_Data_Memory[OS_FSB_List[OS_FSB_List_Index].FreeSpaceBase + i] = DUMMY_DATA;            
            }

            // Page Table
            OS_Data_PageTable_List[PID_Index] = malloc(sizeof(int)*programPagesNumber);
            CPU_Data_TLB[PID_Index] = malloc(sizeof(int)*(programPagesNumber+1));
            CPU_Data_TLB[PID_Index][programPagesNumber] = -2;

            for(int i = 0; i < programPagesNumber; i++)
            {              
                OS_Data_PageTable_List[PID_Index][i] = OS_FSB_List[OS_FSB_List_Index].FreeSpaceBase + i*PAGE_SIZE;     
                CPU_Data_TLB[PID_Index][i] = -1;
            }

            printf("PID: %d nolu processin TLB'si : \n",PID_Index);
            for(int i = 0; i < programPagesNumber; i++)
            {                 
                printf("\t %d. Page'in fiziksel memory adresi: %d \n",i, OS_Data_PageTable_List[PID_Index][i] );              
            }

            OS_FSB_List[OS_FSB_List_Index].FreePageLength -= programPagesNumber;
            OS_FSB_List[OS_FSB_List_Index].FreeSpaceBase += programPagesNumber*PAGE_SIZE;
            OS_FSB_List[OS_FSB_List_Index].FreeSpaceLimit -= programPagesNumber*PAGE_SIZE;

            pcb->PageTable_Pointer = &OS_Data_PageTable_List[PID_Index];
            pcb->PageTable_Size = programPagesNumber;
            pcb->TLB_Hit = 0;
            pcb->TLB_Miss = 0;
            pcb->Dispatch_Count = 0;

            // Text Segment
            char code[1000] = "";
            char c_instruction[10] = "";
            uint32_t i_instruction;
            if(Read_Line_From_File(PID_Index, code, STORAGE_PROGRAMS))
            {
                pcb->Text_Segment_Base = RAM_Instruction_Memory_Index;
    
                for(int i = 1; i<1000; i++)
                {
                    strcpy(c_instruction,"");
                    if(Read_Word_From_Line(i, c_instruction, code))
                    {
                        i_instruction = atoi(c_instruction);

                        if(i_instruction < 4294967296)
                        {
                            RAM_Instruction_Memory[RAM_Instruction_Memory_Index] = i_instruction;
                            RAM_Instruction_Memory_Index++;
                        } 
                    }
                    else
                    {                       
                        pcb->Text_Segment_Limit = --i; 
                        break;    
                    }
                }                
            }
          
            result = true;
            break;
        }
    }

    return result;
}

void OS_Process_Admitter()
{
    while(OS_NewQueue_Size > 0) 
    {
        if (OS_NewQueue[OS_NewQueue_Tail].Program_Reference != -1)
        {
            if(PID_Index < MAX_PROCESS_SIZE)
            {
                ProcessControlBlock pcb;

                if(OS_Memory_Manager(OS_NewQueue[OS_NewQueue_Tail], &pcb))
                {
                    pcb.PID = PID_Index;
                    pcb.Process_States = Ready;
                    pcb.Program_Counter = 0;
                    OS_PCB_List[PID_Index] = pcb;
                    OS_ReadyQueue[OS_ReadyQueue_Head] = PID_Index;
                    OS_ReadyQueue_Head = (++OS_ReadyQueue_Head) % MAX_PROCESS_SIZE;
                    OS_ReadyQueue_Size++;
                    PID_Index++;
                }    
            }               
        }

        OS_NewQueue_Tail++;
        OS_NewQueue_Size--;
    }
}

int OS_Process_Dispatcher()
{
    int pid = -1;

    if(OS_ReadyQueue_Size > 0)
    {
        pid = OS_ReadyQueue[OS_ReadyQueue_Tail];
        OS_PCB_List[pid].Dispatch_Count++;
        OS_ReadyQueue[OS_ReadyQueue_Tail] = -1;
        OS_ReadyQueue_Tail = (++OS_ReadyQueue_Tail) % MAX_PROCESS_SIZE;
        OS_ReadyQueue_Size--;       
    }

    return pid;
}

void OS_Process_Terminator()
{ 
    while(OS_TerminationQueue_Size > 0)
    {
        int PID = OS_PCB_List[OS_TerminationQueue[OS_TerminationQueue_Tail]].PID;
        int TLB_Hit = OS_PCB_List[OS_TerminationQueue[OS_TerminationQueue_Tail]].TLB_Hit;
        int TLB_Miss = OS_PCB_List[OS_TerminationQueue[OS_TerminationQueue_Tail]].TLB_Miss;
        int Dispatch_Count = OS_PCB_List[OS_TerminationQueue[OS_TerminationQueue_Tail]].Dispatch_Count;
        OS_TerminationQueue_Tail++; 
        OS_TerminationQueue_Size--;

        printf("PID: %d, TLB Hit: %d, TLB Miss: %d, Dispatch Count: %d \n",PID, TLB_Hit, TLB_Miss, Dispatch_Count); 
    }
}

// User
bool USER_Program_Start(int ProgramReference)
{
    bool result = false;
    char line[1000] = "";
    char heapMemory[100] = "";

    if(Read_Line_From_File(ProgramReference, line, STORAGE_PROGRAMS))
    {
        if(Read_Word_From_Line(0, heapMemory, line))
        {
            printf("%s process'i kontrol edilmek için New Queue'ya alındı. PID: %d olacak. \n", heapMemory, ProgramReference);
            OS_NewQueue[OS_NewQueue_Head].Program_Reference = ProgramReference;
            OS_NewQueue[OS_NewQueue_Head].Program_Size = Get_Length_Of_File(heapMemory);
            OS_NewQueue_Head++;
            OS_NewQueue_Size++;
            result = true;
        }      
    }
   
    return result;
}

void USER_Program_Initialize()
{
    int programCount = Get_Length_Of_File(STORAGE_PROGRAMS);

    for(int i = 0; i<programCount; i++)
    {
        USER_Program_Start(i);      
    }
}

int main (int argc, char **argv)
{
    printf("OS Başlatılıyor \n");

    Memory_Initialize();
    CPU_Initialize();
    OS_Initialize();
    USER_Program_Initialize();

    int pid = -1;
    int oldPid = -1;
    int cpuResult;
   
    while(true)
    {
        // OS Simulation
        OS_Process_Admitter();
        pid = OS_Process_Dispatcher();
        OS_Context_Switch(1,pid);

        // CPU Simulation
        if(pid != -1)
        {
            cpuResult = CPU_Executor();
        }
        else
        {
            break;
        }
        
        if(cpuResult == 0)
        {
            // User Simulation
            CPU_Instruction_Halt();  //USER Program       

            // OS Simulation
            OS_Context_Switch(2,pid);
            pid = -1;
            cpuResult = 1;
        }        

        OS_Context_Switch(2,pid);
    }

    OS_Process_Terminator();
    
    printf("Mavi Ekran \n");
    return 0;
}
