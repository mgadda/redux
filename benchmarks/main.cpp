//
//  main.cpp
//  benchmarks
//
//  Created by Matt Gadda on 22/07/12.
//  Copyright (c) 2012 Catalpa Labs. All rights reserved.
//

#include <iostream>
#include "runtime.h"

void spawn_lots(rdx_process *proc, int argc, void *argv);
void wait_for_death(rdx_process *proc, int argc, void *argv);

void spawn_lots(rdx_process *proc, int argc, void *argv) {
	int n = 10;
	int pids[10];
	
	for (int i=0; i<n; i++) {
		rdx_reduce(*proc);
		pids[i] = rdx_spawn(wait_for_death, 0, NULL);
	}
	
	for (int i=0; i<n; i++) {
		rdx_reduce(*proc);
		rdx_send_message(NULL, 0, pids[i]);
	}
	rdx_exit(*proc);	
}

void wait_for_death(rdx_process *proc, int argc, void *argv) {
	void *msg = rdx_receive(*proc);	
	rdx_exit(*proc);
}

int main(int argc, const char * argv[])
{
	init_redux();
	rdx_spawn(spawn_lots, 0, NULL);
	rdx_run_scheduler();
	return 0;
}

/*

 %% ---
 %%  Excerpted from "Programming Erlang",
 %%  published by The Pragmatic Bookshelf.
 %%  Copyrights apply to this code. It may not be used to create training material, 
 %%  courses, books, articles, and the like. Contact us if you are in doubt.
 %%  We make no guarantees that this code is fit for any purpose. 
 %%  Visit http://www.pragmaticprogrammer.com/titles/jaerlang for more book information.
 %%---
 -module(processes).
 
 -export([max/1]).
 
 %% max(N) 
 %%   Create N processes then destroy them
 %%   See how much time this takes
 
 max(N) ->
	 Max = erlang:system_info(process_limit),
	 io:format("Maximum allowed processes:~p~n",[Max]),
	 statistics(runtime),
	 statistics(wall_clock),
	 L = for(1, N, fun() -> spawn(fun() -> wait() end) end),
	 {_, Time1} = statistics(runtime),
	 {_, Time2} = statistics(wall_clock),
	 lists:foreach(fun(Pid) -> Pid ! die end, L),
	 U1 = Time1 * 1000 / N,
	 U2 = Time2 * 1000 / N,
	 io:format("Process spawn time=~p (~p) microseconds~n",
	 [U1, U2]).
 
 wait() ->
	 receive
		die -> void
	 end.
 
 for(N, N, F) -> [F()];
 for(I, N, F) -> [F()|for(I+1, N, F)].
 
 */