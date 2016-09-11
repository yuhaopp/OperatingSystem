
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#define RANDOM_MAX 0x7FFFFFFF
 
  
 
static long my_do_rand(unsigned long *value)
 
{
 
   
 
/*      这个算法保证所产生的值不会超过(2^31 - 1)
 
这里(2^31 - 1)就是 0x7FFFFFFF。而 0x7FFFFFFF
 
      等于127773 * (7^5) + 2836,7^5 = 16807。
 
      整个算法是通过：t = (7^5 * t) mod (2^31 - 1)
 
      这个公式来计算随机值，并且把这次得到的值，作为
 
下次计算的随机种子值。
 
   */
 
   long quotient, remainder, t;
 
  
 
   quotient = *value / 127773L;
 
   remainder = *value % 127773L;
 
   t = 16807L * remainder - 2836L * quotient;
 
  
 
   if (t <= 0)
 
      t += 0x7FFFFFFFL;
 
   return ((*value = t) % ((unsigned long)RANDOM_MAX + 1));
 
}
 
static unsigned long next = 1;
 
int my_rand(void)
 
{
 
   return my_do_rand(&next);
 
}
 
  
 
void my_srand(unsigned int seed)
 
{
 
   next = seed;
 
}
 

/*======================================================================*
                                   计算器
 *======================================================================*/
#define FALSE 0
#define TRUE 1
#define BOOL int
#define NULL 0


typedef struct{
unsigned char is_available;         /* whether blcok is avaiable */
unsigned int prior_blocksize;       /* size of prior block */
unsigned int current_blocksize;     /* block size */
}mem_control_block;

static unsigned char has_initialized;
static unsigned int managed_memory_start;
static unsigned int managed_memory_end;
static unsigned int managed_memory_size;

/* heap buffer size */
#define HEAPSIZE	500

/* heap start addrress */
#define malloc_addr heap
/* heap size */ 
#define malloc_size HEAPSIZE

/* heap buffer */
unsigned char heap[HEAPSIZE]; 
/**
  * @brief Initializes malloc's gloabl varialbe and head of heap memory    
  * @param None 
  * @retval None
  */
void malloc_init(void)
{
    mem_control_block * mcb = NULL;
    
    /* confirm heap's start address */
    managed_memory_start = (unsigned int)malloc_addr;
    /* confirm heap's size */
    managed_memory_size = malloc_size;
    /*confirm heap's end address */
    managed_memory_end = managed_memory_start + managed_memory_size;
    
    /* make mcb point to heap's start address */ 
    mcb = (mem_control_block *)managed_memory_start;
    /*the first blcok is avaialbe */
    mcb->is_available = 1;
    /*there is no block before the first block */
    mcb->prior_blocksize = 0;
    /*the first block's block size is difference of between heap's size and control block */ 
    mcb->current_blocksize = managed_memory_size - sizeof(mem_control_block);
    /* Initialize done */
    has_initialized = 1;
}


/**
  * @brief Dynamic distribute memory function
  * @param numbytes: what size you need   
  * @retval a void pointer to the distribute first address
  */ 
