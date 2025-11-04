CC = g++
LIB = -lcrypto -lz -lstdc++ -llz4 -lpthread -lstdc++fs -lm -lisal_crypto -lzip -L./utils/lz4-1.9.1/lib
SRC = 	./src/main.cpp \
		./src/Router.cpp \
		./src/LocalDeduplicator.cpp \
		./utils/cJSON.c \
		./src/ramcdc.cpp

EXE_NAME = cRouting
INC = -I./include -I./utils 
OPTION = -g -O0 -std=c++17 -mavx512bw
CXXFLAGS="-fdiagnostics-color=always"

amazing:
	$(CC) -o $(EXE_NAME) $(SRC) $(INC) $(LIB) $(OPTION) $(CXXFLAGS)

clean:
	rm $(EXE_NAME)