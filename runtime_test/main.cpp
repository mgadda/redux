//
//  main.cpp
//  runtime_test
//
//  Created by Matt Gadda on 21/07/12.
//  Copyright (c) 2012 Catalpa Labs. All rights reserved.
//

#include <iostream>
#include "runtime.h"

void foo(rdx_process *proc, int argc, void *argv) {	
	rdx_exit(*proc);
}

void do_something(rdx_process *proc, int argc, void *argv);
void do_something(rdx_process *proc, int argc, void *argv) {
	rdx_spawn(foo, 0, NULL);
	
	void *expression = rdx_receive(*proc);
	printf("%i got a message: %s\n", proc->pid, (char*)expression);
	
	const char response[] = "ok";
	rdx_send_message(response, strlen(response)+1, 2);
	
	rdx_exit(*proc);	
}

void do_something_else(rdx_process *proc, int argc, void *argv);
void do_something_else(rdx_process *proc, int argc, void *argv) {
	
	const char msg[] = "Mr Watson–Come here–I want to see you\0";
	
	rdx_send_message(msg, strlen(msg)+1, 1);
	
	void *response = rdx_receive(*proc);	
	printf("%i got a response: %s\n", proc->pid, (char*)response);
	
	rdx_spawn(foo, 0, NULL);
	
	rdx_exit(*proc);
}


int main(int argc, const char * argv[])
{
	init_redux();
	
	rdx_spawn(do_something, 0, NULL);
	rdx_spawn(do_something_else, 0, NULL);

	rdx_run_scheduler();
	return 0;
}

