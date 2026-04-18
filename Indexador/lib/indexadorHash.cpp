#ifndef INDEXADOR_HASH_H
#define INDEXADOR_HASH_H

#include "indexadorInformacion.h"
#include "tokenizador.h" // Se asume que está en el mismo directorio o path de inclusión
#include <unordered_set>

class IndexadorHash {
    friend std::ostream& operator<<(std::ostream& s, const IndexadorHash& p);

public:
    // Constructor principal [cite: 226]
    IndexadorHash(const std::string& fichStopWords, const std::string& delimitadores,
                  const bool& detectComp, const bool& minuscSinAcentos, 
                  const std::string& dirIndice, const int& tStemmer, const bool& almPosTerm);

    // Carga de índice previo [cite: 238]
    IndexadorHash(const std::string& directorioIndexacion);

    // Métodos de indexación [cite: 246, 253]
    bool Indexar(const std::string& ficheroDocumentos);
    bool IndexarDirectorio(const std::string& dirAIndexar);

    // Persistencia [cite: 264, 272]
    bool GuardarIndexacion() const;
    bool RecuperarIndexacion(const std::string& directorioIndexacion);

    // Preguntas [cite: 282]
    bool IndexarPregunta(const std::string& preg);

    // Otros métodos obligatorios (Getters, Listar, Borrar) [cite: 317-357]
    bool BorraDoc(const std::string& nomDoc);
    void VaciarIndiceDocs();
    int NumPalIndexadas() const;

private:
    std::unordered_map<std::string, InformacionTermino> indice; // Términos -> Info [cite: 364]
    std::unordered_map<std::string, InfDoc> indiceDocs; // Nombredoc -> Info [cite: 366]
    InfColeccionDocs informacionColeccionDocs; // Stats globales [cite: 368]
    
    std::string pregunta; // Pregunta actual [cite: 374]
    std::unordered_map<std::string, InformacionTerminoPregunta> indicePregunta; // [cite: 376]
    InformacionPregunta infPregunta;

    std::unordered_set<std::string> stopWords; // Palabras de parada [cite: 379]
    Tokenizador tok; // Clase de la Práctica 1 [cite: 385]
    int tipoStemmer; // 0=no, 1=español, 2=inglés [cite: 390]
    bool almacenarPosTerm; // [cite: 401]
};

#endif