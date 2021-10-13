## SOFT3410 Assignment1 -- implement pagerank in c and use pthread to perform multi-threading
This project implemented the basic page ranking algorithm in c. The program reads the core number, page number, pages' names, dampening factor, number of edges
and outputs the final scores when algorithm converges.

The pagerank.c has both sequential and parallelized version of program

pthread_create() and pthread_join() is used for multi-threading, and mutex lock is for atomicity.

comparison between sequential and parallel is in the report.

to run the program, type `make` under the same directory as makefile, tests will run and data would recored in .txt
