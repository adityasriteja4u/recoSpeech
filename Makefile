all:
	gcc -o roger -std=c99 `pkg-config --cflags --libs pocketsphinx` roger.c 
# for convenience because tramsmitters differentiate signals by the case of the messages
capsmode:
	gcc -o roger -std=c99 -DCAPSMODE `pkg-config --cflags --libs pocketsphinx` roger.c 
static:
	gcc -o roger -std=c99 -static -L ../local_root/lib `pkg-config --cflags --libs pocketsphinx` roger.c 
clean:
	rm roger 
