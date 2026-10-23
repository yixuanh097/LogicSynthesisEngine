CC := g++
FLAGS := -std=c++17 -Wall -Wextra -Werror 

all: parser.o analyzer.o verilog_generation.o
	$(CC) $(FLAGS) -o main main.cpp parser.o analyzer.o verilog_generation.o
parser.o: parser.cpp
	$(CC) $(FLAGS) -o parser.o -c parser.cpp
analyzer.o: analyzer.cpp
	$(CC) $(FLAGS) -o analyzer.o -c analyzer.cpp
verilog_generation.o: verilog_generation.cpp
	$(CC) $(FLAGS) -o verilog_generation.o -c verilog_generation.cpp

clean:
	rm *.o