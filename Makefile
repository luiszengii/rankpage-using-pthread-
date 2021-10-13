CC=gcc
CFLAGS=-g -std=c11 -Wall -Werror -O0
TARGET=program
.PHONY: clean
all: pagerank permit test record clean
# all: pagerank

pagerank: pagerank.c pagerank.h
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lm -lrt

permit:
	chmod a+x ./test.sh
	chmod a+x ./record_data.sh

test: test.sh
	./test.sh

record: record_data.sh
	./record_data.sh

clean:
	rm -f *.o
	rm -f pagerank
	rm -f test