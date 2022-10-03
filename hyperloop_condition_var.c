/*
 ============================================================================
 COURSE     : CS-UH 3010
 DESCRIPTION: This is the starter code base for PS 1.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define FOURS 1
#define TWOS 2
#define POD 0

#define QATAR 0
#define GERMAN 1

atomic_flag pod;
atomic_flag board;
atomic_flag boarded;

atomic_flag arrive;

int fan_no = 0;
int fans[4] = {-1, -1, -1, -1};

int idx = 0;

int num_fans  = 0;

int *types; // hold fan team 
int *tnums; // thread number
int *cases; // pod case (got into pod / launched a 4 pod / launched a 2 - 2 pod)


struct fan_info {
	pthread_t t_id; // fan thread info
	int t_num; // thread number (order of creation)
	int type; // team fan belongs to
};


static void launch(void *f, int c){
	struct fan_info* fan = f;
	// sem_wait(pod);
	while(atomic_flag_test_and_set(&pod)) {
		sched_yield();
	}

	types[idx] = fan->type;
	tnums[idx] = fan->t_num;
	cases[idx] = c;
	idx++;

	atomic_flag_clear(&pod);
	
	atomic_flag_clear(&board);
	while(atomic_flag_test_and_set(&boarded)) {
		sched_yield();
	}
	atomic_flag_test_and_set(&board);
}

static void get_in_pod(void *f){
	struct fan_info* fan = f;
	while(atomic_flag_test_and_set(&board)) {
		sched_yield();
	}
	while(atomic_flag_test_and_set(&pod)) {
		sched_yield();
	}
	types[idx] = fan->type;
	tnums[idx] = fan->t_num;
	cases[idx] = POD;
	idx++;
	if(idx % 4 == 0) {
		atomic_flag_clear(&boarded);
	}
	atomic_flag_clear(&pod);
	atomic_flag_clear(&board);
}


/**
 * @brief
 * arrive will treat pods as pairs where you can only board as pairs of same team 
 * 
 * @param f 
 * @return void* 
 */
static void* arrives(void *f){
	struct fan_info* fan = f;

	/*
	 * The following is just starter code. Please replace with the
	 * correct code.
	 */
	// printf("Thread %d started with type %s\n", fan->t_num, (fan->type ? "QATAR": "GERMAN"));
	

	while(1) {
		while(atomic_flag_test_and_set(&arrive)) {
			sched_yield();
		}

		/*
		 * Case 2: fan != fan behind (and fan_no is odd)
		 * 		release empty_arrive (will wait til finds partner to board)
		 * 		Release arrive_mutex
		 * 		Go back to top of function 
		 */
		if((fan_no % 2 == 1) && (fan->type != fans[fan_no - 1])) {
			
			// release locks
			atomic_flag_clear(&arrive);
		}

		/*
		 * Case 3: fan_no == 3
		 * 		Pod arrivals are full
		 * 		Board the pod
		 * 		set fan_no to 0
		 * 		clear fans array
		 * 		Check if TWOS or FOURS
		 * 		call launch
		 * 		post to empty_arrive 4 times
		 * 		release arrive_mutex
		 */
		else if(fan_no == 3) { // Case 2 guarantees last fan fits into pod

			//  launch pod! 
			launch(f, fans[0] == fans[2] ? FOURS : TWOS);
			// reset info
			fan_no = 0;
			// release locks
			atomic_flag_clear(&arrive);
			return f;
		}
		
		/*
		 * Case 4: fan == fan behind
		 * 		Fan can't be fan 3
		 */
		else {

			// add fan ( by treating pods as pairs, indexes 0 and 2 determine 
			//		team and can just be added later)
			fans[fan_no] = fan->type;
			++fan_no;

			// release mutex for other threads to use
			atomic_flag_clear(&arrive);

			// board fan
			get_in_pod(f); // this will stall, semaphore will only release at the very end
			return f;
		}
	}
}



int main(void) {
	int s;

	//Now open semaphores.
	atomic_flag_clear(&pod);
	atomic_flag_test_and_set(&board);
	atomic_flag_test_and_set(&boarded);

	atomic_flag_clear(&arrive);


	//Get number of fans from stdin
	scanf("%d", &num_fans);
	
	//Create fan information data structure
	struct fan_info *finfo;
	finfo = calloc(num_fans, sizeof(struct fan_info));
	if(finfo == NULL) handle_error("calloc");

	// Initialize info for pods
	types = malloc(num_fans * sizeof(int)); // fan's team 
	tnums = malloc(num_fans * sizeof(int)); // thread number
	cases = malloc(num_fans * sizeof(int)); /* cases (get in pod / launch 4 / 
														launch 2-2) */


	//Create and run threads
	for(int t=0; t<num_fans; t++){
		finfo[t].t_num = t+1; // thread num
		//Get type from stdin string
		scanf("%d", &finfo[t].type);
		// Each thread calls arrives() on startup
		s = pthread_create(&finfo[t].t_id, NULL, &arrives, &finfo[t]);
		if(s!= 0)handle_error_en(s, "pthread_create");
	}

	//Cleanup all threads
	for(int t=0;t<num_fans; t++){
		s = pthread_join(finfo[t].t_id, NULL);
		if(s!=0) handle_error_en(s, "pthread_join");

		//Print output! 
		printf("%2d-%s (%s)%s", tnums[t],
					(types[t] == QATAR ? "QATAR": "GERMAN"),
					(cases[t] == POD ? "pod": (cases[t] == TWOS ? "twos" : "fours")),
					((t+1)%4==0? "\n": ", "));

	}

	return EXIT_SUCCESS;
}


