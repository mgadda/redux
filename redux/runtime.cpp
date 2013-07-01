//
//  runtime.cpp
//  redux
//
//  Created by Matthew Gadda on 2/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#define _XOPEN_SOURCE
#define GC_DEBUG

#include <iostream>
#include <gc.h>
#include <assert.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <ucontext.h>
#include "runtime.h"

#include <map>
#include <gc.h>
#include <gc/gc_allocator.h>

typedef std::map<int, rdx_process*, 
								 std::less<int>, 
								 gc_allocator<std::pair<int, rdx_process*> > > rdx_process_hash;

static int rdx_next_free_pid = 1;
static _STRUCT_UCONTEXT rdx_scheduler_context;
//static rdx_process_queue process_queue;
static rdx_process_hash process_map;

void init_redux() {
  GC_INIT();
//
//  int i;
//  for (i = 0; i < 10000000; ++i)
//  {
//    int **p = (int **) GC_MALLOC(sizeof(int *));
//    int *q = (int *) GC_MALLOC_ATOMIC(sizeof(int));
//    assert(*p == 0);
//    *p = (int *) GC_REALLOC(q, 2 * sizeof(int));
//    if (i % 100000 == 0)
//      printf("Heap size = %d\n", GC_get_heap_size());
//	}

	printf("initializing redux runtime...\n");

	getcontext(&rdx_scheduler_context); // is this necessary?

	//rdx_run_scheduler();
	
}

void rdx_reduce(rdx_process &proc) {
	proc.steps_left -= 1;
	if(proc.steps_left == 0) {
		proc.steps_left = 2000;
		swapcontext(&proc.context, &rdx_scheduler_context);
		printf("%i going to sleep because reductions is 0.\n", proc.pid);
	}
}


int rdx_spawn(void (*fun)(rdx_process *proc, int argc, void *argv), int argc, void *args) {
	GC_disable();
	rdx_process *proc = (rdx_process*)GC_MALLOC(sizeof(rdx_process));
	
	proc->steps_left = 2000;
	proc->pid = rdx_next_free_pid++;  

	proc->waiting = false;
	proc->first_message = NULL;
	proc->last_message = NULL;

	getcontext(&proc->context);
  proc->context.uc_link = &rdx_scheduler_context;
	proc->context.uc_stack.ss_sp = GC_MALLOC(SIGSTKSZ);
	proc->context.uc_stack.ss_size = SIGSTKSZ;

	//GC_dump();

	makecontext(&proc->context, (void (*)(void))fun, 3, proc, argc, args);
	rdx_queue_proc(proc);
	GC_enable();	
	return proc->pid;
}

void rdx_queue_proc(rdx_process *proc) {
	process_map[proc->pid] = proc;
}

void rdx_remove_proc(rdx_process &proc) {
	process_map.erase(proc.pid);
}

void rdx_run_scheduler() {
	while(process_map.size() > 0) { // exit if no more processes left
		rdx_process_hash::iterator it;
		for (it=process_map.begin(); it != process_map.end(); it++) {

			rdx_process* proc = (rdx_process*)it->second;
			if (!proc || proc->exited) {
				process_map.erase(it);
				break;
			}
			
			if(!proc->waiting) {
				if(swapcontext(&rdx_scheduler_context, &proc->context) != 0) {
					strerror(errno);
				}
				// we're here because the process either entered it wait state
				// or it ran out of steps
				// lets take a moment and deliver some messages (could this be done in another thread?)				
			}
			
		}
	}
}

void rdx_send_message(const void* expression, size_t size, int pid) {
	rdx_process* proc = process_map[pid];
	const char *msg = (const char*)expression;
	printf("sending message: '%s'\n", msg);
	
	if (proc) {
		// clear waiting flag so that scheduler gives this proc a chance
		// to do something with message
		proc->waiting = false;
		rdx_message* new_message = (rdx_message*)GC_MALLOC(sizeof(rdx_message));
		new_message->next = NULL;
		new_message->expression = GC_MALLOC(sizeof(size));
		memcpy(new_message->expression, expression, size);
		
		if (proc->last_message) {
			proc->last_message->next = new_message;
		}
		else {
			proc->last_message = new_message;
		}
		
		if (!proc->first_message) {
			proc->first_message = new_message;
		}
		
	}
	else {
		// proc already terminated
		// record this event somewhere, but its not an error
		printf("ERROR: process with pid=%i does not exist\n", pid);
	}
}

void *rdx_receive(rdx_process &proc) {
	if(proc.first_message) {
		void *msg = proc.first_message->expression;
		proc.first_message = proc.first_message->next;
		
		return msg;		
	}
	else {
		// go to sleep until there *is* at least one message
		proc.waiting = true;		
		swapcontext(&proc.context, &rdx_scheduler_context);		
		
		if (proc.first_message) {
			void *msg = proc.first_message->expression;
			proc.first_message = proc.first_message->next;
			return msg;
		}
		else {
			// why did we wake up if not to process a message?
			printf("ERROR: %i waking up, mysteriously\n", proc.pid);
			return NULL;
		}
	}
}

void rdx_yield(rdx_process &proc) {
	swapcontext(&proc.context, &rdx_scheduler_context);
}

void rdx_exit(rdx_process &proc) {
	printf("proc %i exiting.\n", proc.pid);
	proc.exited = true;
}


