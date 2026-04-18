#include "indexadorHash.h"
#include "stemmer.h"
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <algorithm>
#include <vector>

using namespace std;

  //////////////////////////////////////////////////////
 //     CONSTRUCTORES, DESTRUCTORES Y OPERADORES     //
//////////////////////////////////////////////////////

IndexadorHash::IndexadorHash(const string& fichStopWords, const string& delimitadores,
                             const bool& detectComp, const bool& minuscSinAcentos, 
                             const string& dirIndice, const int& tStemmer, const bool& almPosTerm)
    : tok(delimitadores, detectComp, minuscSinAcentos), 
      ficheroStopWords(fichStopWords), 
      directorioIndice(dirIndice), 
      tipoStemmer(tStemmer), 
      almacenarPosTerm(almPosTerm) {
    
    pregunta = "";
    CargarStopWords(fichStopWords);
}

IndexadorHash::IndexadorHash(const string& directorioIndexacion) {
    pregunta = "";
    if (!RecuperarIndexacion(directorioIndexacion)) {
        cerr << "ERROR: No se pudo recuperar la indexacion del directorio: " << directorioIndexacion << endl;
    }
}

IndexadorHash::IndexadorHash(const IndexadorHash& o) {
    *this = o;
}

IndexadorHash::~IndexadorHash() {
    VaciarIndiceDocs();
    VaciarIndicePreg();
}

IndexadorHash& IndexadorHash::operator=(const IndexadorHash& o) {
    if (this != &o) {
        indice = o.indice;
        indiceDocs = o.indiceDocs;
        informacionColeccionDocs = o.informacionColeccionDocs;
        pregunta = o.pregunta;
        indicePregunta = o.indicePregunta;
        infPregunta = o.infPregunta;
        stopWords = o.stopWords;
        ficheroStopWords = o.ficheroStopWords;
        tok = o.tok;
        directorioIndice = o.directorioIndice;
        tipoStemmer = o.tipoStemmer;
        almacenarPosTerm = o.almacenarPosTerm;
    }
    return *this;
}

  //////////////////////////////////////////////////////
 //          METODOS PRIVADOS AUXILIARES             //
//////////////////////////////////////////////////////

string IndexadorHash::AplicarStemmer(const string& palabra) const {
    if (tipoStemmer == 1 || tipoStemmer == 2) {
        return stemmer(palabra, tipoStemmer);
    }
    return palabra;
}

string IndexadorHash::TransformarTerm(const string& t) const {
    return AplicarStemmer(t);
}

void IndexadorHash::CargarStopWords(const string& fichero) {
    stopWords.clear();
    ifstream f(fichero.c_str());
    if (!f) {
        cerr << "ERROR: No se puede abrir el fichero de palabras de parada: " << fichero << endl;
        return;
    }

    string linea;
    while (getline(f, linea)) {
        if (linea.empty()) continue;
        list<string> tokens;
        tok.Tokenizar(linea, tokens);
        for (const string& t : tokens) {
            string sw = TransformarTerm(t);
            if (!sw.empty()) stopWords.insert(sw);
        }
    }
    f.close();
}

int IndexadorHash::SiguienteIdDoc() const {
    int maxId = 0;
    for (auto it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
        if (it->second.ObtenerIdDoc() > maxId) {
            maxId = it->second.ObtenerIdDoc();
        }
    }
    return maxId + 1; // El primer idDoc será 1 según el enunciado
}

