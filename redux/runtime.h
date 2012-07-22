//
//  runtime.h
//  redux
//
//  Created by Matthew Gadda on 2/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef redux_runtime_h
#define redux_runtime_h

#include <map>

/* undefine the defaults */
//#undef uthash_malloc
//#undef uthash_free

/* re-define, specifying alternate functions */
//#define uthash_malloc(sz) GC_MALLOC(sz)
//#define uthash_free(ptr,sz) GC_FREE(ptr);

extern "C" {
  void init_redux();
  int runtime_fun(int value);  
	
	// like objc_send(), this is the primary mechanism through which all function
	// calls are made. it keeps track of which process made the function call
	// and how many reductions is has left before it will be preempted and another
	// process gets to run. 	
	//void *rdx_call(); 
	
	// invoked by every expression to give runtime opportunity to preempt process
	
	typedef struct rdx_message_t {
		void *expression;
		rdx_message_t *next;
	} rdx_message;
	
	typedef struct rdx_process_t {
		int					pid;
		int					steps_left;
		ucontext_t	context;
		bool				waiting;
		bool			  exited;
		rdx_message *first_message; 
		rdx_message *last_message;
	} rdx_process;
	
	typedef std::map<int, rdx_process*> rdx_process_hash;
	
//	typedef struct rdx_process_queue_item_t {
//		rdx_process *proc;
//		rdx_process_queue_item_t *next;
//	} rdx_process_queue_item;
	
	//typedef rdx_process_queue_item* rdx_process_queue;
	
	void rdx_reduce(rdx_process &proc);
	void rdx_spawn(void (*fun)(rdx_process *proc, int argc, void *argv), int argc, void *args);
	void rdx_queue_proc(rdx_process *proc);
	void rdx_remove_proc(rdx_process &proc);
	
	void rdx_run_scheduler();
	
	void rdx_send_message(const void* expression, size_t size, int pid);
	void *rdx_receive(rdx_process &proc);
	void rdx_yield(rdx_process &proc);
	void rdx_exit(rdx_process &proc);
}
#endif

