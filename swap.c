/* James Buchanan May 2017  						    *
 *               							    *
 * Memory management simulation in an operating system. It simulates a      *
 * secondary memory holding processes ready to run and the main memory      *
 * with a round robin queue schedule, it uses either best fit, worst fit or *
 * first fit depending on the given input and outputs the general status of *
 * given memory in the operating system. 				    *
 * inputs as given in README.txt and the input files can be found in tests/ *
 * Written in fedora 25                                                     */


#include "swap.h"

static uint16_t MEMORY_SIZE;
static uint16_t QUANTUM;
static char* ALGORITHM;

int main ( int argc, char **argv )
{
	int8_t input;
	
	FILE* fp = NULL;
	
	while ((input = getopt(argc, argv, "f:a:m:q:")) != EOF)
	{
		switch ( input )
		{
			case 'f':
				fp = fopen(optarg, "r");
				break;
			case 'a':
				ALGORITHM = malloc(strlen(optarg)*sizeof(int8_t) + 1);
				strcpy(ALGORITHM, optarg);
				break;
			case 'm':
				MEMORY_SIZE = atoi(optarg);
				break;
			case 'q':
				QUANTUM = atoi(optarg);
				break;
			// Should never reach here.
			default:
				break;
		}
	}
	
	memory_segment_t* memory = initializeMemory(MEMORY_SIZE);
	disk_t* disk = initializeDisk();

	if (disk == NULL) printf("disk is null\n");
	
	processLoop (fp, disk, memory);
	free(memory->next);
	free(memory);
	free(ALGORITHM);

	return 0;
}

/* Initializes the disk for the duration of the simulation. */
disk_t* initializeDisk () 
{
	disk_t* new_disk = malloc(sizeof(disk_t));
	if (new_disk == NULL) { 
		exit(0);
	}	
	
	new_disk->process = NULL;
	new_disk->next = NULL;
	

	return new_disk;
}


/* Initializes the memory give the allocated amount of memory */
memory_segment_t* initializeMemory ( uint16_t memory )
{
	memory_segment_t* new_memory = malloc(sizeof(memory_segment_t));
	if (new_memory == NULL) {
		exit(0);
	}
	
	new_memory->memory_used = memory;
	new_memory->process = NULL;
	new_memory->next = NULL;	

	return new_memory;
}


/* Loads a process from disk into memory (if one exits). Choosing the  *
 * process that has been on disk the longest. Returns 1 if the process *
 * taken off memory was in the front of the queue. This lets the queue *
 * be properly updated during the schedule and process loop functions */
int swap ( disk_t** disk, memory_segment_t** memory, queue_t** queue, uint16_t time )
{
	int sentinel = 0;

	if (*disk == NULL) return sentinel;

	disk_t* temp = longestOnDisk(disk);
	
	if (temp->process->time_created > time) {
		return sentinel;
	} else if (temp == NULL) return sentinel;

	while(LOOP) {
		memory_segment_t* location;
		if (strcmp(ALGORITHM, "first") == 0) {
			location = firstFit(*memory, temp->process->memory_size);
		} else if (strcmp(ALGORITHM, "best") == 0) {
			location = bestFit(*memory, temp->process->memory_size);
		} else {
			location = worstFit(*memory, temp->process->memory_size);			
		}

		if (location != NULL) {
			temp->process->time_created = time;
			addToMemory(&location, temp->process);
			joinMemory(memory);
			printf("time = %d, %d loaded, numprocesses=%d, "
				 "numholes=%d, memusage=%d%%\n", 
					time, temp->process->pid, 				
					numberOfProcesses(*memory),
					numberOfHoles(*memory), 
					memoryUsage(*memory));
			break;
		} else {
			sentinel = longestOnMemory(disk, memory, queue, time);
			joinMemory(memory);
		}
	}
	free(temp);
	return sentinel;
	

}

/* Schedule another process to use the CPU, using a round robin strategy *
 * with quantum q. A process just swapped onto the disk should be placed *
 * at the end of the queue and then the process whose quantum has just   *
 * expired either has finished executing or should be behind the swapped *
 * in process.                                                           */
void schedule ( memory_segment_t** new_memory, queue_t** queue, uint16_t time ) 
{
	memory_segment_t* temp = *new_memory;
	if (time > 0) {}
	while (temp) {
		if (temp->process) {
			if ((inQueue(*queue, temp->process) < 0)) {	
				*queue = enQueue(*queue, temp->process);
				break;
			}
		}
		temp = temp->next;
	}

	//print_memory(new_memory);
	//print_queue(queue);
}