bool IndexadorHash::IndexarDocumento(const string& nomDoc, int idDoc, long tamBytes, const Fecha& fecha) {
    ifstream f(nomDoc.c_str());
    if (!f) {
        cerr << "ERROR: No se puede abrir el documento a indexar: " << nomDoc << endl;
        return false;
    }

    int numPal = 0;
    int numPalSinParada = 0;
    int posicion = 0; // Posición 0-based, cuenta stopwords

    unordered_set<string> diferentesDoc; 
    string linea;

    try {
        while (getline(f, linea)) {
            list<string> tokens;
            tok.Tokenizar(linea, tokens);

            for (list<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                numPal++;
                string term = TransformarTerm(*it);

                if (stopWords.count(term) > 0) {
                    posicion++;
                    continue;
                }

                numPalSinParada++;
                bool nuevoEnColeccion = (indice.count(term) == 0);
                bool nuevoEnDoc = (diferentesDoc.count(term) == 0);
                
                if (nuevoEnDoc) diferentesDoc.insert(term);

                indice[term].AnadirOcurrenciaDoc(idDoc, posicion, almacenarPosTerm);

                if (nuevoEnColeccion) {
                    informacionColeccionDocs.AjustarPalDiferentes(1);
                }
                posicion++;
            }
        }
    } catch (const bad_alloc&) {
        cerr << "ERROR: Falta de memoria al indexar el documento: " << nomDoc << endl;
        f.close();
        return false;
    }

    InfDoc infDoc;
    infDoc.Inicializar(idDoc, numPal, numPalSinParada, (int)diferentesDoc.size(), (int)tamBytes, fecha);
    indiceDocs[nomDoc] = infDoc;
    informacionColeccionDocs.AnadirDoc(infDoc);

    f.close();
    return true;
}

  //////////////////////////////////////////////////////
 //          MÉTODOS DE INDEXACIÓN (DOCUMENTOS)      //
//////////////////////////////////////////////////////

bool IndexadorHash::Indexar(const string& ficheroDocumentos) {
    ifstream f(ficheroDocumentos.c_str());
    if (!f) {
        cerr << "ERROR: No existe el fichero de documentos: " << ficheroDocumentos << endl;
        return false;
    }

    string nomDoc;
    while (getline(f, nomDoc)) {
        if (nomDoc.empty()) continue;

        struct stat st;
        if (stat(nomDoc.c_str(), &st) != 0 || S_ISDIR(st.st_mode)) {
            cerr << "ERROR: No existe o es un directorio el documento: " << nomDoc << endl;
            continue;
        }

        Fecha fechaDoc(st.st_mtime);
        long tamDoc = st.st_size;
        int idDocAsignado;

        auto it = indiceDocs.find(nomDoc);
        if (it != indiceDocs.end()) {
            if (fechaDoc.esMasReciente(it->second.ObtenerFecha())) {
                idDocAsignado = it->second.ObtenerIdDoc();
                BorraDoc(nomDoc);
            } else {
                continue; // Ya está indexado y no es más reciente
            }
        } else {
            idDocAsignado = SiguienteIdDoc();
        }

        if (!IndexarDocumento(nomDoc, idDocAsignado, tamDoc, fechaDoc)) {
            f.close();
            return false;
        }
    }

    f.close();
    return true;
}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar) {
    struct stat st;
    if (stat(dirAIndexar.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        cerr << "ERROR: No existe el directorio a indexar: " << dirAIndexar << endl;
        return false;
    }

    string tmpLista = ".lista_docs_idx_tmp";
    string cmd = "find " + dirAIndexar + " -type f > " + tmpLista;
    if (system(cmd.c_str()) != 0) {
        cerr << "ERROR: Fallo al ejecutar find sobre el directorio." << endl;
        return false;
    }

    bool resultado = Indexar(tmpLista);
    remove(tmpLista.c_str());
    return resultado;
}

  //////////////////////////////////////////////////////
 //          MÉTODOS DE INDEXACIÓN (PREGUNTA)        //
//////////////////////////////////////////////////////

bool IndexadorHash::IndexarPregunta(const string& preg) {
    VaciarIndicePreg();
    pregunta = preg;

    list<string> tokens;
    tok.Tokenizar(preg, tokens);

    int numPal = 0;
    int numPalSinParada = 0;
    int posicion = 0;
    unordered_set<string> diferentesPreg;

    for (list<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
        numPal++;
        string term = TransformarTerm(*it);

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
        cerr << "ERROR: La pregunta no contiene términos válidos (no stop-words)." << endl;
        pregunta = "";
        return false;
    }

    infPregunta.Inicializar(numPal, numPalSinParada, (int)diferentesPreg.size());
    return true;
}

bool IndexadorHash::DevuelvePregunta(string& preg) const {
    if (indicePregunta.empty()) return false;
    preg = pregunta;
    return true;
}

bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const {
    string term = TransformarTerm(word);
    auto it = indicePregunta.find(term);
    if (it != indicePregunta.end()) {
        inf = it->second;
        return true;
    }
    inf = InformacionTerminoPregunta();
    return false;
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
 //          MÉTODOS DE CONSULTA Y BORRADO           //
//////////////////////////////////////////////////////

bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const {
    string term = TransformarTerm(word);
    auto it = indice.find(term);
    if (it != indice.end()) {
        inf = it->second;
        return true;
    }
    inf = InformacionTermino();
    return false;
}

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& infDoc) const {
    string term = TransformarTerm(word);
    auto itTerm = indice.find(term);
    if (itTerm == indice.end()) {
        infDoc = InfTermDoc();
        return false;
    }

    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) {
        infDoc = InfTermDoc();
        return false;
    }

    int idDoc = itDoc->second.ObtenerIdDoc();
    if (!itTerm->second.ExisteDoc(idDoc)) {
        infDoc = InfTermDoc();
        return false;
    }

    infDoc = itTerm->second.ObtenerDocs().at(idDoc);
    return true;
}

