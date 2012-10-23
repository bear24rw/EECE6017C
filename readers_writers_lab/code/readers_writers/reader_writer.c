
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "includes.h"
#include "alt_ucosii_simple_error_check.h"
#include "reader_writer.h"



/* Definition of shared_buf_sem Semaphore */
OS_EVENT *shared_buf_sem_rd;    // num of readers
OS_EVENT *shared_buf_sem_wr;    // writer is writing
OS_SEM_DATA sem_data;

/* Definition of Task Stacks */
OS_STK reader_stk[TASK_STACKSIZE];
OS_STK writer_stk[TASK_STACKSIZE];

void elipsis(){
	INT8U i;
	for(i=0; i<3; i++){
		printf(".");
		OSTimeDlyHMSM(0,0,1,0);
	}
}

void reader(void *pdata){
	INT8U return_code;

	while(true)
	{
        // wait for writer to finish
		OSSemPend(shared_buf_sem_wr, 0, &return_code);

        // we are using a read spot
		OSSemPend(shared_buf_sem_rd, 0, &return_code);

        // give write sem back so other readers can read
        OSSemPost(shared_buf_sem_wr);

		if(book_mark == 0){
			OSSemPost(shared_buf_sem_rd);
			OSTimeDlyHMSM(0,0,2,0);
		}else{
			INT8U index = 0;

			printf("Reader is reading");
			elipsis();

			while(index < book_mark){
				printf("%s ", book[index][0]);
				index += 1;
			}

			printf("\n");

			OSSemPost(shared_buf_sem_rd);

			OSTimeDlyHMSM(0,0,10,0);
		}
	}
}

void writer(void *pdata){
	INT8U return_code;

	while(true){

        // wait for all readers to finish
        OSSemQuery(shared_buf_sem_rd, &sem_data);
        if (sem_data.OSCnt < 3) continue;

        // writer is now in use
		OSSemPend(shared_buf_sem_wr, 0, &return_code);

		if(book_mark == WORDS_IN_BOOK -1){
			book_mark = 0;
		}
		printf("Writer is writing");
		elipsis();

		book[book_mark][0] = pangram[book_mark][0];
		printf("%s \n", book[book_mark][0]);

		book_mark += 1;

        // done writing
		OSSemPost(shared_buf_sem_wr);

		OSTimeDlyHMSM(0,0,1,0);
	}
}


void  reader_writer_init()
{
	INT8U return_code = OS_NO_ERR;
	book_mark = 0;

	//initialize pangram
	pangram[0][0] = "A\0";
	pangram[1][0] = "quick\0";
	pangram[2][0] = "brown\0";
	pangram[3][0] = "fox\0";
	pangram[4][0] = "jumps\0";
	pangram[5][0] = "over\0";
	pangram[6][0] = "the\0";
	pangram[7][0] = "lazy\0";\
	pangram[8][0] = "dog\0";

	//create writer
	return_code = OSTaskCreate(writer, NULL, (void*)&writer_stk[TASK_STACKSIZE-1], WRITER_PRIO);
	alt_ucosii_check_return_code(return_code);

	//create reader
	return_code = OSTaskCreate(reader, NULL, (void*)&reader_stk[TASK_STACKSIZE-1], READER_1_PRIO);
	alt_ucosii_check_return_code(return_code);

	return_code = OSTaskCreate(reader, NULL, (void*)&reader_stk[TASK_STACKSIZE-1], READER_2_PRIO);
	alt_ucosii_check_return_code(return_code);

	return_code = OSTaskCreate(reader, NULL, (void*)&reader_stk[TASK_STACKSIZE-1], READER_3_PRIO);
	alt_ucosii_check_return_code(return_code);
}


int main (int argc, char* argv[], char* envp[])
{
	shared_buf_sem_wr = OSSemCreate(1);
	shared_buf_sem_rd = OSSemCreate(3);

    printf("Init..\n");

	reader_writer_init();
    
    printf("Starting..\n");

	OSStart();

	return 0;
}
