
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "nspr/nspr.h"

int GLOBAL_STATE;
int RUN_THREADED;
int NUM_THREADS;

//	printf("thread %d [%s]: %dms\n", ti->n, name, ms);
long timer;
#define TIMER timer
#define START_TIME do {		\
  timer = PR_IntervalNow(); \
  } while(0);

#define END_TIME do {											\
  timer = PR_IntervalToMilliseconds(PR_IntervalNow() - timer);	\
  } while(0);

typedef struct {
	PRThread* thread;
	lua_State* state;
	int n;
} thread_info;


void run(void *arg) {
	thread_info* ti = (thread_info*)arg;
	int n = ti->n;

	lua_State *l = luaL_newstate();
    luaL_openlibs(l);
	
	//printf("*** entering thread %d ***\n", n);

	char* script =
		"for i=1,10000,1 do"
		"  local a = {i,2,3,4,5,6}"
		"  for v=1,#a,1 do"
		"    g=a[v]*5.4321"
		"  end "
		"end";

	luaL_dostring(l, script);

	//printf("*** exiting thread %d ***\n", n);

	lua_close(l);
}

int main(int argc, const char *argv[]) {
	if(argc == 1) {
		printf("Usage: threads count");
		return 1;
	}

	GLOBAL_STATE = atoi(argv[1]);
	RUN_THREADED = atoi(argv[2]);
	NUM_THREADS = atoi(argv[3]);

	int i;
	thread_info* threads[NUM_THREADS];

	START_TIME
	
	for(i=0; i<NUM_THREADS; i++) {
		thread_info *ti = (thread_info*)malloc(sizeof(thread_info));
		ti->n = i;

		ti->thread = PR_CreateThread(PR_USER_THREAD, run, (void*)ti, 1000,
									 PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);

		if(!RUN_THREADED) {
			PR_JoinThread(ti->thread);
		}
		
		threads[i] = ti;
	}

	for(i=0; i<NUM_THREADS; i++) {
		thread_info *ti = threads[i];
		
		if(RUN_THREADED) {
			PR_JoinThread(ti->thread);
		}

		free(ti);
		threads[i] = NULL;
	}

	END_TIME
	printf("%d\n", TIMER);
	
    return 0;
}
