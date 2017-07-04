#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>


#ifndef SWAP
#define SWAP
#define LOOP 1
#define MAX_PROCESS_SIZE 1000000
/* Process data structure */
typedef struct process {
	uint16_t pid;			// Process ID
	uint16_t memory_size;		// Memory required
	uint16_t job_time;		// Time left required
	uint16_t time_created;		// Time the process was creat
} process_t;

/* Data structure for our virtual disk of processes. */
typedef struct disk disk_t;
struct disk {
	process_t *process;
	disk_t *next;
};



/* Data structure for memory */
typedef struct memory memory_segment_t;
struct memory {
	process_t *process;		// Process ID using this segment
	uint16_t memory_used;		// Memory used by this segment
	memory_segment_t *next;
};


/* Round Robin queue */
typedef struct queue queue_t;
struct queue {
	process_t *process;
	queue_t *next_process;
};


disk_t* addToDisk ( process_t *new_process, disk_t *disk );
disk_t* initializeDisk ();
disk_t* longestOnDisk ( disk_t** disk );
int32_t inQueue ( queue_t* q, process_t* process );
int32_t isEmpty ( memory_segment_t* list );
int32_t longestOnMemory ( disk_t** disk, memory_segment_t** memory, queue_t** queue, 
	uint16_t time );
int32_t memoryUsage ( memory_segment_t* memory );
int32_t numberOfHoles ( memory_segment_t* memory);
int32_t numberOfProcesses ( memory_segment_t* memory );
int32_t swap ( disk_t** disk, memory_segment_t** memory, queue_t** queue, 
	uint16_t time );
memory_segment_t* bestFit ( memory_segment_t* memory, uint16_t required_space );
memory_segment_t* firstFit ( memory_segment_t* memory, uint16_t required_space );
memory_segment_t* initializeMemory ( uint16_t memory );
memory_segment_t* worstFit ( memory_segment_t* memory, uint16_t required_space );
process_t *addProcess ( int32_t time_created, int32_t pid, int32_t size, 
	int32_t job_time );
queue_t* enQueue ( queue_t *q, process_t *process );
queue_t* initializeQueue ();
void addToMemory ( memory_segment_t** memory, process_t* process );
void deQueue ( queue_t** queue, process_t* process );
void freeQueueHead ( queue_t **q );
void increaseTimeOnMemory ( memory_segment_t** memory);
void joinMemory ( memory_segment_t** memory );
void processLoop ( FILE* fp, disk_t* disk, memory_segment_t* memory );
void removeFromMemory( memory_segment_t** memory, uint16_t pid );
void schedule ( memory_segment_t** new_memory, queue_t** queue, uint16_t time );

#endif

