#ifndef BUSCADOR_H
#define BUSCADOR_H

#include <indexadorHash.h>
#include <indexadorInformacion.h>
#include <stemmer.h>
#include <tokenizador.h>
#include <queue>

// Resultado de una busqueda: puntuacion de un documento para una pregunta
struct ResultadoRI {
    int    numPregunta; // 0 para busqueda simple, >0 para busqueda por lotes
    int    idDoc;
    double vSimilitud;

    bool operator<(const ResultadoRI& o) const {
        return vSimilitud < o.vSimilitud; // max-heap: mayor puntuacion = mayor prioridad
    }
};

class Buscador: public IndexadorHash {

    friend ostream& operator<<(ostream& s, const Buscador& p) {
        string preg;
        s << "Buscador: " << endl;
        if (p.DevuelvePregunta(preg))
            s << "\tPregunta indexada: " << preg << endl;
        else
            s << "\tNo hay ninguna pregunta indexada" << endl;
        s << "\tDatos del indexador: " << endl << (IndexadorHash) p;
        return s;
    }

public:
    // Carga indexacion desde directorioIndexacion; inicializa formSimilitud=f,
    // c=2, k1=1.2, b=0.75. Si el directorio no existe mantiene indices vacios.
    Buscador(const string& directorioIndexacion, const int& f);

    Buscador(const Buscador&);

    ~Buscador();

    Buscador& operator=(const Buscador&);

    // Busca con la pregunta actualmente indexada. Guarda top-numDocumentos en
    // docsOrdenados (numPregunta=0). Devuelve false si no hay pregunta valida
    // o si falla por falta de memoria.
    bool Buscar(const int& numDocumentos = 99999);

    // Busca todas las preguntas [numPregInicio, numPregFin] del directorio
    // dirPreguntas (ficheros i.txt). Acumula resultados en docsOrdenados.
    // Devuelve false si falla por falta de memoria.
    bool Buscar(const string& dirPreguntas, const int& numDocumentos,
                const int& numPregInicio, const int& numPregFin);

    // Imprime por pantalla los resultados almacenados en docsOrdenados.
    // Formato por linea: NumPregunta Formula NomDoc Posicion Puntuacion PregIndexada
    // Maximo numDocumentos lineas por pregunta. Decimales con punto.
    void ImprimirResultadoBusqueda(const int& numDocumentos = 99999) const;

    // Igual que ImprimirResultadoBusqueda pero guarda en fichero nombreFichero.
    // Devuelve false si no puede crear el fichero.
    bool ImprimirResultadoBusqueda(const int& numDocumentos,
                                   const string& nombreFichero) const;

    int  DevolverFormulaSimilitud() const;

    // Cambia formSimilitud a f (0=DFR, 1=BM25). Devuelve false si f invalido.
    bool CambiarFormulaSimilitud(const int& f);

    void   CambiarParametrosDFR(const double& kc);
    double DevolverParametrosDFR() const;

    void CambiarParametrosBM25(const double& kk1, const double& kb);
    void DevolverParametrosBM25(double& kk1, double& kb) const;

private:
    Buscador();

    priority_queue<ResultadoRI> docsOrdenados;

    int    formSimilitud; // 0: DFR (In_expC2), 1: BM25
    double c;             // parametro normalizacion DFR
    double k1;            // parametro saturacion BM25
    double b;             // parametro normalizacion longitud BM25

    // Calcula scores con indicePregunta ya cargada y añade top-numDocumentos
    // a docsOrdenados con el numPregunta indicado.
    bool ScoreDocumentos(int numPregunta, int numDocumentos);

    // Factoriza la salida de resultados hacia cualquier ostream.
    void PrintResultsTo(ostream& s, int numDocumentos) const;
};

#endif // BUSCADOR_H