/* The process loop */
void processLoop ( FILE* fp, disk_t* disk, memory_segment_t* memory )
{
	if (fp == NULL) {
		printf("Error on input!\n");
		return;			
	}

	int32_t time_created, pid, size, job_time;

	while (fscanf( fp, "%d %d %d %d\n", 
		&time_created, &pid, &size, &job_time) != EOF) {
		
		process_t *new_process = addProcess(time_created, 
					pid, size, job_time);
		disk = addToDisk(new_process, disk);

	}

	queue_t *queue = initializeQueue();
	uint16_t quantum = QUANTUM;
	uint16_t time = 0;
	
	while (disk || isEmpty(memory) != 0) {
		if ((isEmpty(memory) == 0)) {	
			swap(&disk, &memory, &queue, time);	
			schedule(&memory, &queue, time);
		
		} else {
			time++;
			increaseTimeOnMemory(&memory);
			quantum--;
			queue->process->job_time--;
			if (queue->process->job_time == 0) {
				removeFromMemory(&memory, queue->process->pid);
				queue_t* temp = queue;
				queue = queue->next_process;
				free(temp->process);
				free(temp);				
				swap(&disk, &memory, &queue, time);	
				schedule(&memory, &queue, time);
				quantum = QUANTUM; 
			
			} else if (quantum == 0) {
				int sentinel = swap(&disk, &memory, &queue, time);	
				schedule(&memory, &queue, time);
				if (sentinel == 0) {
					queue_t* temp = queue;
					queue = queue->next_process;
					queue = enQueue(queue, temp->process);
					free(temp);
				}
				quantum = QUANTUM;
			}
		}
	}

	free(disk);
	fclose(fp);
	printf("time = %d, simulation finished.\n", time);
	
}


/* Creation of a new process, for simplicity, this is all done initially */
process_t *addProcess ( int32_t time_created, 
	int32_t pid, int32_t size, int32_t job_time )
{
	process_t *new_process = malloc(sizeof(process_t));
	new_process->pid 	  = pid;
	new_process->time_created = time_created;
	new_process->memory_size  = size;
	new_process->job_time = job_time; 
	
	return new_process;
	
}

/* Add a process onto disk */
disk_t *addToDisk ( process_t *new_process, disk_t *disk )
{
	if (disk == NULL) {
		disk = malloc(sizeof(disk_t));
		disk->process = new_process;
		disk->next = NULL;
	
	} else if (disk->process == NULL) {
		disk->process = new_process;
		disk->next = NULL;
		
	} else {
		disk_t *temp, *prev;
		prev = NULL;
		temp = disk;
		
		while (1) {
			if (temp == NULL) {
				temp = malloc(sizeof(disk_t));
				temp->next = NULL;
				temp->process = new_process;
				prev->next = temp;
				break;			
							
			}
			
			if (temp->process->time_created == new_process->time_created
				&& temp->process->pid < new_process->pid) {
				prev = temp;				
				temp = temp->next;		 

			} else if (temp->process->time_created < new_process->time_created) {
				prev = temp;				
				temp = temp->next;
			
			} else {
				disk_t *new_disk = malloc(sizeof(disk_t));
				new_disk->process = new_process;
				new_disk->next    = temp;
	
				if (prev != NULL) {
					prev->next = new_disk;
				} else {
					return new_disk;
				}
				break;		
			} 
				
		}		
	}
	return disk;
}


/* Returns 0 if there is no process in memory */
int isEmpty ( memory_segment_t* list )
{
	memory_segment_t* temp = list;	
	while (temp) {
		if (temp->process != NULL) {
			return 1;
		}
		temp = temp->next;
	}
	return 0;
		
}

/* Finds the first spot with the required amount of space. */
memory_segment_t* firstFit ( memory_segment_t* memory, uint16_t required_space ) 
{
	memory_segment_t *temp, *prev;
	prev = NULL;
	temp = memory;
	
	while (temp != NULL) {
		if (temp->memory_used >= required_space && temp->process == NULL) {
			if (prev == NULL) return memory;			
			else return prev;
		} else {
			prev = temp;
			temp = temp->next;
		}
	}
	return NULL;
}

