#ifndef INDEXADORINFORMACION_H
#define INDEXADORINFORMACION_H

#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
#include <ctime>
#include <cstring>

using namespace std;

// ============================================================
//  Fecha: fecha y hora de modificacion del fichero
// ============================================================
struct Fecha {
    time_t tiempo;

    Fecha(){tiempo = 0; }
    Fecha(time_t t){tiempo = t;}

    bool esMasReciente(const Fecha& o) const { return tiempo > o.tiempo; }

    friend ostream& operator<<(ostream& s, const Fecha& f) {
        char buf[32] = "";
        struct tm* tm_info = localtime(&f.tiempo);
        if (tm_info) strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
        s << buf;
        return s;
    }
};


// ============================================================
//  InfTermDoc: informacion de un termino en un documento
// ============================================================
class InfTermDoc {
    friend ostream& operator<<(ostream& s, const InfTermDoc& p);
    friend class IndexadorHash;
public:
    InfTermDoc();
    InfTermDoc(const InfTermDoc&);
    ~InfTermDoc();
    InfTermDoc& operator=(const InfTermDoc&);

    // Registra una nueva ocurrencia del termino en el documento.
    // 'posicion' es el numero de palabra (0-based, incluyendo stop-words).
    // Solo se almacena si guardarPos == true.
    void AnadirOcurrencia(int posicion, bool guardarPos);

    int              ObtenerFt()      const { return ft;      }
    const list<int>& ObtenerPosTerm() const { return posTerm; }

private:
    int       ft;       // frecuencia del termino en el documento
    list<int> posTerm;  // posiciones (se numeran tambien las stop-words)
};


// ============================================================
//  InformacionTermino: informacion global de un termino indexado
// ============================================================
class InformacionTermino {
    friend ostream& operator<<(ostream& s, const InformacionTermino& p);
    friend class IndexadorHash;
public:
    InformacionTermino();
    InformacionTermino(const InformacionTermino&);
    ~InformacionTermino();
    InformacionTermino& operator=(const InformacionTermino&);

    // Registra una ocurrencia del termino en el documento 'idDoc'
    // en la posicion 'posicion'.
    void AnadirOcurrenciaDoc(int idDoc, int posicion, bool guardarPos);

    // Elimina toda la informacion del documento 'idDoc'.
    void EliminarDoc(int idDoc);

    int  ObtenerFtc() const { return ftc; }
    int  ObtenerFd()  const { return (int)l_docs.size(); }
    const unordered_map<int, InfTermDoc>& ObtenerDocs() const { return l_docs; }
    bool ExisteDoc(int idDoc) const { return l_docs.count(idDoc) > 0; }

private:
    int ftc;                               // frecuencia total en la coleccion
    unordered_map<int, InfTermDoc> l_docs; // InfTermDoc indexado por idDoc
};


// ============================================================
//  InfDoc: informacion de un documento indexado
// ============================================================
class InfDoc {
    friend ostream& operator<<(ostream& s, const InfDoc& p);
    friend class IndexadorHash;
public:
    InfDoc();
    InfDoc(const InfDoc&);
    ~InfDoc();
    InfDoc& operator=(const InfDoc&);

    void Inicializar(int id, int nPal, int nPalSinP,
                     int nPalDif, int bytes, const Fecha& f);

    int          ObtenerIdDoc()           const { return idDoc;            }
    int          ObtenerNumPal()          const { return numPal;           }
    int          ObtenerNumPalSinParada() const { return numPalSinParada;  }
    int          ObtenerNumPalDif()       const { return numPalDiferentes; }
    int          ObtenerTamBytes()        const { return tamBytes;         }
    const Fecha& ObtenerFecha()           const { return fechaModificacion;}

private:
    int   idDoc;            // identificador (desde 1)
    int   numPal;           // total palabras (incluye stop-words)
    int   numPalSinParada;  // total palabras sin stop-words
    int   numPalDiferentes; // diferentes sin stop-words
    int   tamBytes;         // tamano en bytes
    Fecha fechaModificacion;
};


// ============================================================
//  InfColeccionDocs: estadisticas globales de la coleccion
// ============================================================
class InfColeccionDocs {
    friend ostream& operator<<(ostream& s, const InfColeccionDocs& p);
    friend class IndexadorHash;
public:
    InfColeccionDocs();
    InfColeccionDocs(const InfColeccionDocs&);
    ~InfColeccionDocs();
    InfColeccionDocs& operator=(const InfColeccionDocs&);

    void AnadirDoc   (const InfDoc& doc);
    void EliminarDoc (const InfDoc& doc);
    void AjustarPalDiferentes(int delta); // +1 termino nuevo / -1 termino eliminado
    void Vaciar();

    int ObtenerNumDocs()               const { return numDocs;               }
    int ObtenerNumTotalPal()           const { return numTotalPal;           }
    int ObtenerNumTotalPalSinParada()  const { return numTotalPalSinParada;  }
    int ObtenerNumTotalPalDiferentes() const { return numTotalPalDiferentes; }
    int ObtenerTamBytes()              const { return tamBytes;              }

private:
    int numDocs;
    int numTotalPal;
    int numTotalPalSinParada;
    int numTotalPalDiferentes;
    int tamBytes;
};


// ============================================================
//  InformacionTerminoPregunta
// ============================================================
class InformacionTerminoPregunta {
    friend ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p);
    friend class IndexadorHash;
public:
    InformacionTerminoPregunta();
    InformacionTerminoPregunta(const InformacionTerminoPregunta&);
    ~InformacionTerminoPregunta();
    InformacionTerminoPregunta& operator=(const InformacionTerminoPregunta&);

    void AnadirOcurrencia(int posicion, bool guardarPos);

    int              ObtenerFt()      const { return ft;      }
    const list<int>& ObtenerPosTerm() const { return posTerm; }

private:
    int       ft;
    list<int> posTerm;
};


// ============================================================
//  InformacionPregunta
// ============================================================
class InformacionPregunta {
    friend ostream& operator<<(ostream& s, const InformacionPregunta& p);
    friend class IndexadorHash; //Para no tener que hacer tantos getters y setters
public:
    InformacionPregunta();
    InformacionPregunta(const InformacionPregunta&);
    ~InformacionPregunta();
    InformacionPregunta& operator=(const InformacionPregunta&);

    void Inicializar(int nPal, int nPalSinP, int nPalDif);
    void Vaciar();
    bool EstaVacia() const;

    int ObtenerNumTotalPal()           const { return numTotalPal;           }
    int ObtenerNumTotalPalSinParada()  const { return numTotalPalSinParada;  }
    int ObtenerNumTotalPalDiferentes() const { return numTotalPalDiferentes; }

private:
    int numTotalPal;
    int numTotalPalSinParada;
    int numTotalPalDiferentes;
};

#endif // INDEXADORINFORMACION_H