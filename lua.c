
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "nspr/nspr.h"

//	printf("thread %d [%s]: %dms\n", ti->n, name, ms);
#define TIME(ti, name, exp) do		 \
  { PRIntervalTime t = PR_IntervalNow(); \
	do { exp; } while(0);											\
    PRUint32 ms = PR_IntervalToMilliseconds(PR_IntervalNow() - t);	\
	ti->timer = ms;                                                 \
  } while(0);

typedef struct {
	PRThread* thread;
	lua_State* state;
	int n;
	long timer;
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

	TIME(ti, "eval", luaL_dostring(l, script));

	//printf("*** exiting thread %d ***\n", n);

	lua_close(l);
}


int main(int argc, const char *argv[]) {
	if(argc == 1) {
		printf("Usage: threads count");
		return 1;
	}
	
	int NUM_THREADS = atoi(argv[1]);
	int RUN_THREADED = 1;

	int i;
	thread_info* threads[NUM_THREADS];
	
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

	long total_time = 0;
	for(i=0; i<NUM_THREADS; i++) {
		if(RUN_THREADED) {
			PR_JoinThread(threads[i]->thread);
		}

		total_time += threads[i]->timer;
		free(threads[i]);
	}

	printf("%d\n", total_time);
	
    return 0;
}