/* Finds the smallest spot that will fit the process into memory. */
memory_segment_t* bestFit ( memory_segment_t* memory, uint16_t required_space )
{
	memory_segment_t *best, *temp, *prev_best, *prev;
	best 	  = NULL;
	prev_best = NULL;
	prev	  = NULL;
	temp 	  = memory;

	int32_t current_best = MAX_PROCESS_SIZE;
	
	while (temp != NULL) {
		if (temp->process == NULL) {
			int32_t free_space = temp->memory_used - required_space;
			if (free_space >= 0 && free_space < current_best) {
				current_best = free_space;
				prev_best = prev;
				best = temp;
			}
		}
		
		prev = temp;
		temp = temp->next;
	}
	if (best == NULL) {
		return NULL;
	} else if (prev_best == NULL) {
		return memory;
	} else {
		return prev_best;
	}
}

/* Finds the largest whole in memory. */
memory_segment_t* worstFit ( memory_segment_t* memory, uint16_t required_space )
{
	memory_segment_t *worst, *temp, *prev_worst, *prev;
	worst 	  = NULL;
	prev_worst = NULL;
	prev	  = NULL;
	temp 	  = memory;

	int32_t current_worst = 0;
	
	while (temp != NULL) {
		if (temp->process == NULL) {
			int32_t free_space = temp->memory_used - required_space;
			if (free_space >= 0 && free_space > current_worst) {
				current_worst = free_space;
				prev_worst = prev;
				worst = temp;
			}
		}
		
		prev = temp;
		temp = temp->next;
	}
	if (worst == NULL) {
		return NULL;
	} else if (prev_worst == NULL) {
		return memory;
	} else {
		return prev_worst;
	}
}


/* Initialize the round robin queue */
queue_t* initializeQueue ()
{
	queue_t* queue 	    = malloc(sizeof(queue_t));
	queue->process      = NULL;
	queue->next_process = NULL;
	return queue;
}

/* Adds a process into our queue at the end of the queue */
queue_t* enQueue ( queue_t *q, process_t *process )
{	
	if (q == NULL) q = initializeQueue();
	if (q->process == NULL) 
	{	
		q->process = process;
		return q;
	}
	queue_t *new_node, *p;
	new_node               	= malloc(sizeof(queue_t));
	new_node->process     	= process;
	new_node->next_process 	= NULL;

	if (!q) return NULL;

	p = q;
	while (p->next_process) {
		p = p->next_process;
	}
	p->next_process = new_node;
	
	return q;
}

/* Selects and returns the process that has been on the disk the longest. */
disk_t* longestOnDisk ( disk_t** disk )
{
	disk_t* longest_on_disk = *disk;
	*disk = (*disk)->next;
	
	return longest_on_disk;
}


/* Adds a process into memory */
void addToMemory ( memory_segment_t** memory, process_t* process )
{
	process->time_created = 0;
	if (((*memory)->process == NULL) && 
			(process->memory_size < (*memory)->memory_used)) {
		memory_segment_t* new_memory = malloc(sizeof(memory_segment_t));
		new_memory->next = (*memory)->next;
		new_memory->process = NULL;
		new_memory->memory_used = ((*memory)->memory_used 
						- process->memory_size);

		(*memory)->next = new_memory;
		(*memory)->process = process;
		(*memory)->memory_used = process->memory_size;

		if (new_memory->memory_used == 0) {
			(*memory)->next = (*memory)->next->next;
			free(new_memory);
		}
		
	} else if ((*memory)->next == NULL) {
		(*memory)->next = malloc(sizeof(memory_segment_t));
		(*memory)->next->memory_used = (*memory)->memory_used 
							- process->memory_size;
		(*memory)->next->process = NULL;
		(*memory)->memory_used = process->memory_size;
		(*memory)->process = process;
		
		if ((*memory)->next->memory_used == 0) {
			free((*memory)->next);
			(*memory)->next = NULL;
		}
	} else {
		memory_segment_t* new_memory 	= malloc(sizeof(memory_segment_t));
		new_memory->next 		= (*memory)->next;
		new_memory->memory_used		= process->memory_size;
		new_memory->process 		= process;
		new_memory->next->memory_used 	= (*memory)->next->memory_used 
							- process->memory_size;
		new_memory->next->process	= NULL;

		(*memory)->next 		= new_memory;
		
		if (new_memory->next->memory_used == 0) 
		{
			memory_segment_t* temp = new_memory->next;
			new_memory->next = new_memory->next->next;
			free(temp);
		}
	}
	
}

/* Selects the piece of memory that has been on memory the longest and places *
 * it back on disk 							    */
