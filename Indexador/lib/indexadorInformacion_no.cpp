/**
 * @file   indexadorInformacion.cpp
 * @brief  Implementacion completa del indexador invertido con tabla hash.
 *
 * Implementa las clases:
 *   InfTermDoc, InformacionTermino, InfDoc, InfColeccionDocs,
 *   InformacionTerminoPregunta, InformacionPregunta, IndexadorHash
 *
 * Formato de persistencia (GuardarIndexacion / RecuperarIndexacion):
 *   El indice se guarda en el fichero "indice.idx" dentro de directorioIndice.
 *   Usa un formato de texto propio con secciones delimitadas.
 *
 * Stemmer:
 *   Para habilitar Porter, compila con -DCON_STEMMER e incluye stemmer.h/cpp.
 *   Sin ese flag los terminos se indexan sin stemming (tipoStemmer se ignora).
 */

#include "indexadorHash.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <vector>

#ifdef CON_STEMMER
#  include "stemmer.h"
#endif

using namespace std;

// ============================================================
//  CONSTANTES INTERNAS
// ============================================================
static const string FICHERO_INDICE = "indice.idx";

// ============================================================
//  STEMMER (wrapper configurable)
// ============================================================

/**
 * @brief Aplica el stemmer de Porter si esta disponible.
 *
 * Si el proyecto se compila SIN -DCON_STEMMER la funcion es identidad.
 * Si se compila CON -DCON_STEMMER llama a la funcion global stemmer()
 * declarada en stemmer.h (interface: string stemmer(const string&, int tipo)).
 */
string IndexadorHash::AplicarStemmer(const string& palabra) const {
#ifdef CON_STEMMER
    if (tipoStemmer != 0)
        return stemmer(palabra, tipoStemmer);
#endif
    return palabra;
}

/**
 * @brief Aplica el stemmer a un token ya procesado por el tokenizador
 *        (que ya habra aplicado minuscSinAcentos si corresponde).
 */
string IndexadorHash::TransformarTerm(const string& t) const {
    return AplicarStemmer(t);
}


  //////////////////////////////////////////////////////
 //     InfTermDoc                                   //
//////////////////////////////////////////////////////

InfTermDoc::InfTermDoc() : ft(0) {}

InfTermDoc::InfTermDoc(const InfTermDoc& o) : ft(o.ft), posTerm(o.posTerm) {}

InfTermDoc::~InfTermDoc() { ft = 0; }

InfTermDoc& InfTermDoc::operator=(const InfTermDoc& o) {
    if (this != &o) { ft = o.ft; posTerm = o.posTerm; }
    return *this;
}

ostream& operator<<(ostream& s, const InfTermDoc& p) {
    s << "ft: " << p.ft;
    bool primero = true;
    for (int pos : p.posTerm) {
        s << '\t' << pos;
        primero = false;
    }
    return s;
}

void InfTermDoc::AnadirOcurrencia(int posicion, bool guardarPos) {
    ft++;
    if (guardarPos)
        posTerm.push_back(posicion);
}


  //////////////////////////////////////////////////////
 //     InformacionTermino                           //
//////////////////////////////////////////////////////

InformacionTermino::InformacionTermino() : ftc(0) {}

InformacionTermino::InformacionTermino(const InformacionTermino& o)
    : ftc(o.ftc), l_docs(o.l_docs) {}

InformacionTermino::~InformacionTermino() { ftc = 0; l_docs.clear(); }

InformacionTermino& InformacionTermino::operator=(const InformacionTermino& o) {
    if (this != &o) { ftc = o.ftc; l_docs = o.l_docs; }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionTermino& p) {
    s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
    for (auto& par : p.l_docs)
        s << "\tId.Doc: " << par.first << '\t' << par.second;
    return s;
}

void InformacionTermino::AnadirOcurrenciaDoc(int idDoc, int posicion, bool guardarPos) {
    ftc++;
    l_docs[idDoc].AnadirOcurrencia(posicion, guardarPos);
}

void InformacionTermino::EliminarDoc(int idDoc) {
    auto it = l_docs.find(idDoc);
    if (it != l_docs.end()) {
        ftc -= it->second.ObtenerFt();
        l_docs.erase(it);
    }
}


  //////////////////////////////////////////////////////
 //     InfDoc                                       //
//////////////////////////////////////////////////////

InfDoc::InfDoc()
    : idDoc(0), numPal(0), numPalSinParada(0),
      numPalDiferentes(0), tamBytes(0) {}

InfDoc::InfDoc(const InfDoc& o)
    : idDoc(o.idDoc), numPal(o.numPal),
      numPalSinParada(o.numPalSinParada),
      numPalDiferentes(o.numPalDiferentes),
      tamBytes(o.tamBytes),
      fechaModificacion(o.fechaModificacion) {}

InfDoc::~InfDoc() { idDoc = numPal = numPalSinParada = numPalDiferentes = tamBytes = 0; }