void * malloc(unsigned int numbytes)
{
    unsigned int current_location,otherbck_location;
    /* This is the same as current_location, but cast to a memory_control_block */
    mem_control_block * current_location_mcb = NULL,* other_location_mcb = NULL;
    /* varialbe for saving return value and be set to 0 until we find something suitable */
    void * memory_location = NULL;
    /* current dividing block size */
    unsigned int process_blocksize;
    
    /* Initialize if we haven't already done so */
    if(! has_initialized) {
        malloc_init();
    }
    
    /* Begin searching at the start of managed memory */
    current_location = managed_memory_start;
    /* Keep going until we have searched all allocated space */
    while(current_location != managed_memory_end){
        /* current_location and current_location_mcb point to the same address.  However, 
         * current_location_mcb is of the correct type, so we can use it as a struct. current_location 
         * is a void pointer so we can use it to calculate addresses.
         */
        current_location_mcb = (mem_control_block *)current_location;
        /* judge whether current block is avaiable */
        if(current_location_mcb->is_available){
            /* judge whether current block size exactly fit for the need */
            if((current_location_mcb->current_blocksize == numbytes)){
                /* It is no longer available */ 
                current_location_mcb->is_available = 0;            
                /* We own it */
                memory_location = (void *)(current_location + sizeof(mem_control_block));
                /* Leave the loop */
                break;
            /* judge whether current block size is enough for dividing a new block */
            }else if(current_location_mcb->current_blocksize >= numbytes + sizeof(mem_control_block)){
                /* It is no longer available */ 
                current_location_mcb->is_available = 0;
                /* because we will divide current blcok,before we changed current block size,we should
                 * save the integral size.
                 */
                process_blocksize = current_location_mcb->current_blocksize;
                /* Now blcok size could be changed */
                current_location_mcb->current_blocksize = numbytes;
                
                /* find the memory_control_block's head of remaining block and set parameter,block of no
                 * parameter can't be managed. 
                 */
                other_location_mcb = (mem_control_block *)(current_location + numbytes \
                                                + sizeof(mem_control_block));
                /* the remaining block is still avaiable */
                other_location_mcb->is_available = 1;
                /* of course,its prior block size is numbytes */
                other_location_mcb->prior_blocksize = numbytes;
                /* its size should get small */
                other_location_mcb->current_blocksize = process_blocksize - numbytes \
                                                - sizeof(mem_control_block);
                
                /* find the memory_control_block's head of block after current block and \
                 * set parameter--prior_blocksize. 
                 */
                otherbck_location = current_location + process_blocksize \
                                            + sizeof(mem_control_block);                
                /* We need check wehter this block is on the edge of managed memeory! */
                if(otherbck_location != managed_memory_end){
                    other_location_mcb = (mem_control_block *)(otherbck_location);
                    /*  its prior block size has changed! */
                    other_location_mcb->prior_blocksize = process_blocksize\
                        - numbytes - sizeof(mem_control_block);
                }
                /*We own the occupied block ,not remaining block */ 
                memory_location = (void *)(current_location + sizeof(mem_control_block));
                /* Leave the loop */
                break;
            } 
        }
        /* current block is unavaiable or block size is too small and move to next block*/
        current_location += current_location_mcb->current_blocksize \
                                    + sizeof(mem_control_block);
    }
    /* if we still don't have a valid location,we'll have to return NULL */
    if(memory_location == NULL)    {
        return NULL;
    }
    /* return the pointer */
    return memory_location;    
}


/**
  * @brief  free your unused block 
  * @param  firstbyte: a pointer to first address of your unused block
  * @retval None
  */ 
void free(void *firstbyte) 
{
    unsigned int current_location,otherbck_location;
    mem_control_block * current_mcb = NULL,* next_mcb = NULL,* prior_mcb \
                                = NULL,* other_mcb = NULL;
    /* Backup from the given pointer to find the current block */
    current_location = (unsigned int)firstbyte - sizeof(mem_control_block);
    current_mcb = (mem_control_block *)current_location;
    /* Mark the block as being avaiable */
    current_mcb->is_available = 1;
    
    /* find next block location */
    otherbck_location = current_location + sizeof(mem_control_block) \
                                    + current_mcb->current_blocksize;
    /* We need check wehter this block is on the edge of managed memeory! */
    if(otherbck_location != managed_memory_end){
        /* point to next block */
        next_mcb = (mem_control_block *)otherbck_location;
        /* We need check whether its next block is avaiable */ 
        if(next_mcb->is_available){
            /* Because its next block is also avaiable,we should merge blocks */
            current_mcb->current_blocksize = current_mcb->current_blocksize \
                + sizeof(mem_control_block) + next_mcb->current_blocksize;
            
            /* We have merge two blocks,so we need change prior_blocksize of
             * block after the two blocks,just find next block location. 
             */
            otherbck_location = current_location + sizeof(mem_control_block) \
                                    + current_mcb->current_blocksize;
            /* We need check wehter this block is on the edge of managed memeory! */
            if(otherbck_location != managed_memory_end){
                other_mcb = (mem_control_block *)otherbck_location;
                /*  its prior block size has changed! */
                other_mcb->prior_blocksize = current_mcb->current_blocksize;
            }
        }
    }
    
    /* We need check wehter this block is on the edge of managed memeory! */
    if(current_location != managed_memory_start){
        /* point to prior block */
        prior_mcb = (mem_control_block *)(current_location - sizeof(mem_control_block)\
                                            - current_mcb->prior_blocksize);
        /* We need check whether its prior block is avaiable */ 
        if(prior_mcb->is_available){
            /* Because its prior block is also avaiable,we should merge blocks */
            prior_mcb->current_blocksize = prior_mcb->current_blocksize \
                + sizeof(mem_control_block) + current_mcb->current_blocksize;
            
            /* We have merge two blocks,so we need change prior_blocksize of
             * block after the two blocks,just find next block location. 
             */
            otherbck_location = current_location + sizeof(mem_control_block) \
                                    + current_mcb->current_blocksize;
            /* We need check wehter this block is on the edge of managed memeory! */
            if(otherbck_location != managed_memory_end){
                other_mcb = (mem_control_block *)otherbck_location;
                /*  its prior block size has changed! */
                other_mcb->prior_blocksize = prior_mcb->current_blocksize;
            }
        }
    }
}




