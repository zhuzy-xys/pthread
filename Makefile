OBJS = pthread.o httplib.o

pthread: $(OBJS)
	gcc -o pthread $(OBJS)
pthread.o: pthread.c httplib.h
httplib.o: httplib.h httplib.c

.PHONY: clean
clean : 
	rm -f pthread $(OBJS)