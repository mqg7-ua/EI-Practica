# ============================================================
# Makefile para Práctica 2: Indexador (2025-2026)
# ============================================================

.PHONY: all clean test

# Compilador y opciones [cite: 537, 538]
CC = g++
OPTIONS = -g -Wall -Wextra
INCLUDEDIR = include
LIBDIR = lib
SRCDIR = src
TESTDIR = tadsPruebaIndexador

# Objetos necesarios para la práctica [cite: 542]
OBJ_FILES = indexadorHash.o tokenizador.o stemmer.o indexadorInformacion.o
OBJS = $(patsubst %, $(LIBDIR)/%, $(OBJ_FILES))

# Detección automática de tests [cite: 614]
# Buscamos todos los indexadorXX.cpp dentro de la carpeta de pruebas
TEST_SRCS = $(wildcard $(TESTDIR)/indexador*.cpp)
TEST_BINS = $(TEST_SRCS:.cpp=)

# --- REGLAS PRINCIPALES ---

all: indexador

# Compilación del programa principal [cite: 545, 549]
indexador: $(SRCDIR)/main.cpp $(OBJS)
	$(CC) $(OPTIONS) -I$(INCLUDEDIR) $(SRCDIR)/main.cpp $(OBJS) -o indexador

# Regla genérica para compilar objetos en la carpeta lib [cite: 554]
$(LIBDIR)/%.o: $(LIBDIR)/%.cpp $(INCLUDEDIR)/%.h
	$(CC) $(OPTIONS) -c -I$(INCLUDEDIR) $< -o $@

# --- SISTEMA DE PRUEBAS (TESTS) ---

# Compila cada binario de prueba individualmente
$(TESTDIR)/indexador%: $(TESTDIR)/indexador%.cpp $(OBJS)
	$(CC) $(OPTIONS) -I$(INCLUDEDIR) $< $(OBJS) -o $@

# Ejecuta todos los tests y muestra el diff detallado
test: $(TEST_BINS)
	@echo "=== INICIANDO BATERIA DE PRUEBAS CON DIFF DIRECTO ==="
	@for testbin in $(TEST_BINS) ; do \
		echo "------------------------------------------------" ; \
		echo "Probando: $$testbin" ; \
		./$$testbin > $$testbin.misal ; \
		if [ -f $$testbin.cpp.sal ]; then \
			diff -u -b -B $$testbin.cpp.sal $$testbin.misal > $$testbin.diff ; \
			if [ $$? -eq 0 ]; then \
				echo "✅ PASADO" ; \
			else \
				echo "❌ FALLADO. Diferencias encontradas:" ; \
				cat $$testbin.diff ; \
			fi ; \
		else \
			echo "⚠️ AVISO: No se encuentra el fichero esperado ($$testbin.cpp.sal)" ; \
		fi ; \
	done
	@echo "------------------------------------------------"
	@echo "=== FIN DE BATERIA DE PRUEBAS ==="

# Limpieza de archivos generados [cite: 557, 558]
clean:
	rm -f indexador
	rm -f $(OBJS)
	rm -f $(TEST_BINS)
	rm -f $(TESTDIR)/*.misal
	rm -f $(TESTDIR)/*.diff