void *realloc(void *ptr, int size)
{
    void *ptr1 = malloc(size);
    memcpy(ptr1, ptr, size);
    free(ptr);
    return ptr1;
}


void displaymenu(int fd_stdin)
{
	printf("*****************************\n");
	printf("* 1.add    *\n");
	printf("* 2.sub    *\n");
	printf("* 3.multi  *\n");
	printf("* 4.dev    *\n");
	printf("* 5.mode   *\n");
	printf("* 6.fac    *\n");
	printf("* 7.mulsum *\n");
	printf("* 8.end    *\n");
	printf("*****************************\n");

	int flag = 1;
	while(flag==1)
	{
		printf("chose<1,2,3,4,5,6,7,8>?\n");
		char* t;
		read(fd_stdin,t,2);
		t[1]=0;
		switch(t[0])
		{ 
		case 49:
			{int i=0,j=0,add=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			add=i+j;
			printf("add=%d\n",add);
			};break;
		case 50:
			{int i=0,j=0,sub=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			sub=i-j;
			printf("sub=%d\n",sub);
			};break;
		case 51:
			{int i=0,j=0,multi=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			multi=i*j;
			printf("multi=%d\n",multi);
			};break;
		case 52:
			{int i=0,j=0;
			int divide=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			if(j==0)
			printf("error!\n");
			else{
				divide=i/j;
				printf("divide=%d\n",divide);
				}
			};break;
		case 53:
			{int i=0,j=0,arithcompliment=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			arithcompliment=i%j;
			printf("arith-compliment=%d\n",arithcompliment);
			};break;
		case 54:
			{int i,j;
			int fac=1;
			char *a;
			read(fd_stdin,a,2);
			i=a[0]-48;
			for(j=1;j<=i;j++)
			fac=fac*j;
			printf("\n");
			printf("fac=%d\n",fac);
			};break;
		case 55:
			{int i,j,sum_N=0;
			char *a,*b;
			read(fd_stdin,a,2);
			i=a[0]-48;
			read(fd_stdin,b,2);
			j=b[0]-48;
			int k;
			for(k=0;k<j;k++)
			sum_N=sum_N+i;
			printf("\n");
			printf("sum_N=%d\n",sum_N);
			};break;
		case 56:
			{flag=0;
			};break;
		}
	}
}

int jisuanqi(int fd_stdin, int fd_stdout)
{
	displaymenu(fd_stdin);
        return 0;
}


/*======================================================================*
                                   日历
 *======================================================================*/
/*得到所输入年月的第一天是星期几0~6*/
int getdate(int y,int m);
 
/*判断所输入的是否是闰月,是则返回1,否则返回0*/
int leap(int y);
 
/*打印输入月份月历表*/
void print(int y,int m);
 
int rili(int fd_stdin, int fd_stdout)
{
    char year[5];
    char month[3];
    int y,m;
    int i,j;
    printf("input year(yyyy):");
    read(fd_stdin,year,5);
    year[4]=0;
    printf("input month(mm):");
    read(fd_stdin,month,3);
    month[2]=0;
    y=(year[0]-48)*1000+(year[1]-48)*100+(year[2]-48)*10+year[3]-48;
    m=(month[0]-48)*10+month[1]-48;
    print(y,m);
    return 0;
}
 