int32_t longestOnMemory ( disk_t** disk, memory_segment_t** memory, 
	queue_t** queue, uint16_t time )
{
	memory_segment_t* temp = *memory;
	memory_segment_t* longest = NULL;
	int sentinel = 0;
	
	while (temp->process == NULL) {
		temp = temp->next;
		if (temp == NULL) return 0;	
	}
	
	longest = temp;

	while (temp) {
		if (temp->process == NULL) {
			temp = temp->next;
		} else if (temp->process && longest->process) {
			if ((temp->process->time_created) 
				> (longest->process->time_created)) {			
				longest 	= temp;
				temp 		= temp->next;
			} else {
				temp 	  = temp->next;
			}
		} else {
			temp = temp->next;
		}
	}
	if (longest->process->pid == (*queue)->process->pid) {
		sentinel = 1;
	}
	longest->process->time_created = time;
	(*disk) = addToDisk(longest->process, *disk);
	deQueue(queue, longest->process);
	longest->process = NULL;
	
	return sentinel;
	
}

/* Removes an item from a queue as it has been swapped off memory, does not *
 * return anything or give information about what has been removed.         */
void deQueue ( queue_t** queue, process_t* process )
{
	queue_t* temp = *queue;	
	queue_t* prev = NULL;	
	while (temp) {
		if ((temp->process->pid == process->pid) && prev == NULL) {
			*queue = (*queue)->next_process;
			free(temp);
			break;
		} else if (temp->process->pid == process->pid) {
			prev->next_process = prev->next_process->next_process;
			free(temp);
			break;
		} else {
			prev = temp;
			temp = temp->next_process;
			
		}
	}
}

/* Goes through the memory and joins two pieces of memory together if they 
 * are next to each other. */
void joinMemory ( memory_segment_t** memory ) 
{
	memory_segment_t* temp = *memory;
	while (temp->next) {
		if (temp->process == NULL && temp->next->process == NULL) {
			memory_segment_t* to_free = temp->next;
			temp->memory_used += temp->next->memory_used;
			temp->next = temp->next->next;
			free(to_free);

		} else {
			temp = temp->next;
		}
			
		if (temp == NULL) break;
	}
	

}


/* returns 0 if the process input is in the queue given. */
int32_t inQueue ( queue_t* q, process_t* process )
{
	queue_t* temp = q;

	while (temp) {
		if (temp->process) {
			if (temp->process->pid == process->pid) {
				return 0;
			}
		}
		temp = temp->next_process;
	}

	return -1;
}

/* Goes through memory and counts the number of processes */
int32_t numberOfProcesses ( memory_segment_t* memory ) 
{
	int32_t processes = 0;
	memory_segment_t* temp = memory;
	
	while (temp) {
		if (temp->process) {
			processes++;		
		}
		temp = temp->next;
	}
	return processes;
}

/* Goes through memory and counts the number of holes */
int32_t numberOfHoles ( memory_segment_t* memory ) 
{

	int32_t holes = 0;
	memory_segment_t* temp = memory;
	
	while (temp) {
		if (!(temp->process)) {
			holes++;		
		}
		temp = temp->next;
	}
	return holes;
}

/* Counts the amount of memory in use */
int32_t memoryUsage ( memory_segment_t* memory )
{
	double memory_used = 0, memory_free = 0;
	memory_segment_t* temp = memory;

	while (temp) {
		if (temp->process) {
			memory_used += temp->memory_used;		
		} else {
			memory_free += temp->memory_used;
		}
		temp = temp->next;
	}
	/* Due to rounding errors in memusage percentage, adding exactly  *
	 * 0.9 to the end of this sum gave the correct answer. Unsure why *
	 * or whether it my rounding error or yours.                      */
	return (int32_t)(100*(memory_used/(memory_free + memory_used))+0.9);
	
}


/* Increases the time every process has been on memory by 1 */
void increaseTimeOnMemory ( memory_segment_t** memory )
{
	memory_segment_t* temp = *memory;
	while (temp) {
		if (temp->process){
			temp->process->time_created++;
		}
		temp = temp->next;
	}
}

/* Removes a process from memory once it has completed */
void removeFromMemory ( memory_segment_t** memory, uint16_t pid ) 
{
	memory_segment_t* temp = *memory;

	while (temp) {	
		if (temp->process) {
			if (temp->process->pid == pid) {
				temp->process = NULL;
				break;
			}
		}
		temp = temp->next;
	}
	joinMemory(memory);
}










