# Memoria de la Práctica 3 – Buscador

## Archivos creados y modificados

### Modificados

#### `include/indexadorHash.h`
- La sección `private:` de los **datos miembro** se ha dividido en dos bloques:
  - `protected:` para los datos (índices, pregunta, configuración) y el constructor por defecto `IndexadorHash()`.  
    *Motivo*: `Buscador` hereda de `IndexadorHash` y necesita acceso directo a `indice`, `indiceDocs`, `indicePregunta`, `informacionColeccionDocs`, etc. para implementar las fórmulas de scoring. Sin `protected`, la herencia no es suficiente para acceder a miembros privados.
  - `private:` exclusivo para los métodos auxiliares internos (`TransformarTerm`, `CargarStopWords`, `IndexarDocumento`, `SiguienteIdDoc`).  
    *Motivo*: estos métodos son detalles de implementación que no necesita usar `Buscador`.

#### `include/buscador.h`
- Se añade el **`#ifndef BUSCADOR_H`** guard (faltaba).
- Se añade `#include <queue>` (necesario para `priority_queue`).
- Se define **`struct ResultadoRI`** antes de la clase `Buscador`:
  - Campos: `numPregunta`, `idDoc`, `vSimilitud`.
  - `operator<` compara por `vSimilitud` → la `priority_queue` actúa como max-heap (mayor puntuación = mayor prioridad).
  - Era imprescindible: `Buscador` usa `priority_queue<ResultadoRI>` en su cuerpo pero el tipo no estaba definido en ningún header proporcionado.
- Se corrige el `operator<<` amigo: `DevuelvePregunta(preg)` → `p.DevuelvePregunta(preg)` (función libre, necesita objeto explícito).
- Se limpian los comentarios con caracteres corruptos (ISO-8859 mezclado con UTF-8).
- Se declaran dos métodos privados auxiliares:
  - `ScoreDocumentos(int numPregunta, int numDocumentos)` – factoriza el scoring.
  - `PrintResultsTo(ostream& s, int numDocumentos) const` – factoriza la salida.

---

### Creados

#### `buscador.cpp`
Implementación completa de la clase `Buscador`.

**Constructores / destructor / asignación**
- `Buscador()` (privado): inicializa `formSimilitud=0`, `c=2`, `k1=1.2`, `b=0.75`. Llama a `IndexadorHash()`.
- `Buscador(directorioIndexacion, f)`: delega la carga al constructor de `IndexadorHash`; fija los parámetros del modelo.
- Copia / asignación: copian también `docsOrdenados` y los parámetros del modelo.

**`ScoreDocumentos(numPregunta, numDocumentos)` — núcleo del sistema de RI**

Precondición: `indicePregunta` ya está cargada (`IndexarPregunta` ha sido llamada).

1. Construye un mapa inverso `idDoc → numPalSinParada` a partir de `indiceDocs` para acceso O(1).
2. Itera sobre cada término de `indicePregunta`:
   - Lo busca en `indice` (índice del corpus). Si no aparece, lo salta.
   - Para cada documento `d` que contiene el término:
     - **BM25 (Okapi)**  
       `idf = log((N − df + 0.5) / (df + 0.5))`  
       `tf_norm = tf · (k1+1) / (tf + k1·(1 − b + b·dl/avgdl))`  
       `contrib = idf · tf_norm`
     - **DFR In_expC2 con Normalización 2**  
       `idf = log₂((N+1) / (df+0.5))`  
       `tfn = tf · log₂(1 + c · avgdl / dl)`  
       `contrib = qtf · (tfn / (tfn+1)) · idf`  
       (`qtf` = frecuencia del término en la query)
3. Acumula puntuaciones en `unordered_map<idDoc, score>`.
4. Ordena descendente, toma top-`numDocumentos` y los introduce en `docsOrdenados` con el `numPregunta` indicado.
5. Captura `bad_alloc`: imprime a `cerr` el último documento y término procesados y devuelve `false`.

**`Buscar(numDocumentos)`**
- Vacía `docsOrdenados`.
- Comprueba que `infPregunta` tiene términos válidos; si no, devuelve `false`.
- Llama a `ScoreDocumentos(0, numDocumentos)` (`numPregunta=0` = búsqueda simple).

**`Buscar(dirPreguntas, numDocumentos, numPregInicio, numPregFin)`**
- Vacía `docsOrdenados`.
- Bucle de `numPregInicio` a `numPregFin`: abre `dirPreguntas/i.txt`, lee el texto, llama a `IndexarPregunta` y a `ScoreDocumentos(i, numDocumentos)`.
- Si `ScoreDocumentos` devuelve `false`, añade el número de pregunta al mensaje de error y devuelve `false`.

**`PrintResultsTo(ostream& s, int numDocumentos)`**
- Construye mapa inverso `idDoc → ruta` desde `indiceDocs`.
- Copia `docsOrdenados` en local (la función es `const`).
- Agrupa resultados por `numPregunta` con `std::map` (orden ascendente de preguntas garantizado).
- Dentro de cada grupo, ordena por `vSimilitud` descendente.
- Imprime en el formato:
  ```
  NumPregunta Formula NomDoc Posicion Puntuacion PregIndexada
  ```
  - `NomDoc`: nombre del fichero sin ruta ni extensión.
  - `Puntuacion`: `setprecision(15)` con separador decimal forzado a `.` vía `imbue(locale("C"))`.
  - `PregIndexada`: si `numPregunta==0` → texto de `pregunta`; si no → `"ConjuntoDePreguntas"`.

**Getters / setters**
- `DevolverFormulaSimilitud`, `CambiarFormulaSimilitud` (valida 0 ó 1).
- `CambiarParametrosDFR` / `DevolverParametrosDFR` (parámetro `c`).
- `CambiarParametrosBM25` / `DevolverParametrosBM25` (parámetros `k1`, `b`).

#### `Makefile`
Compila los cinco `.cpp` (uno propio + cuatro de la biblioteca) con `-std=c++11 -O2` y genera `libbuscador.a`. Para enlazar con un `main.cpp` externo: `make MAIN=main.cpp ejecutable`.

---

## Fórmulas implementadas

| Parámetro | Valor por defecto | Significado |
|-----------|-------------------|-------------|
| `c`       | 2.0               | Control de normalización de longitud en DFR |
| `k1`      | 1.2               | Saturación de la frecuencia de término en BM25 |
| `b`       | 0.75              | Peso de la normalización de longitud en BM25 |

Las variables de la colección usadas en ambas fórmulas:
- **N** = `informacionColeccionDocs.ObtenerNumDocs()`
- **avgdl** = `numTotalPalSinParada / N`
- **dl** = `InfDoc.ObtenerNumPalSinParada()` (longitud del documento sin palabras de parada)
- **df** = `InformacionTermino.ObtenerFd()` (número de documentos que contienen el término)
- **tf** = `InfTermDoc.ObtenerFt()` (frecuencia del término en el documento)

La evaluación se realiza con `trec_eval -q -o frelevancia.txt salida_buscador.txt`.