int getdate(int y,int m)
{
    int w=(y+(y-1)/4-(y-1)/100+(y-1)/400)%7;
    int days=0;
    switch(m)
    {
        case 12: days+=30;
        case 11: days+=31;
        case 10: days+=30;
        case 9:  days+=31;
        case 8:  days+=31;
        case 7:  days+=30;
        case 6:  days+=31;
        case 5:  days+=30;
        case 4:  days+=31;
        case 3:  if(leap(y)) days+=29;
                 else days+=28;
        case 2:  days+=31;
        case 1:  days+=0;
        }
    w=(w+days)%7;
    return w;     /*返回输入月份1号的星期*/
}
 
void print(int y,int m)
{
    int w=getdate(y,m);
    printf("%dyear%dmonth:\n\n",y,m);
    printf("\n==========================="
            "========================\n\n");
    int month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(leap(y)) month[1]=29;
 
    printf("SUN    MON    TUE    WED    THU    FRI    SAT\n");
    int i,j;
    for(i=0;i<w;i++) printf("       ");
 
    for(i=w,j=1;j<=month[m-1];i++,j++)
    {
        if(i%7==0) printf("\n");
        if(j<10)printf("%d      ",j);
        else printf("%d     ",j);
        }
    printf("\n\n========================"
           "===========================\n\n");
    }
 
int leap(int y)
{
    if((y%4==0&&y%100!=0)||y%400==0) return 1;
    return 0;
    }
  
/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
        u8    privilege;
        u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        /* proc_table[NR_TASKS + 0].nr_tty = 0; */
        /* proc_table[NR_TASKS + 1].nr_tty = 1; */
        /* proc_table[NR_TASKS + 2].nr_tty = 1; */

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}




/*****************************************************************************
 *                                五子棋
 *****************************************************************************/

//棋盘初始化函数Chessboard棋盘数组，ln=棋盘大小,成功返回Chessboard，不成功NULL
void init_Chessboard(char Chessboard[][7], int ln)
{
	if ((Chessboard != 0) && (ln>0)){
		int i = 0, j = 0;
		for (i = 0; i<ln; ++i){
			for (j = 0; j<ln; ++j){
				Chessboard[i][j] = 48;
			}
		}
		return Chessboard;
	}
	return 0;
}


//显示棋盘函数
void show_Chessboard(char Chessboard[][7], int ln)
{
	if((Chessboard != 0) && (ln > 0)){
	int i = 0, j = 0;
        printf(" ");
	for (i = 0; i<ln; ++i){
		printf("%d",i);
	}//end for1
	printf("\n");

	for (i = 0; i<ln; ++i){
		printf("%d",i);
		for (j = 0; j<ln; ++j){
		    printf("%c",Chessboard[i][j]);
		}
		printf("\n");
	}//end for2
    }
}

//棋子下子函数
//下子成功返回1，不成功返回0
int play(char Chessboard[][7], int ln, int x, int y, char ChessPieces)
{
	if ((x<ln) && (y<ln) && (x >= 0) && (y >= 0)){
		if (48 == Chessboard[x][y]){

			Chessboard[x][y] = ChessPieces;
                        printf("%d %d %c\n",x,y,Chessboard[x][y]);
			return 1;  //成功
		}
		else
		{
			return 0;
		}//end if2
	}//end if1
	return 0;
}

//满盘判断
//棋盘满了判断//满了就返回-1
int full_Chess(char Chessboard[][7], int ln)
{
	int i = 0, j = 0;
	for (i = 0; i<ln; ++i){
		for (j = 0; j<ln; ++j){
			if (48 == Chessboard[i][j]){
				return 0;  //棋盘未满
			}//end if
		}//end for j
	}//end for i

	return 1;//棋盘满了

}