bool IndexadorHash::Existe(const string& word) const {
    return indice.count(TransformarTerm(word)) > 0;
}

bool IndexadorHash::BorraDoc(const string& nomDoc) {
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false;

    int idDoc = itDoc->second.ObtenerIdDoc();
    vector<string> terminosAEliminar;

    for (auto it = indice.begin(); it != indice.end(); ++it) {
        if (it->second.ExisteDoc(idDoc)) {
            it->second.EliminarDoc(idDoc);
            if (it->second.ObtenerFd() == 0) {
                terminosAEliminar.push_back(it->first);
            }
        }
    }

    for (size_t i = 0; i < terminosAEliminar.size(); ++i) {
        indice.erase(terminosAEliminar[i]);
        informacionColeccionDocs.AjustarPalDiferentes(-1);
    }

    informacionColeccionDocs.EliminarDoc(itDoc->second);
    indiceDocs.erase(itDoc);

    return true;
}

void IndexadorHash::VaciarIndiceDocs() {
    indice.clear();
    indiceDocs.clear();
    informacionColeccionDocs.Vaciar();
}

void IndexadorHash::VaciarIndicePreg() {
    pregunta = "";
    indicePregunta.clear();
    infPregunta.Vaciar();
}

  //////////////////////////////////////////////////////
 //          PERSISTENCIA (GUARDAR Y RECUPERAR)      //
//////////////////////////////////////////////////////

bool IndexadorHash::GuardarIndexacion() const {
    string dir = directorioIndice.empty() ? "." : directorioIndice;
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
        string cmd = "mkdir -p " + dir;
        if (system(cmd.c_str()) != 0) {
            cerr << "ERROR: No se pudo crear el directorio: " << dir << endl;
            return false;
        }
    }

    string ruta = dir + "/indexacion.idx";
    ofstream f(ruta.c_str());
    if (!f) {
        cerr << "ERROR: No se pudo abrir para escritura: " << ruta << endl;
        return false;
    }

    try {
        // PARAMS
        f << ficheroStopWords << "\n";
        f << tok.DelimitadoresPalabra() << "\n";
        f << (tok.casosEspeciales() ? 1 : 0) << "\n";
        f << (tok.pasarAminuscSinAcentos() ? 1 : 0) << "\n";
        f << directorioIndice << "\n";
        f << tipoStemmer << "\n";
        f << (almacenarPosTerm ? 1 : 0) << "\n";

        // STOPWORDS
        f << stopWords.size() << "\n";
        for (auto sw = stopWords.begin(); sw != stopWords.end(); ++sw) f << *sw << "\n";

        // COLECION
        f << informacionColeccionDocs.ObtenerNumDocs() << " "
          << informacionColeccionDocs.ObtenerNumTotalPal() << " "
          << informacionColeccionDocs.ObtenerNumTotalPalSinParada() << " "
          << informacionColeccionDocs.ObtenerNumTotalPalDiferentes() << " "
          << informacionColeccionDocs.ObtenerTamBytes() << "\n";

        // DOCUMENTOS
        f << indiceDocs.size() << "\n";
        for (auto it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
            const InfDoc& d = it->second;
            f << it->first << "|" << d.ObtenerIdDoc() << "|" << d.ObtenerNumPal() << "|"
              << d.ObtenerNumPalSinParada() << "|" << d.ObtenerNumPalDif() << "|"
              << d.ObtenerTamBytes() << "|" << (long)d.ObtenerFecha().tiempo << "\n";
        }

        // INDICE TERMINOS
        f << indice.size() << "\n";
        for (auto it = indice.begin(); it != indice.end(); ++it) {
            const InformacionTermino& infT = it->second;
            f << it->first << "|" << infT.ObtenerFtc() << "|" << infT.ObtenerFd() << "\n";
            for (auto itDoc = infT.ObtenerDocs().begin(); itDoc != infT.ObtenerDocs().end(); ++itDoc) {
                f << itDoc->first << "|" << itDoc->second.ObtenerFt();
                for (int pos : itDoc->second.ObtenerPosTerm()) f << "|" << pos;
                f << "\n";
            }
        }

        // PREGUNTA
        f << pregunta << "\n";
        f << infPregunta.ObtenerNumTotalPal() << "|" 
          << infPregunta.ObtenerNumTotalPalSinParada() << "|" 
          << infPregunta.ObtenerNumTotalPalDiferentes() << "\n";
        f << indicePregunta.size() << "\n";
        for (auto it = indicePregunta.begin(); it != indicePregunta.end(); ++it) {
            f << it->first << "|" << it->second.ObtenerFt();
            for (int pos : it->second.ObtenerPosTerm()) f << "|" << pos;
            f << "\n";
        }
    } catch (...) {
        cerr << "ERROR: Fallo al escribir indexación." << endl;
        f.close();
        return false;
    }

    f.close();
    return true;
}

bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion) {
    string dir = directorioIndexacion.empty() ? "." : directorioIndexacion;
    string ruta = dir + "/indexacion.idx";
    ifstream f(ruta.c_str());

    if (!f) {
        cerr << "ERROR: No existe la indexacion en el directorio." << endl;
        return false;
    }

    VaciarIndiceDocs();
    VaciarIndicePreg();

    try {
        string delim;
        int ce, ma;
        getline(f, ficheroStopWords);
        getline(f, delim);
        f >> ce >> ma; f.ignore();
        getline(f, directorioIndice);
        f >> tipoStemmer >> almacenarPosTerm; f.ignore();

        tok = Tokenizador(delim, ce, ma);

        int nsw; f >> nsw; f.ignore();
        for (int i = 0; i < nsw; i++) {
            string sw; getline(f, sw);
            stopWords.insert(sw);
        }

        int nd, ntp, ntps, ntpd, tb;
        f >> nd >> ntp >> ntps >> ntpd >> tb; f.ignore();
        InfDoc ficticio; ficticio.Inicializar(0, ntp, ntps, 0, tb, Fecha());
        informacionColeccionDocs.AnadirDoc(ficticio);
        informacionColeccionDocs.AjustarPalDiferentes(ntpd);
        informacionColeccionDocs = InfColeccionDocs(); // Reset para AnadirDoc manual

        int totalPalDif = ntpd;
        int ndocs; f >> ndocs; f.ignore();
        for (int i = 0; i < ndocs; i++) {
            string linea; getline(f, linea);
            istringstream ss(linea);
            string nom, sid, snp, snps, snpd, stb, sti;
            getline(ss, nom, '|'); getline(ss, sid, '|'); getline(ss, snp, '|');
            getline(ss, snps, '|'); getline(ss, snpd, '|'); getline(ss, stb, '|'); getline(ss, sti, '|');
            
            InfDoc d;
            d.Inicializar(stoi(sid), stoi(snp), stoi(snps), stoi(snpd), stoi(stb), Fecha((time_t)stol(sti)));
            indiceDocs[nom] = d;
            informacionColeccionDocs.AnadirDoc(d);
        }
        informacionColeccionDocs.AjustarPalDiferentes(totalPalDif);

        int nterms; f >> nterms; f.ignore();
        for (int i = 0; i < nterms; i++) {
            string lhead; getline(f, lhead);
            istringstream ss(lhead);
            string term, sftc, sfd;
            getline(ss, term, '|'); getline(ss, sftc, '|'); getline(ss, sfd, '|');
            int fd = stoi(sfd);

            InformacionTermino infT;
            for (int j = 0; j < fd; j++) {
                string ldoc; getline(f, ldoc);
                istringstream sd(ldoc);
                string sId, sFt, sPos;
                getline(sd, sId, '|'); getline(sd, sFt, '|');
                int idD = stoi(sId); int ft = stoi(sFt);
                list<int> positions;
                while (getline(sd, sPos, '|')) if (!sPos.empty()) positions.push_back(stoi(sPos));

                for (int k = 0; k < ft; k++) {
                    int pos = positions.empty() ? 0 : positions.front();
                    if (!positions.empty()) positions.pop_front();
                    infT.AnadirOcurrenciaDoc(idD, pos, almacenarPosTerm);
                }
            }
            indice[term] = infT;
        }

        getline(f, pregunta);
        string lq; getline(f, lq);
        istringstream sq(lq);
        string sp, sps, spd;
        getline(sq, sp, '|'); getline(sq, sps, '|'); getline(sq, spd, '|');
        infPregunta.Inicializar(stoi(sp), stoi(sps), stoi(spd));

        int nq; f >> nq; f.ignore();
        for (int i = 0; i < nq; i++) {
            string lq2; getline(f, lq2);
            istringstream sq2(lq2);
            string term, sFt, sPos;
            getline(sq2, term, '|'); getline(sq2, sFt, '|');
            int ft = stoi(sFt);
            InformacionTerminoPregunta itp;
            while (getline(sq2, sPos, '|')) if (!sPos.empty()) itp.AnadirOcurrencia(stoi(sPos), almacenarPosTerm);
            indicePregunta[term] = itp;
        }
    } catch (...) {
        cerr << "ERROR: Datos corruptos." << endl;
        f.close();
        return false;
    }
    f.close();
    return true;
}

  //////////////////////////////////////////////////////
 //          GETTERS Y LISTADOS                      //
