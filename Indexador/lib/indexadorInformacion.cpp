#include "../include/indexadorInformacion.h"
using namespace std; 


  //////////////////////////////////////////////////////
 //             CLASE InfTermDoc                     //
//////////////////////////////////////////////////////

InfTermDoc::InfTermDoc() {
    ft = 0;
}

InfTermDoc::InfTermDoc(const InfTermDoc& o) {
    ft = o.ft;
    posTerm = o.posTerm;
}

InfTermDoc::~InfTermDoc() {
    ft = 0;
    posTerm.clear();
}

InfTermDoc& InfTermDoc::operator=(const InfTermDoc& o) {
    if (this != &o) {
        ft = o.ft;
        posTerm = o.posTerm;
    }
    return *this;
}

// Se numerarán las palabras de parada. Estará ordenada de menor a mayor posición.
void InfTermDoc::AnadirOcurrencia(int posicion, bool guardarPos) {
    ft++;
    if (guardarPos) {
        posTerm.push_back(posicion);
    }
}

ostream& operator<<(ostream& s, const InfTermDoc& p) {
    s << "ft: " << p.ft;
    for (list<int>::const_iterator it = p.posTerm.begin(); it != p.posTerm.end(); ++it) {
        s << "\t" << *it;
    }
    return s;
}


  //////////////////////////////////////////////////////
 //     CLASE InformacionTermino                     //
//////////////////////////////////////////////////////

InformacionTermino::InformacionTermino() {
    ftc = 0;
    l_docs.clear();
}

InformacionTermino::InformacionTermino(const InformacionTermino& o) {
    ftc = o.ftc;
    l_docs = o.l_docs;
}

InformacionTermino::~InformacionTermino() {
    ftc = 0;
    l_docs.clear();
}

InformacionTermino& InformacionTermino::operator=(const InformacionTermino& o) {
    if (this != &o) {
        ftc = o.ftc;
        l_docs = o.l_docs;
    }
    return *this;
}

void InformacionTermino::AnadirOcurrenciaDoc(int idDoc, int posicion, bool guardarPos) {
    ftc++;
    l_docs[idDoc].AnadirOcurrencia(posicion, guardarPos);
}

ostream& operator<<(ostream& s, const InformacionTermino& p) {
    s << "Frecuencia total: " << p.ftc << "\tfd: " << p.l_docs.size();
    for (unordered_map<int, InfTermDoc>::const_iterator it = p.l_docs.begin(); it != p.l_docs.end(); ++it) {
        s << "\tId.Doc: " << it->first << "\t" << it->second;
    }
    return s;
}


  //////////////////////////////////////////////////////
 //     CLASE InfDoc                                 //
//////////////////////////////////////////////////////

InfDoc::InfDoc() {
    idDoc = 0;
    numPal = 0;
    numPalSinParada = 0;
    numPalDiferentes = 0;
    tamBytes = 0;
}

InfDoc::InfDoc(const InfDoc& o) {
    idDoc = o.idDoc;
    numPal = o.numPal;
    numPalSinParada = o.numPalSinParada;
    numPalDiferentes = o.numPalDiferentes;
    tamBytes = o.tamBytes;
    fechaModificacion = o.fechaModificacion;
}

InfDoc::~InfDoc() {
    idDoc = 0;
    numPal = 0;
    numPalSinParada = 0;
    numPalDiferentes = 0;
    tamBytes = 0;
}

InfDoc& InfDoc::operator=(const InfDoc& o) {
    if (this != &o) {
        idDoc = o.idDoc;
        numPal = o.numPal;
        numPalSinParada = o.numPalSinParada;
        numPalDiferentes = o.numPalDiferentes;
        tamBytes = o.tamBytes;
        fechaModificacion = o.fechaModificacion;
    }
    return *this;
}

void InfDoc::Inicializar(int id, int nPal, int nPalSinP, int nPalDif, int bytes, const Fecha& f) {
    idDoc = id;
    numPal = nPal;
    numPalSinParada = nPalSinP;
    numPalDiferentes = nPalDif;
    tamBytes = bytes;
    fechaModificacion = f;
}

ostream& operator<<(ostream& s, const InfDoc& p) {
    s << "idDoc: " << p.idDoc
      << "\tnumPal: " << p.numPal
      << "\tnumPalSinParada: " << p.numPalSinParada
      << "\tnumPalDiferentes: " << p.numPalDiferentes
      << "\ttamBytes: " << p.tamBytes;
    return s;
}


  //////////////////////////////////////////////////////
 //     CLASE InfColeccionDocs                       //
//////////////////////////////////////////////////////

InfColeccionDocs::InfColeccionDocs() {
    numDocs = 0;
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
    tamBytes = 0;
}

