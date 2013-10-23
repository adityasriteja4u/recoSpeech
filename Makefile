all:
	gcc -o roger -std=c99 `pkg-config --cflags --libs pocketsphinx` roger.c 
clean:
	rm roger 
