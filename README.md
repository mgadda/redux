Redux
=====

Redux is a playground in which I have been experimenting with compiler development using LLVM. I can't promise you'll find HEAD in a working state, but there is a Makefile which you can use to compile redux. You're far better off reading the code than attempting to make redux do something useful for you.

But if you insist:

Usage: redux [options] file...  
Options:  
-d  Extremely erbose output  
-i  Launch interactive mode  
-c  Compile redux files specified by filename and output  
-z  Enable LLVM code optimizations  

redux will parse and just-in-time compile anything it finds on STDIN. 
