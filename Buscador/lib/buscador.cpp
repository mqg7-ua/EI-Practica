#include "../include/buscador.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <map>
#include <vector>

using namespace std;

  //////////////////////////////////////////////////////
 //   CONSTRUCTORES / DESTRUCTOR / ASIGNACION        //
//////////////////////////////////////////////////////

Buscador::Buscador() : IndexadorHash() {
    formSimilitud = 0;
    c  = 2.0;
    k1 = 1.2;
    b  = 0.75;
}

Buscador::Buscador(const string& directorioIndexacion, const int& f)
    : IndexadorHash(directorioIndexacion) {
    formSimilitud = f;
    c  = 2.0;
    k1 = 1.2;
    b  = 0.75;
}

Buscador::Buscador(const Buscador& o) : IndexadorHash(o) {
    docsOrdenados = o.docsOrdenados;
    formSimilitud = o.formSimilitud;
    c  = o.c;
    k1 = o.k1;
    b  = o.b;
}

Buscador::~Buscador() {
    while (!docsOrdenados.empty()) docsOrdenados.pop();
}

Buscador& Buscador::operator=(const Buscador& o) {
    if (this != &o) {
        IndexadorHash::operator=(o);
        docsOrdenados = o.docsOrdenados;
        formSimilitud = o.formSimilitud;
        c  = o.c;
        k1 = o.k1;
        b  = o.b;
    }
    return *this;
}

  //////////////////////////////////////////////////////
 //   METODO AUXILIAR:SCORING                        //
//////////////////////////////////////////////////////

bool Buscador::ScoreDocumentos(int numPregunta, int numDocumentos) {
    int N = informacionColeccionDocs.ObtenerNumDocs();
    if (N == 0) return true;

    double avgdl = (double)informacionColeccionDocs.ObtenerNumTotalPalSinParada() / N;
    if (avgdl < 1.0) avgdl = 1.0;

    // Mapa inverso idDoc -> longitud (numPalSinParada) para acceso O(1)
    unordered_map<int, double> docLen;
    docLen.reserve(indiceDocs.size());
    for (const auto& kv : indiceDocs) {
        double dl = kv.second.ObtenerNumPalSinParada();
        docLen[kv.second.ObtenerIdDoc()] = (dl > 0) ? dl : 1.0;
    }

    unordered_map<int, double> scores;
    string lastTerm;
    int    lastId = -1;

    try {
        for (const auto& qKv : indicePregunta) {
            lastTerm  = qKv.first;
            int qtf   = qKv.second.ObtenerFt();

            auto it = indice.find(lastTerm);
            if (it == indice.end()) continue;

            const InformacionTermino& tInfo = it->second;
            int df = tInfo.ObtenerFd();

            for (const auto& docKv : tInfo.ObtenerDocs()) {
                lastId   = docKv.first;
                int tf   = docKv.second.ObtenerFt();

                auto dlIt = docLen.find(lastId);
                double dl = (dlIt != docLen.end()) ? dlIt->second : avgdl;

                double contrib = 0.0;
                if (formSimilitud == 1) {
                    // BM25 (Okapi)
                    double idf    = log((N - df + 0.5) / (df + 0.5));
                    double tfNorm = tf * (k1 + 1.0) /
                                   (tf + k1 * (1.0 - b + b * dl / avgdl));
                    contrib = idf * tfNorm;
                } else {
                    // DFR In_expC2 con normalizacion 2
                    double idf = log((N + 1.0) / (df + 0.5)) / log(2.0);
                    double tfn = tf * log(1.0 + c * avgdl / dl) / log(2.0);
                    contrib    = (tfn > 0.0)
                                 ? qtf * (tfn / (tfn + 1.0)) * idf
                                 : 0.0;
                }
                scores[lastId] += contrib;
            }
        }
    } catch (const bad_alloc&) {
        cerr << "ERROR: Sin memoria. Documento: " << lastId
             << " Termino: " << lastTerm << endl;
        return false;
    }

    // Seleccionar top-numDocumentos por puntuacion descendente
    vector<pair<int, double>> ranking(scores.begin(), scores.end());
    sort(ranking.begin(), ranking.end(),
         [](const pair<int,double>& a, const pair<int,double>& b) {
             return a.second > b.second;
         });

    int limit = min((int)ranking.size(), numDocumentos);
    for (int i = 0; i < limit; i++) {
        ResultadoRI r;
        r.numPregunta = numPregunta;
        r.idDoc       = ranking[i].first;
        r.vSimilitud  = ranking[i].second;
        docsOrdenados.push(r);
    }
    return true;
}

  //////////////////////////////////////////////////////
 //   BUSCAR                                         //