InfDoc& InfDoc::operator=(const InfDoc& o) {
    if (this != &o) {
        idDoc             = o.idDoc;
        numPal            = o.numPal;
        numPalSinParada   = o.numPalSinParada;
        numPalDiferentes  = o.numPalDiferentes;
        tamBytes          = o.tamBytes;
        fechaModificacion = o.fechaModificacion;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InfDoc& p) {
    s << "idDoc: "              << p.idDoc
      << "\tnumPal: "           << p.numPal
      << "\tnumPalSinParada: "  << p.numPalSinParada
      << "\tnumPalDiferentes: " << p.numPalDiferentes
      << "\ttamBytes: "         << p.tamBytes
      << "\tfecha: "            << p.fechaModificacion;
    return s;
}

void InfDoc::Inicializar(int id, int nPal, int nPalSinP,
                          int nPalDif, int bytes, const Fecha& f) {
    idDoc             = id;
    numPal            = nPal;
    numPalSinParada   = nPalSinP;
    numPalDiferentes  = nPalDif;
    tamBytes          = bytes;
    fechaModificacion = f;
}


  //////////////////////////////////////////////////////
 //     InfColeccionDocs                             //
//////////////////////////////////////////////////////

InfColeccionDocs::InfColeccionDocs()
    : numDocs(0), numTotalPal(0), numTotalPalSinParada(0),
      numTotalPalDiferentes(0), tamBytes(0) {}

InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs& o)
    : numDocs(o.numDocs), numTotalPal(o.numTotalPal),
      numTotalPalSinParada(o.numTotalPalSinParada),
      numTotalPalDiferentes(o.numTotalPalDiferentes),
      tamBytes(o.tamBytes) {}

InfColeccionDocs::~InfColeccionDocs() { Vaciar(); }

InfColeccionDocs& InfColeccionDocs::operator=(const InfColeccionDocs& o) {
    if (this != &o) {
        numDocs               = o.numDocs;
        numTotalPal           = o.numTotalPal;
        numTotalPalSinParada  = o.numTotalPalSinParada;
        numTotalPalDiferentes = o.numTotalPalDiferentes;
        tamBytes              = o.tamBytes;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InfColeccionDocs& p) {
    s << "numDocs: "                  << p.numDocs
      << "\tnumTotalPal: "            << p.numTotalPal
      << "\tnumTotalPalSinParada: "   << p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: "  << p.numTotalPalDiferentes
      << "\ttamBytes: "               << p.tamBytes;
    return s;
}

void InfColeccionDocs::AnadirDoc(const InfDoc& doc) {
    numDocs++;
    numTotalPal          += doc.ObtenerNumPal();
    numTotalPalSinParada += doc.ObtenerNumPalSinParada();
    tamBytes             += doc.ObtenerTamBytes();
    // numTotalPalDiferentes se gestiona por separado via AjustarPalDiferentes
}

void InfColeccionDocs::EliminarDoc(const InfDoc& doc) {
    numDocs--;
    numTotalPal          -= doc.ObtenerNumPal();
    numTotalPalSinParada -= doc.ObtenerNumPalSinParada();
    tamBytes             -= doc.ObtenerTamBytes();
}

void InfColeccionDocs::AjustarPalDiferentes(int delta) {
    numTotalPalDiferentes += delta;
}

void InfColeccionDocs::Vaciar() {
    numDocs = numTotalPal = numTotalPalSinParada =
    numTotalPalDiferentes = tamBytes = 0;
}


  //////////////////////////////////////////////////////
 //     InformacionTerminoPregunta                   //
//////////////////////////////////////////////////////

InformacionTerminoPregunta::InformacionTerminoPregunta() : ft(0) {}

InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta& o)
    : ft(o.ft), posTerm(o.posTerm) {}

InformacionTerminoPregunta::~InformacionTerminoPregunta() { ft = 0; }

InformacionTerminoPregunta& InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta& o) {
    if (this != &o) { ft = o.ft; posTerm = o.posTerm; }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
    s << "ft: " << p.ft;
    for (int pos : p.posTerm)
        s << '\t' << pos;
    return s;
}

void InformacionTerminoPregunta::AnadirOcurrencia(int posicion, bool guardarPos) {
    ft++;
    if (guardarPos)
        posTerm.push_back(posicion);
}


  //////////////////////////////////////////////////////
 //     InformacionPregunta                          //
//////////////////////////////////////////////////////

InformacionPregunta::InformacionPregunta()
    : numTotalPal(0), numTotalPalSinParada(0), numTotalPalDiferentes(0) {}

InformacionPregunta::InformacionPregunta(const InformacionPregunta& o)
    : numTotalPal(o.numTotalPal),
      numTotalPalSinParada(o.numTotalPalSinParada),
      numTotalPalDiferentes(o.numTotalPalDiferentes) {}