//是否连成五子线判断函数
//Chessboard=棋盘数组,ln=棋盘宽度,(XS,YS)方向判断起点坐标,(dx,dy)方向增量标记
//连成线返回1，没有返回0
int judga_line(char Chessboard[][7], int ln, int XS, int YS, int dx, int dy)
{
	if ((XS <ln) && (YS<ln)  //起点坐标在棋盘内
		&& (XS >= 0) && (YS >= 0)
		&& (dx != 0 || dy != 0))        //坐标增量不为同时0
	{

		if (((XS + dx * 4) > ln) || ((XS + dx * 4)<0) || //判断终点坐标
			((YS + dy * 4)>ln) || ((YS + dy * 4) < 0) || //在棋盘外
			(48 == Chessboard[XS][YS]))
		{
			return 0;  //不在棋盘内，或者起点是没下子
		}
		else
		{
			int i = 0;
			for (i = 1; i < 5; ++i){
				if (Chessboard[XS][YS] != Chessboard[XS + (dx * i)][YS + (dy * i)])
				{
					return 0;  //如果不是连续5个一样的
				}//end if3
			}//end for1
			return 1;  //五个都一样，且都在棋盘内
		}//end if 2
	}
	return 0;  //其他情况
}

//裁判函数
//Chessboard 棋盘数组，ln=棋盘宽度
//赢了返回1，否则返回0
int judga(char Chessboard[][7], int ln)
{
	int i = 0, j = 0;
	//纵向成五子连线判断
	for (i = 0; i<(ln - 4); ++i){
		for (j = 0; j<ln; ++j){
			if (judga_line(Chessboard, ln, i, j, 1, 0)){
				return 1;
			}
		}//end for_j
	}//end for_i

	//横向成五子连线判断
	for (i = 0; i<ln; ++i){
		for (j = 0; j<(ln - 4); ++j){
			if (judga_line(Chessboard, ln, i, j, 0, 1)){
				return 1;
			}
		}//end for_j
	}//end for_i

	//左上到右下成五子连线判断
	for (i = 0; i<(ln - 4); ++i){
		for (j = 0; j<(ln - 4); ++j){
			if (judga_line(Chessboard, ln, i, j, 1, 1)){
				return 1;
			}
		}//end for_j
	}//end for_i

	//左下到右上成五子连线判断
	for (i = ln - 1; i>(ln - 4); --i){
		for (j = 0; j <(ln - 4); ++j){
			if (judga_line(Chessboard, ln, i, j, -1, 1)){
				return 1;
			}
		}//end for_j
	}//end for_i

	return 0;  //没能赢
}


//主函数


int wuziqi(int fd_stdin, int fd_stdout)
{
	char CB[7][7];
	char nameA[50] = "playerA";
	char nameB[50] = "playerB";
        int x = -1;  	
        int y = -1;
        char x1[2] = "a";
        char y1[2] = "b";
	//初始化
	init_Chessboard(CB, 7);
        
	printf("input playerA's name:");
	int n = read(fd_stdin, nameA, 50);
        nameA[n] = 0;
	printf("input playerB's name:");
	int m = read(fd_stdin, nameB, 50);
        nameB[m] = 0;
	//显示棋盘
	show_Chessboard(CB, 7);

	while (1){
		//判断是否棋盘已满
		if (full_Chess(CB, 7)){
			printf("\nThe board is full!");
			break; //跳出最外层while
		}//end if


		//玩家Ａ下子
		while (1){
			printf("\nIt turns to %s @\n", nameA);
			printf("the row x = ");
			int a = read(fd_stdin, x1, 2);
                        x1[a] = 0;
			printf("the col y = ");
			int b = read(fd_stdin, y1, 2);
                        y1[b] = 0;
                        if(strcmp(x1,"0")==0){x=0;}
                        else if(strcmp(x1,"1")==0){x=1;}
                        else if(strcmp(x1,"2")==0){x=2;}
                        else if(strcmp(x1,"3")==0){x=3;}
                        else if(strcmp(x1,"4")==0){x=4;}
                        else if(strcmp(x1,"5")==0){x=5;}
                        else if(strcmp(x1,"6")==0){x=6;}
                        if(strcmp(y1,"0")==0){y=0;}
                        else if(strcmp(y1,"1")==0){y=1;}
                        else if(strcmp(y1,"2")==0){y=2;}
                        else if(strcmp(y1,"3")==0){y=3;}
                        else if(strcmp(y1,"4")==0){y=4;}
                        else if(strcmp(y1,"5")==0){y=5;}
                        else if(strcmp(y1,"6")==0){y=6;}
			if (play(CB, 7, x, y, 64)){  //@ ascii=64
				break;   //下子成功
			}
			else
			{
				printf("try again!");
				continue;  //下子不成功，重新下子
			}//end if
		}//end while A

		//显示棋盘
		show_Chessboard(CB, 7);

		//判断玩家A是否胜利
		if (judga(CB, 7)==1){
			printf("\n %s win!\n", nameA);
			break; //跳出最外层while
		}//不用下了

		//玩家B下子
		while (1){
			printf("\nit turns to %s O\n", nameB);
			printf("the row x = ");
			int c = read(fd_stdin, x1, 2);
                        x1[c] = 0;
			printf("the col y = ");
			int d = read(fd_stdin, y1, 2);
                        y1[d] = 0;
                        if(strcmp(x1,"0")==0){x=0;}
                        else if(strcmp(x1,"1")==0){x=1;}
                        else if(strcmp(x1,"2")==0){x=2;}
                        else if(strcmp(x1,"3")==0){x=3;}
                        else if(strcmp(x1,"4")==0){x=4;}
                        else if(strcmp(x1,"5")==0){x=5;}
                        else if(strcmp(x1,"6")==0){x=6;}
                        if(strcmp(y1,"0")==0){y=0;}
                        else if(strcmp(y1,"1")==0){y=1;}
                        else if(strcmp(y1,"2")==0){y=2;}
                        else if(strcmp(y1,"3")==0){y=3;}
                        else if(strcmp(y1,"4")==0){y=4;}
                        else if(strcmp(y1,"5")==0){y=5;}
                        else if(strcmp(y1,"6")==0){y=6;}
			if ((play(CB, 7, x, y, 79))){ //O ascii=79
				break;   //下子成功
			}
			else
			{
				printf("try again!");
				continue;  //下子不成功，重新下子
			}//end if

		}//end while B

		//显示棋盘
		show_Chessboard(CB, 7);

		//判断玩家B是否胜利
		if (judga(CB, 7)==1){
			printf("\n %s win\n", nameB);
			break; //跳出最外层while
		}//不用下了

	}

	return 0;

}