//////////////////////////////////////////////////////

bool Buscador::Buscar(const int& numDocumentos) {
    while (!docsOrdenados.empty()) docsOrdenados.pop();

    if (infPregunta.ObtenerNumTotalPalSinParada() == 0) return false;

    return ScoreDocumentos(0, numDocumentos);
}

bool Buscador::Buscar(const string& dirPreguntas, const int& numDocumentos,
                      const int& numPregInicio, const int& numPregFin) {
    while (!docsOrdenados.empty()) docsOrdenados.pop();

    for (int i = numPregInicio; i <= numPregFin; i++) {
        string rutaFich = dirPreguntas + "/" + to_string(i) + ".txt";

        ifstream f(rutaFich.c_str());
        if (!f) continue;

        string texto, linea;
        while (getline(f, linea)) {
            if (!texto.empty()) texto += " ";
            texto += linea;
        }
        f.close();

        if (!IndexarPregunta(texto)) continue;

        if (!ScoreDocumentos(i, numDocumentos)) {
            cerr << " Pregunta: " << i << endl;
            return false;
        }
    }
    return true;
}

  //////////////////////////////////////////////////////
 //   IMPRIMIR RESULTADOS                            //
//////////////////////////////////////////////////////

// Extrae el nombre del fichero sin ruta ni extension
static string extraerNombreDoc(const string& ruta) {
    size_t sep = ruta.rfind('/');
    string nombre = (sep != string::npos) ? ruta.substr(sep + 1) : ruta;
    size_t punto  = nombre.rfind('.');
    if (punto != string::npos) nombre = nombre.substr(0, punto);
    return nombre;
}

void Buscador::PrintResultsTo(ostream& s, int numDocumentos) const {
    // Construir mapa inverso idDoc -> nombre del fichero
    unordered_map<int, string> idANombre;
    idANombre.reserve(indiceDocs.size());
    for (const auto& kv : indiceDocs)
        idANombre[kv.second.ObtenerIdDoc()] = kv.first;

    // Extraer todos los resultados y agrupar por numPregunta
    priority_queue<ResultadoRI> copia = docsOrdenados;
    map<int, vector<ResultadoRI>> grupos;
    while (!copia.empty()) {
        grupos[copia.top().numPregunta].push_back(copia.top());
        copia.pop();
    }

    string formula = (formSimilitud == 0) ? "DFR" : "BM25";

    // Forzar separador decimal ingles
    locale localeC("C");
    s.imbue(localeC);

    for (auto& kv : grupos) {
        auto& docs = kv.second;
        // Ordenar por puntuacion descendente dentro del grupo
        sort(docs.begin(), docs.end(),
             [](const ResultadoRI& a, const ResultadoRI& b) {
                 return a.vSimilitud > b.vSimilitud;
             });

        string pregImpresa = (kv.first == 0) ? pregunta : "ConjuntoDePreguntas";
        int limit = min((int)docs.size(), numDocumentos);

        for (int pos = 0; pos < limit; pos++) {
            auto nomIt = idANombre.find(docs[pos].idDoc);
            string nomDoc = (nomIt != idANombre.end())
                            ? extraerNombreDoc(nomIt->second)
                            : "";

            s << kv.first      << " "
              << formula       << " "
              << nomDoc        << " "
              << pos           << " "
              << setprecision(15) << docs[pos].vSimilitud << " "
              << pregImpresa   << "\n";
        }
    }
}

void Buscador::ImprimirResultadoBusqueda(const int& numDocumentos) const {
    PrintResultsTo(cout, numDocumentos);
}

bool Buscador::ImprimirResultadoBusqueda(const int& numDocumentos,
                                          const string& nombreFichero) const {
    ofstream f(nombreFichero.c_str());
    if (!f) return false;
    PrintResultsTo(f, numDocumentos);
    return true;
}

  //////////////////////////////////////////////////////
 //   GETTERS / SETTERS                              //
//////////////////////////////////////////////////////

int Buscador::DevolverFormulaSimilitud() const {
    return formSimilitud;
}

bool Buscador::CambiarFormulaSimilitud(const int& f) {
    if (f != 0 && f != 1) return false;
    formSimilitud = f;
    return true;
}

void Buscador::CambiarParametrosDFR(const double& kc) {
    c = kc;
}

double Buscador::DevolverParametrosDFR() const {
    return c;
}

void Buscador::CambiarParametrosBM25(const double& kk1, const double& kb) {
    k1 = kk1;
    b  = kb;
}

void Buscador::DevolverParametrosBM25(double& kk1, double& kb) const {
    kk1 = k1;
    kb  = b;
}