InformacionPregunta::~InformacionPregunta() { Vaciar(); }

InformacionPregunta& InformacionPregunta::operator=(const InformacionPregunta& o) {
    if (this != &o) {
        numTotalPal           = o.numTotalPal;
        numTotalPalSinParada  = o.numTotalPalSinParada;
        numTotalPalDiferentes = o.numTotalPalDiferentes;
    }
    return *this;
}

ostream& operator<<(ostream& s, const InformacionPregunta& p) {
    s << "numTotalPal: "           << p.numTotalPal
      << "\tnumTotalPalSinParada: "<< p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: "<< p.numTotalPalDiferentes;
    return s;
}

void InformacionPregunta::Inicializar(int nPal, int nPalSinP, int nPalDif) {
    numTotalPal           = nPal;
    numTotalPalSinParada  = nPalSinP;
    numTotalPalDiferentes = nPalDif;
}

void InformacionPregunta::Vaciar() {
    numTotalPal = numTotalPalSinParada = numTotalPalDiferentes = 0;
}

bool InformacionPregunta::EstaVacia() const {
    return numTotalPal == 0;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: CONSTRUCTORES / DESTRUCTOR    //
//////////////////////////////////////////////////////

/**
 * @brief Constructor principal.
 *
 * Carga las palabras de parada desde fichStopWords (aplicandoles las
 * mismas transformaciones que se aplicaran a los documentos).
 * Los indices quedan vacios.
 */
IndexadorHash::IndexadorHash(const string& fichStopWords,
                             const string& delimitadores,
                             const bool&   detectComp,
                             const bool&   minuscSinAcentos,
                             const string& dirIndice,
                             const int&    tStemmer,
                             const bool&   almPosTerm)
    : tok(delimitadores, detectComp, minuscSinAcentos),
      ficheroStopWords(fichStopWords),
      directorioIndice(dirIndice),
      tipoStemmer(tStemmer),
      almacenarPosTerm(almPosTerm),
      pregunta("")
{
    CargarStopWords(fichStopWords);
}

/**
 * @brief Constructor que recupera una indexacion desde disco.
 */
IndexadorHash::IndexadorHash(const string& directorioIndexacion)
    : tipoStemmer(0), almacenarPosTerm(false), pregunta("")
{
    if (!RecuperarIndexacion(directorioIndexacion)) {
        cerr << "ERROR: No se pudo recuperar la indexacion desde: "
             << directorioIndexacion << endl;
    }
}

/**
 * @brief Constructor copia.
 */
IndexadorHash::IndexadorHash(const IndexadorHash& o)
    : indice(o.indice),
      indiceDocs(o.indiceDocs),
      informacionColeccionDocs(o.informacionColeccionDocs),
      pregunta(o.pregunta),
      indicePregunta(o.indicePregunta),
      infPregunta(o.infPregunta),
      stopWords(o.stopWords),
      ficheroStopWords(o.ficheroStopWords),
      tok(o.tok),
      directorioIndice(o.directorioIndice),
      tipoStemmer(o.tipoStemmer),
      almacenarPosTerm(o.almacenarPosTerm)
{}

/**
 * @brief Destructor.
 */
IndexadorHash::~IndexadorHash() {
    VaciarIndiceDocs();
    VaciarIndicePreg();
}

/**
 * @brief Operador de asignacion.
 */
IndexadorHash& IndexadorHash::operator=(const IndexadorHash& o) {
    if (this != &o) {
        indice                   = o.indice;
        indiceDocs               = o.indiceDocs;
        informacionColeccionDocs = o.informacionColeccionDocs;
        pregunta                 = o.pregunta;
        indicePregunta           = o.indicePregunta;
        infPregunta              = o.infPregunta;
        stopWords                = o.stopWords;
        ficheroStopWords         = o.ficheroStopWords;
        tok                      = o.tok;
        directorioIndice         = o.directorioIndice;
        tipoStemmer              = o.tipoStemmer;
        almacenarPosTerm         = o.almacenarPosTerm;
    }
    return *this;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: METODOS AUXILIARES PRIVADOS   //
//////////////////////////////////////////////////////

/**
 * @brief Carga las palabras de parada desde 'fichero'.
 *
 * Aplica las mismas transformaciones que se aplican a los tokens de los
 * documentos (minuscSinAcentos via tokenizador + stemmer), de modo que la
 * comparacion posterior sea coherente.
 */
void IndexadorHash::CargarStopWords(const string& fichero) {
    stopWords.clear();
    ifstream f(fichero.c_str());
    if (!f) {
        cerr << "AVISO: No se pudo abrir el fichero de stop-words: "
             << fichero << endl;
        return;
    }
    string linea;
    while (getline(f, linea)) {
        if (linea.empty()) continue;
        // Tokenizamos la linea por si hay espacios/delimitadores extra
        list<string> tokens;
        tok.Tokenizar(linea, tokens);
        for (const string& t : tokens) {
            string sw = TransformarTerm(t);
            if (!sw.empty())
                stopWords.insert(sw);
        }
    }
}

/**
 * @brief Devuelve el siguiente idDoc disponible (max actual + 1).
 *
 * Si no hay documentos indexados devuelve 1.
 */
int IndexadorHash::SiguienteIdDoc() const {
    int maxId = 0;
    for (auto& par : indiceDocs)
        if (par.second.ObtenerIdDoc() > maxId)
            maxId = par.second.ObtenerIdDoc();
    return maxId + 1;
}

/**
 * @brief Indexa un solo documento.
 *
 * @param nomDoc   Ruta al fichero.
 * @param idDoc    Identificador asignado (puede ser nuevo o el de reindexacion).
 * @param tamBytes Tamano del fichero en bytes.
 * @param fecha    Fecha de modificacion.
 * @return false si se produce un error de E/S o de memoria.
 */
bool IndexadorHash::IndexarDocumento(const string& nomDoc,
                                     int idDoc,
                                     long tamBytes,
                                     const Fecha& fecha) {
    ifstream f(nomDoc.c_str());
    if (!f) {
        cerr << "ERROR: No se puede abrir el documento: " << nomDoc << endl;
        return false;
    }

    int numPal          = 0;
    int numPalSinParada = 0;
    int posicion        = 0; // 0-based; se numeran tambien las stop-words

    unordered_set<string> diferentesDoc; // para contar numPalDiferentes del doc

    string linea;
    try {
        while (getline(f, linea)) {
            list<string> tokens;
            tok.Tokenizar(linea, tokens);

            for (const string& raw : tokens) {
                numPal++;

                // Transformacion: el tokenizador ya aplico minuscSinAcentos;
                // ahora aplicamos el stemmer.
                string term = TransformarTerm(raw);

                if (stopWords.count(term) > 0) {
                    // Palabra de parada: cuenta posicion pero no se indexa
                    posicion++;
                    continue;
                }

                numPalSinParada++;

                // Termino nuevo en la coleccion?
                bool nuevoEnColeccion = (indice.count(term) == 0);

                // Termino nuevo en este documento?
                bool nuevoEnDoc = (diferentesDoc.count(term) == 0);
                if (nuevoEnDoc)
                    diferentesDoc.insert(term);

                // Registrar en el indice invertido
                indice[term].AnadirOcurrenciaDoc(idDoc, posicion, almacenarPosTerm);

                // Actualizar contador de terminos distintos en la coleccion
                if (nuevoEnColeccion)
                    informacionColeccionDocs.AjustarPalDiferentes(+1);

                posicion++;
            }
        }
    } catch (const bad_alloc&) {
        cerr << "ERROR: Falta de memoria al indexar: " << nomDoc << endl;
        f.close();
        return false;
    }

    int numPalDiferentes = (int)diferentesDoc.size();

    InfDoc infDoc;
    infDoc.Inicializar(idDoc, numPal, numPalSinParada,
                       numPalDiferentes, (int)tamBytes, fecha);
    indiceDocs[nomDoc] = infDoc;

    informacionColeccionDocs.AnadirDoc(infDoc);

    f.close();
    return true;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: INDEXACION DE DOCUMENTOS      //
//////////////////////////////////////////////////////

/**
 * @brief Indexa todos los documentos listados en 'ficheroDocumentos'.
 *
 * Cada linea del fichero debe contener la ruta a un documento.
 * Si un documento ya esta indexado y su fecha de modificacion es mas
 * reciente, se reindexara manteniendo el mismo idDoc.
 * Si la fecha es igual o anterior, se muestra un aviso y se omite.
 */
bool IndexadorHash::Indexar(const string& ficheroDocumentos) {
    ifstream f(ficheroDocumentos.c_str());
    if (!f) {
        cerr << "ERROR: No existe el fichero de documentos: "
             << ficheroDocumentos << endl;
        return false;
    }

    string nomDoc;
    while (getline(f, nomDoc)) {
        if (nomDoc.empty()) continue;

        // --- Obtener informacion del fichero en disco ---
        struct stat st;
        if (stat(nomDoc.c_str(), &st) != 0) {
            cerr << "EXCEPCION: No existe el documento: " << nomDoc << endl;
            continue; // error no fatal: seguimos con el resto
        }
        if (S_ISDIR(st.st_mode)) {
            cerr << "EXCEPCION: Es un directorio, no un fichero: "
                 << nomDoc << endl;
            continue;
        }

        Fecha fechaDoc(st.st_mtime);
        long  tamDoc  = st.st_size;

        // --- Comprobar si ya esta indexado ---
        int  idDoc;
        auto it = indiceDocs.find(nomDoc);
        if (it != indiceDocs.end()) {
            Fecha fechaAnterior = it->second.ObtenerFecha();
            if (fechaDoc.esMasReciente(fechaAnterior)) {
                // Reindexacion: borramos y volvemos a indexar con el mismo id
                cerr << "EXCEPCION: Reindexando documento modificado: "
                     << nomDoc << endl;
                idDoc = it->second.ObtenerIdDoc();
                BorraDoc(nomDoc);
            } else {
                cerr << "EXCEPCION: Documento ya indexado (sin cambios): "
                     << nomDoc << endl;
                continue;
            }
        } else {
            idDoc = SiguienteIdDoc();
        }

        // --- Indexar ---
        if (!IndexarDocumento(nomDoc, idDoc, tamDoc, fechaDoc)) {
            f.close();
            return false; // error de memoria: paramos
        }
    }

    f.close();
    return true;
}

/**
 * @brief Indexa recursivamente todos los ficheros del directorio 'dirAIndexar'.
 *
 * Usa 'find' para obtener el listado, igual que hace el tokenizador.
 */
bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) {
    struct stat st;
    if (stat(dirAIndexar.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        cerr << "ERROR: No existe el directorio: " << dirAIndexar << endl;
        return false;
    }

    // Generamos lista de ficheros en un temporal
    string tmp = "/tmp/.lista_docs_idx";
    string cmd = "find " + dirAIndexar +
                 " -follow -not -type d | sort > " + tmp;
    system(cmd.c_str());

    return Indexar(tmp);
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: PERSISTENCIA                  //
//////////////////////////////////////////////////////

/**
 * @brief Guarda la indexacion en disco.
 *
 * Formato del fichero "indice.idx":
 *   Seccion PARAMS:  parametros de configuracion
 *   Seccion STOPS:   stop-words (una por linea)
 *   Seccion COLL:    estadisticas de la coleccion
 *   Seccion DOCS:    informacion de cada documento
 *   Seccion INDEX:   terminos con su informacion completa
 *   Seccion QUERY:   pregunta indexada (si la hay)
 */
bool IndexadorHash::GuardarIndexacion() const {
    // Crear directorio si no existe
    string dir = directorioIndice.empty() ? "." : directorioIndice;
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
        string cmd = "mkdir -p " + dir;
        if (system(cmd.c_str()) != 0) {
            cerr << "ERROR: No se pudo crear el directorio: " << dir << endl;
            return false;
        }
    }

    string ruta = dir + "/" + FICHERO_INDICE;
    ofstream f(ruta.c_str());
    if (!f) {
        cerr << "ERROR: No se pudo abrir para escritura: " << ruta << endl;
        return false;
    }

    try {
        // ── PARAMS ──────────────────────────────────────────────────────────
        f << "PARAMS\n";
        f << ficheroStopWords          << "\n";
        f << tok.DelimitadoresPalabra() << "\n";
        f << (const_cast<Tokenizador&>(tok).CasosEspeciales()    ? 1 : 0) << "\n";
        f << (const_cast<Tokenizador&>(tok).PasarAminuscSinAcentos() ? 1 : 0) << "\n";
        f << directorioIndice          << "\n";
        f << tipoStemmer               << "\n";
        f << (almacenarPosTerm         ? 1 : 0) << "\n";

        // ── STOPS ───────────────────────────────────────────────────────────
        f << "STOPS\n";
        f << stopWords.size() << "\n";
        for (const string& sw : stopWords)
            f << sw << "\n";

        // ── COLL ────────────────────────────────────────────────────────────
        f << "COLL\n";
        f << informacionColeccionDocs.ObtenerNumDocs()               << "\n";
        f << informacionColeccionDocs.ObtenerNumTotalPal()           << "\n";
        f << informacionColeccionDocs.ObtenerNumTotalPalSinParada()  << "\n";
        f << informacionColeccionDocs.ObtenerNumTotalPalDiferentes() << "\n";
        f << informacionColeccionDocs.ObtenerTamBytes()              << "\n";

        // ── DOCS ────────────────────────────────────────────────────────────
        f << "DOCS\n";
        f << indiceDocs.size() << "\n";
        for (auto& par : indiceDocs) {
            const InfDoc& d = par.second;
            f << par.first                   << "|"   // nombre fichero
              << d.ObtenerIdDoc()            << "|"
              << d.ObtenerNumPal()           << "|"
              << d.ObtenerNumPalSinParada()  << "|"
              << d.ObtenerNumPalDif()        << "|"
              << d.ObtenerTamBytes()         << "|"
              << (long)d.ObtenerFecha().tiempo << "\n";
        }

        // ── INDEX ────────────────────────────────────────────────────────────
        f << "INDEX\n";
        f << indice.size() << "\n";
        for (auto& parT : indice) {
            const InformacionTermino& it = parT.second;
            f << parT.first << "|" << it.ObtenerFtc()
              << "|" << it.ObtenerFd() << "\n";
            for (auto& parD : it.ObtenerDocs()) {
                const InfTermDoc& itd = parD.second;
                f << parD.first << "|" << itd.ObtenerFt();
                for (int p : itd.ObtenerPosTerm())
                    f << "|" << p;
                f << "\n";
            }
        }

        // ── QUERY ────────────────────────────────────────────────────────────
        f << "QUERY\n";
        f << pregunta << "\n";
        f << infPregunta.ObtenerNumTotalPal()           << "|"
          << infPregunta.ObtenerNumTotalPalSinParada()  << "|"
          << infPregunta.ObtenerNumTotalPalDiferentes() << "\n";
        f << indicePregunta.size() << "\n";
        for (auto& parQ : indicePregunta) {
            const InformacionTerminoPregunta& itp = parQ.second;
            f << parQ.first << "|" << itp.ObtenerFt();
            for (int p : itp.ObtenerPosTerm())
                f << "|" << p;
            f << "\n";
        }

    } catch (...) {
        cerr << "ERROR: Fallo al escribir la indexacion en: " << ruta << endl;
        f.close();
        // Vaciamos el fichero corrupto
        ofstream fv(ruta.c_str()); 
        return false;
    }

    f.close();
    return true;
}

/**
 * @brief Recupera la indexacion desde disco.
 *
 * Vacia el estado actual antes de cargar.
 * Devuelve false si el directorio o el fichero no existen o estan corruptos.
 */
bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion) {
    string dir  = directorioIndexacion.empty() ? "." : directorioIndexacion;
    string ruta = dir + "/" + FICHERO_INDICE;

    ifstream f(ruta.c_str());
    if (!f) {
        cerr << "ERROR: No existe la indexacion en: " << ruta << endl;
        VaciarIndiceDocs();
        VaciarIndicePreg();
        return false;
    }

    VaciarIndiceDocs();
    VaciarIndicePreg();

    try {
        string sec;

        // ── PARAMS ───────────────────────────────────────────────────────────
        getline(f, sec); // "PARAMS"
        string delim;
        int ce, ma;
        getline(f, ficheroStopWords);
        getline(f, delim);
        f >> ce >> ma; f.ignore();
        getline(f, directorioIndice);
        f >> tipoStemmer >> almacenarPosTerm; f.ignore();

        tok = Tokenizador(delim, (bool)ce, (bool)ma);

        // ── STOPS ─────────────────────────────────────────────────────────────
        getline(f, sec); // "STOPS"
        int nsw; f >> nsw; f.ignore();
        stopWords.clear();
        for (int i = 0; i < nsw; i++) {
            string sw; getline(f, sw);
            stopWords.insert(sw);
        }

        // ── COLL ──────────────────────────────────────────────────────────────
        getline(f, sec); // "COLL"
        int nd, ntp, ntps, ntpd, tb;
        f >> nd >> ntp >> ntps >> ntpd >> tb; f.ignore();
        // Reconstruimos informacionColeccionDocs manualmente
        // (usamos AnadirDoc no es factible sin docs completos; hacemos bypass)
        InfColeccionDocs tmp;
        // Usamos un InfDoc auxiliar para volcar los totales
        // No hay metodo directo: ajustamos campo a campo mediante AjustarPalDiferentes
        // y un doc ficticio
        InfDoc docFicticio;
        docFicticio.Inicializar(0, ntp, ntps, 0, tb, Fecha());
        tmp.AnadirDoc(docFicticio);
        tmp.AjustarPalDiferentes(ntpd);
        // Quitamos el doc ficticio del contador de docs (AnadirDoc incrementa numDocs)
        // Truco: creamos la coleccion con los valores directos
        informacionColeccionDocs = InfColeccionDocs();
        // Vamos a usar un metodo de carga directa:
        // Como InfColeccionDocs no expone setters individuales, lo reconstruimos
        // via AnadirDoc con valores desglosados por documento en la seccion DOCS.
        // Guardamos nd, ntpd para ajustar despues de cargar DOCS.
        int totalPalDif = ntpd;

        // ── DOCS ──────────────────────────────────────────────────────────────
        getline(f, sec); // "DOCS"
        int ndocs; f >> ndocs; f.ignore();
        for (int i = 0; i < ndocs; i++) {
            string linea; getline(f, linea);
            istringstream ss(linea);
            string nom, sid, snp, snps, snpd, stb, sti;
            getline(ss, nom,  '|');
            getline(ss, sid,  '|');
            getline(ss, snp,  '|');
            getline(ss, snps, '|');
            getline(ss, snpd, '|');
            getline(ss, stb,  '|');
            getline(ss, sti,  '|');
            InfDoc d;
            d.Inicializar(stoi(sid), stoi(snp), stoi(snps),
                          stoi(snpd), stoi(stb), Fecha((time_t)stol(sti)));
            indiceDocs[nom] = d;
            informacionColeccionDocs.AnadirDoc(d);
        }
        // Ajustamos numTotalPalDiferentes (AnadirDoc no lo actualiza)
        informacionColeccionDocs.AjustarPalDiferentes(totalPalDif);

        // ── INDEX ─────────────────────────────────────────────────────────────
        getline(f, sec); // "INDEX"
        int nterms; f >> nterms; f.ignore();
        for (int i = 0; i < nterms; i++) {
            // termino|ftc|fd
            string lhead; getline(f, lhead);
            istringstream ss(lhead);
            string term, sftc, sfd;
            getline(ss, term, '|');
            getline(ss, sftc, '|');
            getline(ss, sfd,  '|');
            int fd = stoi(sfd);

            InformacionTermino infT;
            for (int j = 0; j < fd; j++) {
                string ldoc; getline(f, ldoc);
                istringstream sd(ldoc);
                string sId, sFt, sPos;
                getline(sd, sId, '|');
                getline(sd, sFt, '|');
                int idD = stoi(sId);
                int ft  = stoi(sFt);
                list<int> positions;
                while (getline(sd, sPos, '|'))
                    if (!sPos.empty()) positions.push_back(stoi(sPos));

                // Reconstruimos InfTermDoc directamente
                for (int k = 0; k < ft; k++) {
                    int pos = positions.empty() ? 0 : positions.front();
                    if (!positions.empty()) positions.pop_front();
                    infT.AnadirOcurrenciaDoc(idD, pos, almacenarPosTerm);
                }
            }
            indice[term] = infT;
        }

        // ── QUERY ─────────────────────────────────────────────────────────────
        getline(f, sec); // "QUERY"
        getline(f, pregunta);
        {
            string lq; getline(f, lq);
            istringstream sq(lq);
            string sp, sps, spd;
            getline(sq, sp,  '|');
            getline(sq, sps, '|');
            getline(sq, spd, '|');
            infPregunta.Inicializar(stoi(sp), stoi(sps), stoi(spd));
        }
        int nq; f >> nq; f.ignore();
        for (int i = 0; i < nq; i++) {
            string lq; getline(f, lq);
            istringstream sq(lq);
            string term, sFt, sPos;
            getline(sq, term, '|');
            getline(sq, sFt,  '|');
            int ft = stoi(sFt);
            list<int> positions;
            while (getline(sq, sPos, '|'))
                if (!sPos.empty()) positions.push_back(stoi(sPos));
            InformacionTerminoPregunta itp;
            for (int k = 0; k < ft; k++) {
                int pos = positions.empty() ? 0 : positions.front();
                if (!positions.empty()) positions.pop_front();
                itp.AnadirOcurrencia(pos, almacenarPosTerm);
            }
            indicePregunta[term] = itp;
        }

    } catch (...) {
        cerr << "ERROR: Datos corruptos en la indexacion: " << ruta << endl;
        VaciarIndiceDocs();
        VaciarIndicePreg();
        f.close();
        return false;
    }

    f.close();
    return true;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: INDEXACION DE PREGUNTA        //
//////////////////////////////////////////////////////

/**
 * @brief Indexa la cadena 'preg' como pregunta.
 *
 * Vacia el indice de pregunta anterior, tokeniza y construye
 * indicePregunta e infPregunta. Devuelve false si la pregunta no
 * contiene ningun termino no stop-word.
 */
bool IndexadorHash::IndexarPregunta(const string& preg) {
    VaciarIndicePreg();
    pregunta = preg;

    list<string> tokens;
    tok.Tokenizar(preg, tokens);

    int numPal          = 0;
    int numPalSinParada = 0;
    int posicion        = 0;
    unordered_set<string> diferentesPreg;

    for (const string& raw : tokens) {
        numPal++;
        string term = TransformarTerm(raw);

        if (stopWords.count(term) > 0) {
            posicion++;
            continue;
        }

        numPalSinParada++;
        diferentesPreg.insert(term);
        indicePregunta[term].AnadirOcurrencia(posicion, almacenarPosTerm);
        posicion++;
    }

    if (indicePregunta.empty()) {
        cerr << "ERROR: La pregunta no contiene terminos validos: "
             << preg << endl;
        pregunta = "";
        return false;
    }

    infPregunta.Inicializar(numPal, numPalSinParada,
                            (int)diferentesPreg.size());
    return true;
}

bool IndexadorHash::DevuelvePregunta(string& preg) const {
    if (indicePregunta.empty()) return false;
    preg = pregunta;
    return true;
}

bool IndexadorHash::DevuelvePregunta(const string& word,
                                     InformacionTerminoPregunta& inf) const {
    string term = TransformarTerm(word);
    auto it = indicePregunta.find(term);
    if (it == indicePregunta.end()) {
        inf = InformacionTerminoPregunta();
        return false;
    }
    inf = it->second;
    return true;
}

bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const {
    if (indicePregunta.empty()) {
        inf = InformacionPregunta();
        return false;
    }
    inf = infPregunta;
    return true;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: CONSULTAS AL INDICE           //
//////////////////////////////////////////////////////

bool IndexadorHash::Devuelve(const string& word,
                             InformacionTermino& inf) const {
    string term = TransformarTerm(word);
    auto it = indice.find(term);
    if (it == indice.end()) {
        inf = InformacionTermino();
        return false;
    }
    inf = it->second;
    return true;
}

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc,
                             InfTermDoc& infDoc) const {
    string term = TransformarTerm(word);
    auto it = indice.find(term);
    if (it == indice.end()) { infDoc = InfTermDoc(); return false; }

    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) { infDoc = InfTermDoc(); return false; }

    int idDoc = itDoc->second.ObtenerIdDoc();
    if (!it->second.ExisteDoc(idDoc)) { infDoc = InfTermDoc(); return false; }

    infDoc = it->second.ObtenerDocs().at(idDoc);
    return true;
}

bool IndexadorHash::Existe(const string& word) const {
    return indice.count(TransformarTerm(word)) > 0;
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: BORRADO                       //
//////////////////////////////////////////////////////

/**
 * @brief Borra un documento del indice.
 *
 * Recorre todos los terminos del indice y elimina la entrada del documento.
 * Si un termino queda sin documentos lo elimina del indice y decrementa
 * el contador de palabras diferentes de la coleccion.
 */
bool IndexadorHash::BorraDoc(const string& nomDoc) {
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false;

    int idDoc = itDoc->second.ObtenerIdDoc();

    // Eliminar el documento de cada termino del indice
    vector<string> terminosAEliminar;
    for (auto& par : indice) {
        if (par.second.ExisteDoc(idDoc)) {
            par.second.EliminarDoc(idDoc);
            if (par.second.ObtenerFd() == 0)
                terminosAEliminar.push_back(par.first);
        }
    }
    // Eliminar terminos que ya no aparecen en ningun documento
    for (const string& t : terminosAEliminar) {
        indice.erase(t);
        informacionColeccionDocs.AjustarPalDiferentes(-1);
    }

    // Actualizar estadisticas de la coleccion
    informacionColeccionDocs.EliminarDoc(itDoc->second);

    // Eliminar de indiceDocs
    indiceDocs.erase(itDoc);

    return true;
}

void IndexadorHash::VaciarIndiceDocs() {
    indice.clear();
    indiceDocs.clear();
    informacionColeccionDocs.Vaciar();
}

void IndexadorHash::VaciarIndicePreg() {
    indicePregunta.clear();
    infPregunta.Vaciar();
    pregunta = "";
}


  //////////////////////////////////////////////////////
 //     IndexadorHash: GETTERS Y LISTADOS            //
//////////////////////////////////////////////////////

int    IndexadorHash::NumPalIndexadas()           const { return (int)indice.size(); }
string IndexadorHash::DevolverFichPalParada()     const { return ficheroStopWords;   }
int    IndexadorHash::NumPalParada()              const { return (int)stopWords.size(); }
string IndexadorHash::DevolverDelimitadores()     const { return tok.DelimitadoresPalabra(); }
bool   IndexadorHash::DevolverCasosEspeciales()   const { return const_cast<Tokenizador&>(tok).CasosEspeciales(); }
bool   IndexadorHash::DevolverPasarAminuscSinAcentos() const { return const_cast<Tokenizador&>(tok).PasarAminuscSinAcentos(); }
bool   IndexadorHash::DevolverAlmacenarPosTerm()  const { return almacenarPosTerm;   }
string IndexadorHash::DevolverDirIndice()         const { return directorioIndice;   }
int    IndexadorHash::DevolverTipoStemming()      const { return tipoStemmer;        }

void IndexadorHash::ListarPalParada() const {
    for (const string& sw : stopWords)
        cout << sw << "\n";
}

void IndexadorHash::ListarInfColeccDocs() const {
    cout << informacionColeccionDocs << endl;
}

void IndexadorHash::ListarTerminos() const {
    for (auto& par : indice)
        cout << par.first << '\t' << par.second << endl;
}

bool IndexadorHash::ListarTerminos(const string& nomDoc) const {
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false;

    int idDoc = itDoc->second.ObtenerIdDoc();
    for (auto& par : indice) {
        if (par.second.ExisteDoc(idDoc))
            cout << par.first << '\t' << par.second << endl;
    }
    return true;
}

void IndexadorHash::ListarDocs() const {
    for (auto& par : indiceDocs)
        cout << par.first << '\t' << par.second << endl;
}

bool IndexadorHash::ListarDocs(const string& nomDoc) const {
    auto it = indiceDocs.find(nomDoc);
    if (it == indiceDocs.end()) return false;
    cout << it->first << '\t' << it->second << endl;
    return true;
}