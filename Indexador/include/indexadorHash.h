#ifndef INDEXADORHASH_H
#define INDEXADORHASH_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "indexadorInformacion.h"
#include "tokenizador.h"

using namespace std;

class IndexadorHash {

    friend ostream& operator<<(ostream& s, const IndexadorHash& p) {
        s << "Fichero con el listado de palabras de parada: " << p.ficheroStopWords          << endl;
        s << "Tokenizador: "                                  << p.tok                       << endl;
        s << "Directorio donde se almacenara el indice generado: " << p.directorioIndice     << endl;
        s << "Stemmer utilizado: "                            << p.tipoStemmer               << endl;
        s << "Informacion de la coleccion indexada: "         << p.informacionColeccionDocs  << endl;
        s << "Se almacenaran las posiciones de los terminos: "<< p.almacenarPosTerm;
        return s;
    }

public:
    // ── Constructores / destructor / asignacion ──────────────────────────────
    IndexadorHash(const string& fichStopWords,
                  const string& delimitadores,
                  const bool&   detectComp,
                  const bool&   minuscSinAcentos,
                  const string& dirIndice,
                  const int&    tStemmer,
                  const bool&   almPosTerm);
    // Carga stop-words desde fichStopWords; inicializa el tokenizador con
    // delimitadores, detectComp y minuscSinAcentos; fija el directorio del
    // indice, el tipo de stemmer y si se almacenan posiciones.
    // Los indices quedan vacios.

    IndexadorHash(const string& directorioIndexacion);
    // Constructor que recupera una indexacion guardada previamente con
    // GuardarIndexacion(). Lanza excepcion si el directorio no existe o
    // los datos son incorrectos.

    IndexadorHash(const IndexadorHash&);

    ~IndexadorHash();

    IndexadorHash& operator=(const IndexadorHash&);

    // ── Indexacion de documentos ─────────────────────────────────────────────
    bool Indexar(const string& ficheroDocumentos);
    // Indexa los documentos listados (uno por linea) en ficheroDocumentos.
    // Los anade a los ya existentes. Reindexara documentos con fecha mas
    // reciente manteniendo el mismo idDoc. Devuelve false solo si se queda
    // sin memoria, dejando lo indexado hasta ese momento.

    bool IndexarDirectorio(const string& dirAIndexar);
    // Indexa todos los ficheros (recursivo) del directorio dirAIndexar.
    // Equivalente a llamar a Indexar con un fichero de lista generado con find.

    // ── Persistencia ────────────────────────────────────────────────────────
    bool GuardarIndexacion() const;
    // Guarda en directorioIndice todo el estado actual (indices, pregunta,
    // parametros, stop-words). Formato propio recuperable con
    // RecuperarIndexacion o el constructor de directorio.

    bool RecuperarIndexacion(const string& directorioIndexacion);
    // Vacia el estado actual y carga la indexacion almacenada en
    // directorioIndexacion. Devuelve false si no existe o esta corrupta.

    // ── Listados de diagnostico ─────────────────────────────────────────────
    void ImprimirIndexacion() const {
        cout << "Terminos indexados:" << endl;
        for (auto& par : indice)
            cout << par.first << '\t' << par.second << endl;
        cout << "Documentos indexados:" << endl;
        for (auto& par : indiceDocs)
            cout << par.first << '\t' << par.second << endl;
    }

    // ── Indexacion de pregunta ───────────────────────────────────────────────
    bool IndexarPregunta(const string& preg);
    // Indexa la pregunta preg en indicePregunta/infPregunta.
    // Vacia los indices de pregunta antes de comenzar.

    bool DevuelvePregunta(string& preg) const;
    // Devuelve true si hay pregunta indexada; copia su texto en preg.

    bool DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const;
    // Devuelve true si 'word' (transformada) esta en indicePregunta; copia inf.

    bool DevuelvePregunta(InformacionPregunta& inf) const;
    // Devuelve true si hay pregunta indexada; copia infPregunta en inf.

    void ImprimirIndexacionPregunta() {
        cout << "Pregunta indexada: " << pregunta << endl;
        cout << "Terminos indexados en la pregunta:" << endl;
        for (auto& par : indicePregunta)
            cout << par.first << '\t' << par.second << endl;
        cout << "Informacion de la pregunta: " << infPregunta << endl;
    }

    void ImprimirPregunta() {
        cout << "Pregunta indexada: " << pregunta << endl;
        cout << "Informacion de la pregunta: " << infPregunta << endl;
    }

    // ── Consulta del indice de documentos ───────────────────────────────────
    bool Devuelve(const string& word, InformacionTermino& inf) const;
    // Devuelve true si 'word' (transformada) esta indexada; copia inf.

    bool Devuelve(const string& word, const string& nomDoc, InfTermDoc& infDoc) const;
    // Devuelve true si 'word' aparece en nomDoc; copia infDoc.

    bool Existe(const string& word) const;
    // Devuelve true si 'word' (transformada) esta indexada.

    // ── Borrado ─────────────────────────────────────────────────────────────
    bool BorraDoc(const string& nomDoc);
    // Borra nomDoc del indice de terminos, de indiceDocs y actualiza
    // informacionColeccionDocs. Devuelve true si existia.

    void VaciarIndiceDocs();
    // Borra toda la indexacion de documentos.

    void VaciarIndicePreg();
    // Borra la indexacion de la pregunta actual.

    // ── Getters de configuracion ─────────────────────────────────────────────
    int    NumPalIndexadas()           const;
    string DevolverFichPalParada()     const;
    void   ListarPalParada()           const;
    int    NumPalParada()              const;
    string DevolverDelimitadores()     const;
    bool   DevolverCasosEspeciales()   const;
    bool   DevolverPasarAminuscSinAcentos() const;
    bool   DevolverAlmacenarPosTerm()  const;
    string DevolverDirIndice()         const;
    int    DevolverTipoStemming()      const;

    // ── Listados ─────────────────────────────────────────────────────────────
    void ListarInfColeccDocs() const;
    void ListarTerminos()      const;
    bool ListarTerminos(const string& nomDoc) const;
    void ListarDocs()          const;
    bool ListarDocs(const string& nomDoc) const;

private:
    IndexadorHash(); // prohibido: no se puede crear sin inicializar

    // ── Indices ──────────────────────────────────────────────────────────────
    unordered_map<string, InformacionTermino>       indice;
    unordered_map<string, InfDoc>                   indiceDocs;
    InfColeccionDocs                                informacionColeccionDocs;

    // ── Pregunta ─────────────────────────────────────────────────────────────
    string                                          pregunta;
    unordered_map<string, InformacionTerminoPregunta> indicePregunta;
    InformacionPregunta                             infPregunta;

    // ── Configuracion ────────────────────────────────────────────────────────
    unordered_set<string> stopWords;     // stop-words transformadas (minusc + stem)
    string                ficheroStopWords;
    Tokenizador           tok;
    string                directorioIndice;
    int                   tipoStemmer;   // 0=ninguno, 1=Porter-es, 2=Porter-en
    bool                  almacenarPosTerm;

    // ── Metodos auxiliares privados ──────────────────────────────────────────
    string AplicarStemmer  (const string& palabra) const;
    string TransformarTerm (const string& tok_out)  const; // stemmer sobre token ya tokenizado
    void   CargarStopWords (const string& fichero);
    bool   IndexarDocumento(const string& nomDoc, int idDoc,
                            long tamBytes, const Fecha& fecha);
    int    SiguienteIdDoc  () const; // max(idDoc)+1
};

#endif // INDEXADORHASH_H