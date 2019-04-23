# Files to compile
_OBJECTS = inet/tcpclient.o inet/tlsclient.o inet/httptypes.o inet/httpclient.o inet/http2types.o inet/http2client.o inet/websocket.o \
	media/jsontypes.o media/json.o main.o
_TARGET = network

# The directories where to find the source files
BIN = ./bin/
SRC = ./src/

# How to compile the files
CCX = clang++
CXFLAGS = -std=c++17 -Wall -pedantic -g

# Path to files
OBJECTS = $(addprefix $(BIN), $(_OBJECTS))
TARGET = $(addprefix $(BIN), $(_TARGET))
LIBS = -lcrypto -lssl

.DEFAULT_GOAL = all

$(BIN)%.o: $(SRC)%.cpp
	$(CCX) $(CXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CCX) -o $(TARGET) $(OBJECTS) $(LIBS)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)