InfColeccionDocs::InfColeccionDocs(const InfColeccionDocs& o) {
    numDocs = o.numDocs;
    numTotalPal = o.numTotalPal;
    numTotalPalSinParada = o.numTotalPalSinParada;
    numTotalPalDiferentes = o.numTotalPalDiferentes;
    tamBytes = o.tamBytes;
}

InfColeccionDocs::~InfColeccionDocs() {
    numDocs = 0;
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
    tamBytes = 0;
}

InfColeccionDocs& InfColeccionDocs::operator=(const InfColeccionDocs& o) {
    if (this != &o) {
        numDocs = o.numDocs;
        numTotalPal = o.numTotalPal;
        numTotalPalSinParada = o.numTotalPalSinParada;
        numTotalPalDiferentes = o.numTotalPalDiferentes;
        tamBytes = o.tamBytes;
    }
    return *this;
}

void InfColeccionDocs::AnadirDoc(const InfDoc& doc) {
    numDocs++;
    numTotalPal += doc.ObtenerNumPal();
    numTotalPalSinParada += doc.ObtenerNumPalSinParada();
    tamBytes += doc.ObtenerTamBytes();
    // numTotalPalDiferentes se deberá ajustar por separado durante la indexación real.
}

void InfColeccionDocs::EliminarDoc(const InfDoc& doc) {
    numDocs--;
    numTotalPal -= doc.ObtenerNumPal();
    numTotalPalSinParada -= doc.ObtenerNumPalSinParada();
    tamBytes -= doc.ObtenerTamBytes();
}

void InfColeccionDocs::AjustarPalDiferentes(int delta) {
    numTotalPalDiferentes += delta;
}

void InfColeccionDocs::Vaciar() {
    numDocs = 0;
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
    tamBytes = 0;
}

ostream& operator<<(ostream& s, const InfColeccionDocs& p) {
    s << "numDocs: " << p.numDocs
      << "\tnumTotalPal: " << p.numTotalPal
      << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes
      << "\ttamBytes: " << p.tamBytes;
    return s;
}


  //////////////////////////////////////////////////////
 //     CLASE InformacionTerminoPregunta             //
//////////////////////////////////////////////////////

InformacionTerminoPregunta::InformacionTerminoPregunta() {
    ft = 0;
}

InformacionTerminoPregunta::InformacionTerminoPregunta(const InformacionTerminoPregunta& o) {
    ft = o.ft;
    posTerm = o.posTerm;
}

InformacionTerminoPregunta::~InformacionTerminoPregunta() {
    ft = 0;
    posTerm.clear();
}

InformacionTerminoPregunta& InformacionTerminoPregunta::operator=(const InformacionTerminoPregunta& o) {
    if (this != &o) {
        ft = o.ft;
        posTerm = o.posTerm;
    }
    return *this;
}

void InformacionTerminoPregunta::AnadirOcurrencia(int posicion, bool guardarPos) {
    ft++;
    if (guardarPos) {
        posTerm.push_back(posicion);
    }
}

ostream& operator<<(ostream& s, const InformacionTerminoPregunta& p) {
    s << "ft: " << p.ft;
    for (list<int>::const_iterator it = p.posTerm.begin(); it != p.posTerm.end(); ++it) {
        s << "\t" << *it;
    }
    return s;
}


  //////////////////////////////////////////////////////
 //     CLASE InformacionPregunta                    //
//////////////////////////////////////////////////////

InformacionPregunta::InformacionPregunta() {
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
}

InformacionPregunta::InformacionPregunta(const InformacionPregunta& o) {
    numTotalPal = o.numTotalPal;
    numTotalPalSinParada = o.numTotalPalSinParada;
    numTotalPalDiferentes = o.numTotalPalDiferentes;
}

InformacionPregunta::~InformacionPregunta() {
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
}

InformacionPregunta& InformacionPregunta::operator=(const InformacionPregunta& o) {
    if (this != &o) {
        numTotalPal = o.numTotalPal;
        numTotalPalSinParada = o.numTotalPalSinParada;
        numTotalPalDiferentes = o.numTotalPalDiferentes;
    }
    return *this;
}

void InformacionPregunta::Inicializar(int nPal, int nPalSinP, int nPalDif) {
    numTotalPal = nPal;
    numTotalPalSinParada = nPalSinP;
    numTotalPalDiferentes = nPalDif;
}
    
void InformacionPregunta::Vaciar() {
    numTotalPal = 0;
    numTotalPalSinParada = 0;
    numTotalPalDiferentes = 0;
}

ostream& operator<<(ostream& s, const InformacionPregunta& p) {
    s << "numTotalPal: " << p.numTotalPal
      << "\tnumTotalPalSinParada: " << p.numTotalPalSinParada
      << "\tnumTotalPalDiferentes: " << p.numTotalPalDiferentes;
    return s;
}