/*
 ============================================================================
 COURSE     : CS-UH 3010
 DESCRIPTION: This is the starter code base for PS 1.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define FOURS 1
#define TWOS 2
#define POD 0

#define QATAR 0
#define GERMAN 1

static sem_t* full_pod; 
static sem_t* empty_pod;
static sem_t* pod_mutex;

// semaphores for arrive
static sem_t* empty_arrive;
static sem_t* arrive_mutex;

int fan_no = 0;
int fans[4];

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
	sem_wait(pod_mutex);
	types[idx] = fan->type;
	tnums[idx] = fan->t_num;
	cases[idx] = c;
	idx++;
	sem_post(pod_mutex);
	for(int i = 0; i<3; i++){
		sem_post(empty_pod);
		sem_wait(full_pod);
	}
}

static void get_in_pod(void *f){
	struct fan_info* fan = f;
	sem_wait(empty_pod);
	sem_wait(pod_mutex);
	types[idx] = fan->type;
	tnums[idx] = fan->t_num;
	cases[idx] = POD;
	idx++;
	sem_post(pod_mutex);
	sem_post(full_pod);
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
		sem_wait(empty_arrive);
		sem_wait(arrive_mutex);

		/*
		 * Case 2: fan != fan behind (and fan_no is odd)
		 * 		release empty_arrive (will wait til finds partner to board)
		 * 		Release arrive_mutex
		 * 		Go back to top of function 
		 */
		if((fan_no % 2 == 1) && (fan->type != fans[fan_no - 1])) {
			
			// release locks
			sem_post(arrive_mutex);
			sem_post(empty_arrive);
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
			sem_post(arrive_mutex);
			sem_post(empty_arrive);
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
			sem_post(arrive_mutex);

			// board fan
			get_in_pod(f); // this will stall, semaphore will only release at the very end
			// release semaphore
			sem_post(empty_arrive);
			return f;
		}
	}
}



int main(void) {
	int s;
	//Initialize Semaphores!

	//Destroy if they exist first ... Named semaphores keep state if not properly destroyed.
	sem_unlink("/ep");
	sem_unlink("/fp");
	sem_unlink("/pm");
	
	sem_unlink("/ea");
	sem_unlink("/am");

	//Now open semaphores.
	empty_pod = sem_open("/ep", O_CREAT, S_IRUSR | S_IWUSR, 0);
	full_pod = sem_open("/fp", O_CREAT, S_IRUSR | S_IWUSR, 0);
	pod_mutex = sem_open("/pm", O_CREAT, S_IRUSR | S_IWUSR, 1);

	empty_arrive = sem_open("/ea", O_CREAT, S_IRUSR | S_IWUSR, 4);
	arrive_mutex = sem_open("/am", O_CREAT, S_IRUSR | S_IWUSR, 1);

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

	//Destroy Semaphores
	sem_unlink("/ep");
	sem_unlink("/fp");
	sem_unlink("/pm");

	sem_unlink("/ea");
	sem_unlink("/am");

	return EXIT_SUCCESS;
}


