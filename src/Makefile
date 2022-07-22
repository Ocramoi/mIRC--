##
# mIRC--
#
# @file
# @version 1.0

CC=clang++
SRC_FILES=main.cpp $(wildcard **/*.cpp)
SRC_FILES:=$(filter-out $(wildcard **/flycheck_*.cpp), $(SRC_FILES))
EXEC=../build/main
DEXEC=../build/debug
OBJ_FOLDER=objs
COMPILE_FLAGS=-Wextra -Ofast -std=gnu++17
DEBUG_FLAGS=-Wextra -Og -g3 -DDEBUG -std=gnu++17
# VALGRIND_FLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes -s
VALGRIND_FLAGS=
LD_FLAGS:=-lpthread

all: setup main.o Client.o Server.o Utils.o
	$(CC) $(COMPILE_FLAGS) *.o -o $(EXEC) $(LD_FLAGS)

main.o: main.cpp
	$(CC) $(COMPILE_FLAGS) main.cpp -c

Client.o: Client/Client.cpp
	$(CC) $(COMPILE_FLAGS) Client/Client.cpp -c

Utils.o: Utils/Utils.cpp
	$(CC) $(COMPILE_FLAGS) Utils/Utils.cpp -c

Server.o: Server/Server.cpp Channel.o
	$(CC) $(COMPILE_FLAGS) Server/Server.cpp -c

Channel.o: Channel/Channel.cpp
	$(CC) $(COMPILE_FLAGS) Channel/Channel.cpp -c

setup:
	rm -rf build
	mkdir build

run:
	./$(EXEC)

drun:
	./$(DEXEC)

debug:
	$(CC) $(DEBUG_FLAGS) main.cpp $(filter-out $(wildcard **/flycheck_*.cpp), $(wildcard **/*.cpp)) -o $(DEXEC) $(LD_FLAGS)

val:
	valgrind $(VALGRIND_FLAGS) ./$(DEXEC)

clear:
	rm *.o

.PHONY: all clear val run setup

# end