/*****************************************************************************
 *                                推箱子
 *****************************************************************************/

void printMap(char map[][10], int fd_stdin, int fd_stdout);


void shoveBox(char map[][10], int fd_stdin, int fd_stdout);




int tuixiangzi(int fd_stdin, int fd_stdout)
{
char map[][10] = {
{'#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
{'#', ' ', '#', '#', '#', '#', '#', '#', '#', '#'},
{'#', ' ', ' ', '#', ' ', '#', '#', '#', '#', '#'},
{'#', '#', ' ', ' ', ' ', '#', '#', '#', '#', '#'},
{'#', '#', ' ', ' ', ' ', ' ', ' ', ' ', '#', '#'},
{'#', '#', ' ', '0', ' ', ' ', ' ', ' ', '#', '#'},
{'#', '#', ' ', 'x', ' ', ' ', ' ', ' ', '#', '#'},
{'#', '#', ' ', ' ', ' ', ' ', ' ', ' ', '#', '#'},
{'#', '#', ' ', ' ', ' ', ' ', ' ', '#', '#', '#'},
{'#', '#', '#', '#', '#', '#', ' ', '#', '#', '#'}
};

printMap(map, fd_stdin, fd_stdout);
shoveBox(map, fd_stdin, fd_stdout);


return 0;
}


void shoveBox(char map[][10],int fd_stdin,int fd_stdout)
{
int pX = 6;
int pY = 3; //人物的坐标

int bX = 5;
int bY = 3; //箱子坐标
char key = ' ';
printf("x is player,0 is box,w,s,a,d is up,down,left,right\n");
while (map[bX][bY] != map[9][6]) { //循环，如果箱子不在出口一直循环
read(fd_stdin,&key,2); //输入方向键控制人物
switch (key) {
case 'w': //往上
if (map[pX-1][pY] == '0' && map[bX-1][bY] == ' ') { //人物前面如果有箱子,并且箱子在上，箱子前面是空
map[pX][pY] = ' ';
map[bX][bY] = 'x';
map[bX-1][bY] = '0';
--pX;
--bX;
}else if (map[pX-1][pY] == ' '){ //人物前面如果是空
map[pX][pY] = ' ';
map[pX-1][pY] = 'x';
--pX;
}
break;

case 's': // 往下
if (map[pX+1][pY] == '0' && map[bX+1][bY] == ' ') { //人物前面如果有箱子
map[pX][pY] = ' ';
map[bX][bY] = 'x';
map[bX+1][bY] = '0';
++pX;
++bX;
}else if (map[pX+1][pY] == ' '){ //人物前面如果是空
map[pX][pY] = ' ';
map[pX+1][pY] = 'x';

++pX;
}
break;

case 'a': // 往左
if (map[pX][pY-1] == '0' && map[bX][bY-1] == ' ') { //人物前面如果有箱子
map[pX][pY] = ' ';
map[bX][bY] = 'x';
map[bX][bY-1] = '0';
--pY;
--bY;
}else if (map[pX][pY-1] == ' '){ //人物前面如果是空
map[pX][pY] = ' ';
map[pX][pY-1] = 'x';

--pY;
}
break;

case 'd': // 往右
if (map[pX][pY+1] == '0' && map[bX][bY+1] == ' ') { //人物前面如果有箱子
map[pX][pY] = ' ';
map[bX][bY] = 'x';
map[bX][bY+1] = '0';
++pY;
++bY;
}else if (map[pX][pY+1] == ' '){ //人物前面如果是空
map[pX][pY] = ' ';
map[pX][pY+1] = 'x';

++pY;
}
break;
}
printMap(map,fd_stdin,fd_stdout);
}
}








/**
* 打印地图
*
* @param map 地图
*/
void printMap(char map[][10], int fd_stdin, int fd_stdout)
{
int x;
int j;
for (x = 0; x <= 9 ; x++) {
for (j = 0 ; j <= 9; j++) {
printf("%c", map[x][j]);
}
printf("\n");

}
if (map[9][6] == '0') {
printf("succeed!\n");
}
}




/*****************************************************************************
 *                                猜拳
 *****************************************************************************/

int caiquan(int fd_stdin, int fd_stdout)
{
    char gamer;  // 玩家出拳
    int computer;  // 电脑出拳
    int result;  // 比赛结果
    // 为了避免玩一次游戏就退出程序，可以将代码放在循环中
    while (1){
        printf("cai quan xiao you xi:\n");
        printf("A:jian dao\nB:shi tou\nC:bu\nD:bu wan le\n");
        read(fd_stdin,&gamer,2);
        switch (gamer){
            case 65 | 97:  // A | a
                gamer=4; break;
            case 66 | 98:  // B | b
                gamer=7; break;
            case 67 | 99:  // C | c
                gamer=10; break;
            case 68 | 100:  // D | d
                return 0;
           
            default:
                printf("you chose %c, it's fault\n",gamer);
                return 0;
                break;
        }
        my_srand(get_ticks());  // 随机数种子
        computer=my_rand()%3;  // 产生随机数并取余，得到电脑出拳
        result=(int)gamer+computer;  // gamer 为 char 类型，数学运算时要强制转换类型
        printf("computer finished");
        switch (computer)
        {
            case 0:printf("jian dao\n");break; //4    1
            case 1:printf("shi tou\n");break; //7  2
            case 2:printf("bu\n");break;   //10 3
        }
        printf("you finished");
        switch (gamer)
        {
            case 4:printf("jian dao\n");break;
            case 7:printf("shi tou\n");break;
            case 10:printf("bu\n");break;
        }
        if (result==6||result==7||result==11) printf("you win\n");
        else if (result==5||result==9||result==10) printf("computer win\n");
        else printf("no winner\n");
    }
    return 0;
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
        char tty_name[] = "/dev_tty0";

	char rdbuf[128];


	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
        printf("*********************************************************\n");
	printf("* ****      ****       *****             ****      **** *\n");
	printf("*  ****    ****        *****              ****    ****  *\n");
	printf("*   ****  ****         *****               ****  ****   *\n");
	printf("*     ******           *****                 ******     *\n");
	printf("*     ******           *****                 ******     *\n");
	printf("*     ******           *****                 ******     *\n");
	printf("*     ******           *****                 ******     *\n");
	printf("*     ******           **************        ******     *\n");
	printf("*     ******           **************        ******     *\n");
	printf("*     ******           **************        ******     *\n");
	printf("*********************************************************\n");
	while(1){
            char select[100];
            char file[100];
            char ins[100];
            int k = 0;
            for(; k < 100; k++){
                select[k]=0;
                file[k]=0;
                ins[k]=0;
                }
            int n = read(fd_stdin,select,100);
            select[99] = 0;
            int j = 0;
            int i = 0;
            for(; i < 100; i++){
                if(select[i]!=' '&&select[i]!=0){ins[i]=select[i];}
                else break;
                }
            for(; i < 100; i++){
                if(select[i]==' '){;}
                else{file[j]=select[i];j++;}
                } 
            while(strcmp("exit",ins)!=0){
                 if(strcmp("create",ins)==0){
                       int fd;
                       char* text;
                       fd = open(file,O_RDWR|O_CREAT);
                       assert(fd!=-1);
                       printf("input the content: \n");
                       int n = read(fd_stdin,text,10000);
                       text[n]=0;
                       write(fd,text,10000);
                       close(fd);
                       break;
                 }
                 else if(strcmp("read",ins)==0){
                       int fd;
                       char* readtext;
                       fd = open(file,O_RDWR);
                       assert(fd!=-1);
                       printf("show the content: \n");
                       int n = read(fd,readtext,10000);
                       readtext[n]=0;
                       printf("%s\n",readtext);
                       close(fd);
                       break;
                 }
                 else if(strcmp("write",ins)==0){
                       int fd;
                       char* writetext;
                       fd = open(file,O_RDWR);
                       assert(fd!=-1);
                       printf("input the content: \n");
                       int n = read(fd_stdin,writetext,10000);
                       writetext[n]=0;
                       close(fd);
                       unlink(file);
                       fd = open(file,O_RDWR|O_CREAT);
                       assert(fd!=-1);
                       write(fd,writetext,10000);	
                       close(fd);
                      
                       break;
                 }
                 else if(strcmp("show",ins)==0){
	               char pathname[100];
                       char filename[100];
                       struct inode* dir_inode;
                       memset(filename,0,100);
                       dir_inode = root_inode;
                       char* s = pathname;
                       char* t = filename;
                       if (s == 0)
		           break;

	               if (*s == '/')
		           s++;

	               while (*s) {		/* check each character */
		           if (*s == '/')
			        break;
		           *t++ = *s++;
		/* if filename is too long, just truncate it */
		           if (t - filename >= 99)
			          break;
	               }
	               *t = 0;
                          int dir_blk0_nr = dir_inode->i_start_sect;
	                  int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1)/SECTOR_SIZE;
	                  int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE; 
	                  int m = 0;
                          int j;
	                  struct dir_entry * pde;
	                  for (i = 0; i < nr_dir_blks; i++) {
		               RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		               pde = (struct dir_entry *)fsbuf;
		               for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			            printf("%s ",pde->name);
		                   }
		
	                 }
	                 printf("\n");
                       
                       break;
                 }
                 else if(strcmp("remove",ins)==0){
                       int t = unlink(file);
                       if(t==0) printf("remove succeed!\n");
                       else printf("remove failed!\n");
                       break;
                 }
            }
        }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	char tty_name[] = "/dev_tty1";

	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];

	while (1) {
                printf("*****************************\n");
		printf("*     Game and application  *\n");
		printf("*           gobang          *\n");
		printf("*           boxman          *\n");
		printf("*        fingerguessing     *\n");
		printf("*          calendar         *\n");
		printf("*         calculator        *\n");
		printf("*****************************\n");

		printf("$ ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;

		if (strcmp(rdbuf, "gobang") == 0)
			wuziqi(fd_stdin, fd_stdout);
                else if(strcmp(rdbuf,"boxman")==0)
                        tuixiangzi(fd_stdin, fd_stdout);
                else if(strcmp(rdbuf,"fingerguessing")==0)
                        caiquan(fd_stdin, fd_stdout);
                else if(strcmp(rdbuf,"calendar")==0)
                        rili(fd_stdin, fd_stdout);
                else if(strcmp(rdbuf,"calculator")==0)
                        jisuanqi(fd_stdin, fd_stdout);
		else
			if (rdbuf[0])
				printf("{%s}\n", rdbuf);
	}

	assert(0); /* never arrive here */
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	spin("TestC");
	/* assert(0); */
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

