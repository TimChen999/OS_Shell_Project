all:
	gcc -w main.c -lreadline -g

#gcc main builds main, -lreadline gets readline library, -w ignores error, -o decides name of object file it generates
#gcc -o goo gabagoo.c -g compiles gabagoo.c into goo.out
#gdb allows for debug (read more on it). (gdb) b main.c:5 sets breakpoint at line 5 of main.c
#tui enable for gui debugger to show. tui d to disable

