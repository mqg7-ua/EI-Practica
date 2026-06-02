CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -I./include
LDFLAGS  =

# Fuentes propias + libreria proporcionada
SRCS = lib/buscador.cpp \
       lib/indexadorHash.cpp \
       lib/indexadorInformacion.cpp \
       lib/tokenizador.cpp \
       lib/stemmer.cpp

OBJS = $(SRCS:.cpp=.o)

# Destino por defecto: biblioteca estática (para enlazar con el main del evaluador)
TARGET = libbuscador.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Si hay un main.cpp en la raiz, compila el ejecutable
ifdef MAIN
ejecutable: $(OBJS) $(MAIN)
	$(CXX) $(CXXFLAGS) $(OBJS) $(MAIN) -o buscador_exec $(LDFLAGS)
endif

clean:
	rm -f $(OBJS) $(TARGET) buscador_exec
