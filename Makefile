OBJS = pthread.o

pthread: $(OBJS)
	gcc -o pthread $(OBJS)
pthread.o: pthread.c 

.PHONY: clean
clean : 
	rm -f pthread $(OBJS)