//////////////////////////////////////////////////////

int IndexadorHash::NumPalIndexadas() const { return (int)indice.size(); }
string IndexadorHash::DevolverFichPalParada() const { return ficheroStopWords; }
int IndexadorHash::NumPalParada() const { return (int)stopWords.size(); }
string IndexadorHash::DevolverDelimitadores() const { return tok.DelimitadoresPalabra(); }
bool IndexadorHash::DevolverCasosEspeciales() const { return const_cast<Tokenizador&>(tok).CasosEspeciales(); }
bool IndexadorHash::DevolverPasarAminuscSinAcentos() const { return const_cast<Tokenizador&>(tok).PasarAminuscSinAcentos(); }
bool IndexadorHash::DevolverAlmacenarPosTerm() const { return almacenarPosTerm; }
string IndexadorHash::DevolverDirIndice() const { return directorioIndice; }
int IndexadorHash::DevolverTipoStemming() const { return tipoStemmer; }

void IndexadorHash::ListarPalParada() const {
    for (auto it = stopWords.begin(); it != stopWords.end(); ++it) cout << *it << "\n";
}

void IndexadorHash::ListarInfColeccDocs() const { cout << informacionColeccionDocs << endl; }

void IndexadorHash::ListarTerminos() const {
    for (auto it = indice.begin(); it != indice.end(); ++it) cout << it->first << '\t' << it->second << endl;
}

bool IndexadorHash::ListarTerminos(const string& nomDoc) const {
    auto itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false;
    int idDoc = itDoc->second.ObtenerIdDoc();
    for (auto it = indice.begin(); it != indice.end(); ++it) {
        if (it->second.ExisteDoc(idDoc)) cout << it->first << '\t' << it->second << endl;
    }
    return true;
}

void IndexadorHash::ListarDocs() const {
    for (auto it = indiceDocs.begin(); it != indiceDocs.end(); ++it) cout << it->first << '\t' << it->second << endl;
}

bool IndexadorHash::ListarDocs(const string& nomDoc) const {
    auto it = indiceDocs.find(nomDoc);
    if (it == indiceDocs.end()) return false;
    cout << it->first << '\t' << it->second << endl;
    return true;
}