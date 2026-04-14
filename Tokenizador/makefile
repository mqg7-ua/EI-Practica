.PHONY: clean

CC=g++
OPTIONS=-g
DEBUG=#-D DEBUG
LIBDIR=lib
INCLUDEDIR=include
OBJ= tokenizador.o
OBJ_PATH=$(patsubst %,$(LIBDIR)/%,$(OBJ))

all: practical

practical: src/main.cpp $(OBJ_PATH)
	$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ_PATH) -o practica1

$(LIBDIR)/%.o: $(LIBDIR)/%.cpp $(INCLUDEDIR)/%.h
	$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<

clean:
	rm -f $(OBJ_PATH)