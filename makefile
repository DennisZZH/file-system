all: sample_grader

sample_grader:autograder_main.c disk fs 
	g++ -g autograder_main.c disk.o fs.o -o sample_grader

# test1:test1.cpp threads tls
# 	g++ -g test1.cpp threads.o tls.o -o test1

# test3:test3.cpp threads tls
# 	g++ -g test3.cpp threads.o tls.o -o test3
fs:	fs.cpp
	g++ -g -c fs.cpp -o fs.o

disk: disk.c
	g++ -g -c disk.c -o disk.o

clean:
	rm fs.o
	rm disk.o
	rm sample_grader

# clean_test1:
# 	rm threads.o
# 	rm tls.o
# 	rm test1

# clean_test3:
# 	rm threads.o
# 	rm tls.o
# 	rm test3

