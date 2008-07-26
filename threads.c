
#define WINCE

#include <stdio.h>
#include <string.h>
#include "nspr/nspr.h"
#include "jsapi.h"

int GLOBAL_RUNTIME;
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
	JSRuntime* rt;
	int n;
} thread_info;

void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

JSRuntime* create_runtime() {
	JSRuntime* rt = JS_NewRuntime(8L * 1024L * 1024L);
	
	if (rt == NULL) {
		printf("runtime could not be created");
		exit(1);
	}

	return rt;
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
	JSRuntime* rt;
	
	if(GLOBAL_RUNTIME) {
		rt = ti->rt;
	}
	else {
		rt = create_runtime();
	}

	JSContext *cx = create_context(rt);
	JSObject *global = init_context(cx);
	
	JS_BeginRequest(cx);
	
	// This script is tweaked for various runs
	char* script =
		"var i=0; while(i<10000) {"
		"  for(var j in [1,2,3,4,5]) {"
		"    var g = j*1.2345;"
		"  }"
		"  i++;"
		"}";
		
	jsval r;
	JS_EvaluateScript(cx, global,
					  script, strlen(script),
					  "javascript", 0, &r);

	JS_EndRequest(cx);
	JS_DestroyContext(cx);

	if(!GLOBAL_RUNTIME) {
		JS_DestroyRuntime(rt);
	}
}

int main(int argc, const char *argv[]) {
	if(argc != 4) {
		printf("Usage: threads runtimes? threaded? thread-count");
		return 1;
	}

	GLOBAL_RUNTIME = atoi(argv[1]);
	RUN_THREADED = atoi(argv[2]);
	NUM_THREADS = atoi(argv[3]);

	int i;
	thread_info* threads[NUM_THREADS];
	JSRuntime *rt = NULL;

	if(GLOBAL_RUNTIME) {
		rt = create_runtime();
	}

	START_TIME
	
	for(i=0; i<NUM_THREADS; i++) {
		thread_info *ti = (thread_info*)malloc(sizeof(thread_info));
		ti->rt = rt;
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
	
	/* Cleanup. */
	if(GLOBAL_RUNTIME) JS_DestroyRuntime(rt);
	JS_ShutDown();
    return 0;
}
