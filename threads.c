
#define WINCE

#include <stdio.h>
#include <string.h>
#include "nspr/nspr.h"
#include "jsapi.h"

//printf("thread %d [%s]: %dms\n", ti->n, name, ms);
#define TIME(ti, name, exp) do		 \
  { PRIntervalTime t = PR_IntervalNow(); \
	do { exp; } while(0);											\
    PRUint32 ms = PR_IntervalToMilliseconds(PR_IntervalNow() - t);	\
	ti->timer = ms;                                                 \
  } while(0);

typedef struct {
	PRThread* thread;
	JSRuntime* rt;
	int n;
	long timer;
} thread_info;

/* The error reporter callback. */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

JSContext* create_context(JSRuntime *rt) {
	JSContext *cx;

	/* Create a context. */
    cx = JS_NewContext(rt, 8192);
    if (cx == NULL)
        return NULL;
    JS_SetOptions(cx, JSOPTION_VAROBJFIX);
    JS_SetVersion(cx, JSVERSION_LATEST);
    JS_SetErrorReporter(cx, reportError);

	return cx;
}

JSObject* init_context(JSContext *cx) {
	JSObject  *global;

	/* The class of the global object. */
	static JSClass global_class = {
		"global", JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
		JSCLASS_NO_OPTIONAL_MEMBERS
	};
	
    /* Create the global object. */
	JS_BeginRequest(cx);
	
    global = JS_NewObject(cx, &global_class, NULL, NULL);
    if (global == NULL) {
        return  NULL;
	}

    /* Populate the global object with the standard globals,
       like Object and Array. */
    if (!JS_InitStandardClasses(cx, global))
		return NULL;

	JS_EndRequest(cx);

	return global;
}


void run(void *arg) {
	thread_info* ti = (thread_info*)arg;
	int n = ti->n;
	JSRuntime *rt = ti->rt;
	JSContext *cx = create_context(rt);
	JSObject *global = init_context(cx);
	
	JS_BeginRequest(cx);
	
	//printf("*** entering thread 1 ***\n");
	
	char* script =
		"var i=0; while(i<100000) { i*123.456; i++; }";

	jsval r;
	TIME(ti, "eval", JS_EvaluateScript(cx, global,
									   script, strlen(script),
									   "javascript", 0, &r));


	//printf("*** exiting thread 1 ***\n");
	
	JS_EndRequest(cx);
	JS_DestroyContext(cx);
}

int main(int argc, const char *argv[])
{
	if(argc == 1) {
		printf("Usage: threads count");
		return 1;
	}
	
	int NUM_THREADS = atoi(argv[1]);
	int RUN_THREADED = 1;

	int i;
	thread_info* threads[NUM_THREADS];
    JSRuntime *rt;
		
	/* Create a JS runtime. */
	rt = JS_NewRuntime(8L * 1024L * 1024L);
	if (rt == NULL)
		return 1;
	
	for(i=0; i<NUM_THREADS; i++) {
		thread_info *ti = (thread_info*)malloc(sizeof(thread_info));
		ti->rt = rt;
		ti->n = i;

		if(RUN_THREADED) {
			ti->thread = PR_CreateThread(PR_USER_THREAD, run, (void*)ti, 50,
										 PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
		}
		else {
			run(ti);
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
	
	/* Cleanup. */
	JS_DestroyRuntime(rt);
	JS_ShutDown();
    return 0;
}
