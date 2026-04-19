#include "../include/indexadorHash.h"
using namespace std; 
#include <sys/stat.h>  // Necesario para la función mkdir() en Linux

  //////////////////////////////////////////////////////
 //     CONSTRUCTORES, DESTRUCTORES Y OPERADORES     //
//////////////////////////////////////////////////////

IndexadorHash::IndexadorHash(){
    ficheroStopWords = ""; 
    directorioIndice = ""; 
    //Por defecto no hay stemmer
    tipoStemmer = 0; 
    almacenarPosTerm = false; 
    pregunta = "";
}

IndexadorHash::IndexadorHash(const string& fichStopWords,const string& delimitadores,const bool&   detectComp,                  const bool&   minuscSinAcentos, const string& dirIndice, const int& tStemmer, const bool& almPosTerm){
    //Tokenizador
    tok = Tokenizador(delimitadores,detectComp,minuscSinAcentos); 

    ficheroStopWords = fichStopWords; 
    directorioIndice = dirIndice; 
    tipoStemmer = tStemmer; 
    almacenarPosTerm = almPosTerm; 
    pregunta = "";
    CargarStopWords(ficheroStopWords);
}

IndexadorHash::IndexadorHash(const string& directorioIndexacion) {
    ficheroStopWords = "";
    directorioIndice = directorioIndexacion;
    tipoStemmer = 0;
    almacenarPosTerm = false;
    pregunta = "";

    // delego toda la carga a RecuperarIndexacion
    if (!RecuperarIndexacion(directorioIndexacion)) {
        cerr << "ERROR: No se ha podido recuperar la indexacion del directorio: " << directorioIndexacion << endl;
    }
}

IndexadorHash::IndexadorHash(const IndexadorHash& o) {
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

IndexadorHash::~IndexadorHash() {
    indice.clear();
    indiceDocs.clear();
    informacionColeccionDocs.Vaciar();
    pregunta = "";
    indicePregunta.clear();
    infPregunta.Vaciar();
    stopWords.clear();
}

bool IndexadorHash::Indexar(const string& ficheroDocumentos){
    ifstream listaFicheros(ficheroDocumentos.c_str());
    if(!listaFicheros){
        cerr << "ERROR: no se ha podido abrir lista documentos" << endl;
        return false; 
    }
    bool estadoFinal = true; 
    string nomDoc; 

    //Leo linea a linea cada nombre de los documentos. 
    while(getline(listaFicheros,nomDoc)){
        //si la linea esta vacia la salto
        if(nomDoc.empty()){
            continue; 
        }

        // Comprobacion reindexacion

        //Declaro la estructura en la que almaceno el archivo. 
        struct stat infoArchivo; 

        //Si el archivo no existe emito error y siguiente
        if(stat(nomDoc.c_str(), &infoArchivo) != 0){ // si es true es que ahora nomdoc se aloja en infoArchivo. Lo usamos para mirar los metadatos del archivo 
            cerr <<"ERROR: no se ha podido acceder al documento a indexar" << endl; 
            continue; 
        }

        //si se ha asignado bien los metadatos, en st_mtime tienes la fecha del documento. 
        Fecha fechaActual(infoArchivo.st_mtime);
        int idDocAUsar = -1; 

        //Miro si ya existe el indice 
        unordered_map<string,InfDoc>::iterator it = indiceDocs.find(nomDoc);

        //Si no encuentra el archivo el it apunta al final de indiceDocs
        //Si ya hemos procesado el archivo antes, entonces entramos en le if para dejar el mas reciente. 
        if(it != indiceDocs.end()){
            //Obtengo la fecha 
            Fecha fechaGuardada = it->second.ObtenerFecha();

            //Comparamos cual es el mas reciente
            if(fechaActual.esMasReciente(fechaGuardada)){
                idDocAUsar = it->second.ObtenerIdDoc(); 
                BorraDoc(nomDoc); //Borro los datos del documento viejo. 
            }else{
                //Si no es mas reciente, lo ignoramos
                continue; 
            }
        }else{
            //El archivo es nuevo, por lo que hay que asignarle un id
            idDocAUsar = SiguienteIdDoc(); 
        }
        
        //funcion auxiliar
        if(!IndexarDocumento(nomDoc,idDocAUsar,infoArchivo.st_size,fechaActual)){
            listaFicheros.close();
            //si diese algun fallo devuelvo false.
            estadoFinal = false; 
            break; 
        }
    }

    listaFicheros.close(); 
    return estadoFinal; 
}

bool IndexadorHash::IndexarDocumento(const string& nomDoc,int idDoc, long tamBytes, const Fecha& fecha){

    ifstream fichero(nomDoc.c_str()); 
    if(!fichero){
        cerr << "ERROR: no se puede abrir el documento" << endl;
    }

    //Variables de este documento
    int numPal = 0; 
    int numPalSinParada = 0; 
    int posicionActual = 0; 

    //declaro un conjunto para contar las palabras diferentes (sin stopwords) tiene el documento. 
    unordered_set<string> terminosUnicosDoc; 

    string linea; 
    list<string> tokensLinea; 
    stemmerPorter miStemmer; //el stemmer a usar. 

    //Leemos el documento linea a linea
    while(getline(fichero,linea)){
        if(linea.empty()){continue;}

        //Tokenizo cada linea con el tokenizador
        tok.Tokenizar(linea,tokensLinea);

        //recorro cada palabra de la linea
        for(list<string>::const_iterator it = tokensLinea.begin(); it != tokensLinea.end() ; ++it){
            string termino = *it; 
            numPal++; //anado uno al total de palabras

             //es una palabra de parada? 
            if(stopWords.find(termino) != stopWords.end()){
                //SI es palabra de parada, la ignoramos pero la posicion avanza
                posicionActual++; 
                continue; 
            }

            //si el stemmer esta activo lo aplico 
            string terminoProcesado = termino; 
            if(tipoStemmer == 1){
                miStemmer.stemmer(terminoProcesado,1); //espanol
            }else if(tipoStemmer ==2){
                miStemmer.stemmer(terminoProcesado,2);//ingles
            }

            //palabra valida
            numPalSinParada++; 
            terminosUnicosDoc.insert(terminoProcesado);

            //miro si el termino es nuevo o existe
            bool esTerminoNuevo = !Existe(terminoProcesado);

            //aqui accedo al unordered_map, si el objeto no existe lo crea, si existe lo devuelve
            //con anadirocurrencia anado el nuevo termino. 
            try {
                 // Intentamos insertar/modificar en el unordered_map
                indice[terminoProcesado].AnadirOcurrenciaDoc(idDoc, posicionActual, almacenarPosTerm);
            } 
            catch (const std::bad_alloc& e) {
                 // Capturamos la falta de memoria
                cerr << "ERROR: Falta de memoria indexando el documento " << nomDoc 
                     << " en el termino " << terminoProcesado << endl;
                fichero.close();
                return false; 
            }
            //si es nuevo sumamos 1 al contador de palabras unicas. 
            if(esTerminoNuevo){
                informacionColeccionDocs.AjustarPalDiferentes(1);
            }

            //avanza la posicion actual
            posicionActual++;
        }
    }

    fichero.close(); 

    //guardamos la informacion del documento en indiceDocs
    InfDoc infoDelDocumento; 
    infoDelDocumento.Inicializar(idDoc,numPal,numPalSinParada,terminosUnicosDoc.size(),tamBytes,fecha);
    indiceDocs[nomDoc] = infoDelDocumento; 

    //actualizo los contadores globales a la coleccion 
    informacionColeccionDocs.AnadirDoc(infoDelDocumento);

    return true; 
}

bool IndexadorHash::IndexarDirectorio(const string& dirAIndexar){
    struct stat dir; 

    //si stat devuelve -1 no existe el directorio
    //compruebo si existe, entra en el bucle si no existe o no es directorio. 
    if(stat(dirAIndexar.c_str(),&dir) == -1 || !S_ISDIR(dir.st_mode)){
        cerr<< "ERROR: no existe el directorio" << endl;
        return false; 
    }

    //creo un fichero temporal oculto para guardar la lista de archivos 
    string ficheroListaTmp = ".lista_fich_indexador";

    //creo un comando de consola que ejecuto con system
    // -type f hace que solo busquemos ficheros
    // sort para hacerlo en orden alfabetico 
    string cmd = "find " + dirAIndexar + " -type f | sort > " + ficheroListaTmp;
    
    // Ejecutamos el comando. Ahora tenemos un archivo de texto con la lista de documentos.
    system(cmd.c_str()); //c_str para traducir a un array de caracteres de c original, para la consola. 

    //llamo al metodo indexar con el fichero temporal para que lo gestione, 
    bool estadoFinal = Indexar(ficheroListaTmp);

    //borro el fichero temporal 
    string cmdBorrar = "rm -f " + ficheroListaTmp;
    system(cmdBorrar.c_str());
    return estadoFinal; 

}

bool IndexadorHash::IndexarPregunta(const string& preg){
    //vacio pregunta anterior
    pregunta = ""; 
    indicePregunta.clear(); 
    infPregunta.Vaciar(); 

    //cadena vacia = nada que indexar
    if(preg.empty()){
        cerr<<"ERROR: pregunta vacia" << endl;
        return false; 
    }

    //vars de la pregunta

    int numPal = 0 ; 
    int numPalSinParada = 0 ; 
    int posicionActual = 0 ; 
    unordered_set<string> terminosUnicosPregunta; 
    list<string> tokensPregunta; 
    stemmerPorter miStemmer; 

    //tokenizo la pregunta completa 
    tok.Tokenizar(preg,tokensPregunta);

    //proceso tokens
    for(list<string>::const_iterator it = tokensPregunta.begin(); it != tokensPregunta.end(); it++){
        string termino = *it; 
        numPal++; 

        //palabra parada? 
        if(stopWords.find(termino) != stopWords.end()){
            posicionActual++; 
            continue; 
        }

        //aplico el stemmer si toca
        string terminoProcesado =termino; 
        if(tipoStemmer == 1){
            miStemmer.stemmer(terminoProcesado,1);
        }else if(tipoStemmer == 2){
            miStemmer.stemmer(terminoProcesado,2);
        }
    
        //palabra valida
        numPalSinParada++; 
        terminosUnicosPregunta.insert(terminoProcesado);

        // Añadir ocurrencia en el índice exclusivo de la pregunta
        indicePregunta[terminoProcesado].AnadirOcurrencia(posicionActual, almacenarPosTerm);
        posicionActual++;
    }

    //si la pregunta no tiene ningun termino eran todo stopwords
    if(numPalSinParada == 0 ){
        cerr << "ERROR: La pregunta no contiene terminos validos" << endl;
        return false;
    }

    //guardo toda la informacion 
    pregunta = preg; 
    infPregunta.Inicializar(numPal,numPalSinParada,terminosUnicosPregunta.size());
    return true; 
}

//devuelve la pregunta en preg
bool IndexadorHash::DevuelvePregunta(string& preg) const {
    // Si hay una pregunta indexada (infPregunta no está vacía)
    if (infPregunta.ObtenerNumTotalPalSinParada() > 0) {
        preg = pregunta;
        return true;
    }
    return false;
}

//a partir de una palabra te devuelve la informacion de esta en la pregunta
bool IndexadorHash::DevuelvePregunta(const string& word, InformacionTerminoPregunta& inf) const {
    
    // aplicamos a la palabra tokenizado
    list<string> tokensBusqueda;
    tok.Tokenizar(word, tokensBusqueda);
    
    // Si al tokenizar la palabra desaparece entonces no existe
    if (tokensBusqueda.empty()) {
        inf = InformacionTerminoPregunta();
        return false;
    }
    
    // Nos quedamos con la palabra procesada
    string terminoProcesado = tokensBusqueda.front(); 

    //aplico el steamming 
    stemmerPorter miStemmer;
    if (tipoStemmer == 1) {
        miStemmer.stemmer(terminoProcesado, 1);
    } else if (tipoStemmer == 2) {
        miStemmer.stemmer(terminoProcesado, 2);
    }

    // busco el termino en el indice de la pregunta
    unordered_map<string, InformacionTerminoPregunta>::const_iterator it = indicePregunta.find(terminoProcesado);
    
    //si esta, guardamos la informacion en inf 
    if (it != indicePregunta.end()) {
        inf = it->second;
        return true;
    }
    
    // Si no está, devolvemos inf vacío
    inf = InformacionTerminoPregunta(); 
    return false;
}

//en inf queda la informacion de la pregunta 
bool IndexadorHash::DevuelvePregunta(InformacionPregunta& inf) const {
    if (infPregunta.ObtenerNumTotalPalSinParada() > 0) {
        inf = infPregunta;
        return true;
    }
    
    inf = InformacionPregunta();
    return false;
}

  //////////////////////////////////////////////////////
 //          MÉTODOS AUXILIARES PRIVADOS             //
//////////////////////////////////////////////////////

void IndexadorHash::CargarStopWords(const string& fichero) {
    ifstream fichStop(fichero.c_str());
    if (!fichStop) {
        cerr << "ERROR: no se ha podido abrir fichero stopwords" << endl;
        return; 
    }
    string linea;
    stemmerPorter miStemmer; 
    list<string> tokensStop;

    while (getline(fichStop, linea)) {
        if (linea.empty()) continue;
        tokensStop.clear();
        tok.Tokenizar(linea, tokensStop);
        for (list<string>::iterator it = tokensStop.begin(); it != tokensStop.end(); ++it) {
            stopWords.insert(*it);
        }
    }
    fichStop.close();
}

string IndexadorHash::TransformarTerm(const string& word) const {
    list<string> tokensBusqueda;
    tok.Tokenizar(word, tokensBusqueda);
    if (tokensBusqueda.empty()) return "";
    
    string terminoProcesado = tokensBusqueda.front(); 
    stemmerPorter miStemmer;
    if (tipoStemmer == 1) miStemmer.stemmer(terminoProcesado, 1);
    else if (tipoStemmer == 2) miStemmer.stemmer(terminoProcesado, 2);
    
    return terminoProcesado;
}

int IndexadorHash::SiguienteIdDoc() const {
    int maxId = 0;
    for (unordered_map<string, InfDoc>::const_iterator it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
        if (it->second.idDoc > maxId) {
            maxId = it->second.idDoc;
        }
    }
    return maxId + 1; // El primer idDoc será 1 si el mapa está vacío
}


  //////////////////////////////////////////////////////
 //           MÉTODOS DE BORRADO Y VACIADO           //
//////////////////////////////////////////////////////

bool IndexadorHash::BorraDoc(const string& nomDoc) {
    unordered_map<string, InfDoc>::iterator itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false; 

    int idDocABorrar = itDoc->second.idDoc;
    informacionColeccionDocs.EliminarDoc(itDoc->second);

    unordered_map<string, InformacionTermino>::iterator itTermino = indice.begin();
    while (itTermino != indice.end()) {
        unordered_map<int, InfTermDoc>::iterator itAparicion = itTermino->second.l_docs.find(idDocABorrar);
        if (itAparicion != itTermino->second.l_docs.end()) {
            itTermino->second.ftc -= itAparicion->second.ft;
            itTermino->second.l_docs.erase(itAparicion);
        }
        if (itTermino->second.l_docs.empty()) {
            itTermino = indice.erase(itTermino);
            informacionColeccionDocs.AjustarPalDiferentes(-1);
        } else {
            ++itTermino;
        }
    }
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
 //              MÉTODOS DE CONSULTA                 //
//////////////////////////////////////////////////////

bool IndexadorHash::Existe(const string& word) const {
    string term = TransformarTerm(word);
    if(term.empty()) return false;
    return (indice.find(term) != indice.end());
}

bool IndexadorHash::Devuelve(const string& word, InformacionTermino& inf) const {
    string term = TransformarTerm(word);
    if(term.empty()) return false;

    unordered_map<string, InformacionTermino>::const_iterator it = indice.find(term);
    if (it != indice.end()) {
        inf = it->second;
        return true;
    }
    inf = InformacionTermino(); 
    return false;
}

bool IndexadorHash::Devuelve(const string& word, const string& nomDoc, InfTermDoc& infDoc) const {
    string term = TransformarTerm(word);
    if(term.empty()) return false;

    unordered_map<string, InfDoc>::const_iterator itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) {
        infDoc = InfTermDoc();
        return false;
    }

    unordered_map<string, InformacionTermino>::const_iterator itTerm = indice.find(term);
    if (itTerm != indice.end()) {
        unordered_map<int, InfTermDoc>::const_iterator itAparicion = itTerm->second.l_docs.find(itDoc->second.idDoc);
        if (itAparicion != itTerm->second.l_docs.end()) {
            infDoc = itAparicion->second;
            return true;
        }
    }
    infDoc = InfTermDoc();
    return false;
}


  //////////////////////////////////////////////////////
 //           GETTERS Y MÉTODOS SIMPLES              //
//////////////////////////////////////////////////////

int IndexadorHash::NumPalIndexadas() const {
    return indice.size();
}

string IndexadorHash::DevolverFichPalParada() const {
    return ficheroStopWords;
}

void IndexadorHash::ListarPalParada() const {
    for (unordered_set<string>::const_iterator it = stopWords.begin(); it != stopWords.end(); ++it) {
        cout << *it << endl;
    }
}

int IndexadorHash::NumPalParada() const {
    return stopWords.size();
}

string IndexadorHash::DevolverDelimitadores() const {
    return tok.DelimitadoresPalabra();
}

bool IndexadorHash::DevolverCasosEspeciales() const {
    // const_cast es para no modificar el codigo del tokenizador y quitarle el const al metodo. 
    return const_cast<Tokenizador&>(tok).CasosEspeciales();
}

bool IndexadorHash::DevolverPasarAminuscSinAcentos() const {
    return const_cast<Tokenizador&>(tok).PasarAminuscSinAcentos();
}

bool IndexadorHash::DevolverAlmacenarPosTerm() const {
    return almacenarPosTerm;
}

string IndexadorHash::DevolverDirIndice() const {
    return directorioIndice;
}

int IndexadorHash::DevolverTipoStemming() const {
    return tipoStemmer;
}

void IndexadorHash::ListarInfColeccDocs() const {
    cout << informacionColeccionDocs << endl;
}

void IndexadorHash::ListarTerminos() const {
    for (unordered_map<string, InformacionTermino>::const_iterator it = indice.begin(); it != indice.end(); ++it) {
        cout << it->first << '\t' << it->second << endl;
    }
}

bool IndexadorHash::ListarTerminos(const string& nomDoc) const {
    unordered_map<string, InfDoc>::const_iterator itDoc = indiceDocs.find(nomDoc);
    if (itDoc == indiceDocs.end()) return false;

    int idDelDoc = itDoc->second.idDoc;
    for (unordered_map<string, InformacionTermino>::const_iterator it = indice.begin(); it != indice.end(); ++it) {
        if (it->second.l_docs.find(idDelDoc) != it->second.l_docs.end()) {
            cout << it->first << '\t' << it->second << endl;
        }
    }
    return true;
}

void IndexadorHash::ListarDocs() const {
    for (unordered_map<string, InfDoc>::const_iterator it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
        cout << it->first << '\t' << it->second << endl;
    }
}

bool IndexadorHash::ListarDocs(const string& nomDoc) const {
    unordered_map<string, InfDoc>::const_iterator it = indiceDocs.find(nomDoc);
    if (it != indiceDocs.end()) {
        cout << it->first << '\t' << it->second << endl;
        return true;
    }
    return false;
}


  //////////////////////////////////////////////////////
 //             PERSISTENCIA (GUARDAR/CARGAR)        //
//////////////////////////////////////////////////////

bool IndexadorHash::GuardarIndexacion() const {
    if (directorioIndice.empty()) {
        cerr << "ERROR: Directorio de indice vacio" << endl;
        return false;
    }

    // Crea el directorio (0755 son permisos estandar de Linux)
    mkdir(directorioIndice.c_str(), 0755);

    // 1. GUARDAR CONFIGURACIÓN
    string rutaConfig = directorioIndice + "/config.txt";
    ofstream fConfig(rutaConfig.c_str());
    if (!fConfig) {
        cerr << "ERROR: No se pudo crear el archivo de configuracion" << endl;
        return false;
    }
    
    fConfig << ficheroStopWords << '\n' 
            << tipoStemmer << '\n' 
            << almacenarPosTerm << '\n'
            << tok.DelimitadoresPalabra() << '\n' 
            << const_cast<Tokenizador&>(tok).CasosEspeciales() << '\n' 
            << const_cast<Tokenizador&>(tok).PasarAminuscSinAcentos() << '\n';
    fConfig.close();

    // 2. GUARDAR COLECCIÓN DOCS
    string rutaColeccion = directorioIndice + "/coleccion.txt";
    ofstream fCol(rutaColeccion.c_str());
    if (!fCol) {
        cerr << "ERROR: No se pudo crear el archivo de coleccion" << endl;
        return false;
    }
    fCol << informacionColeccionDocs.numDocs << " " 
         << informacionColeccionDocs.numTotalPal << " "
         << informacionColeccionDocs.numTotalPalSinParada << " " 
         << informacionColeccionDocs.numTotalPalDiferentes << " "
         << informacionColeccionDocs.tamBytes << '\n';
    fCol.close();

    // 3. GUARDAR ÍNDICE DE DOCUMENTOS (indiceDocs)
    string rutaDocs = directorioIndice + "/indiceDocs.txt";
    ofstream fDocs(rutaDocs.c_str());
    if (!fDocs) {
        cerr << "ERROR: No se pudo crear el archivo de documentos" << endl;
        return false;
    }
    fDocs << indiceDocs.size() << '\n';
    for (unordered_map<string, InfDoc>::const_iterator it = indiceDocs.begin(); it != indiceDocs.end(); ++it) {
        // En una linea el nombre del doc, en la siguiente sus datos.
        // NOTA SOBRE LA FECHA: Asumimos que tu clase 'Fecha' puede guardarse. Si no, guarda sus datos primarios.
        fDocs << it->first << '\n' 
              << it->second.idDoc << " " 
              << it->second.numPal << " " 
              << it->second.numPalSinParada << " " 
              << it->second.numPalDiferentes << " " 
              << it->second.tamBytes << '\n'
              << it->second.fechaModificacion.tiempo <<'\n';
    }
    fDocs.close();

    // 4. GUARDAR ÍNDICE PRINCIPAL (indice)
    string rutaIndice = directorioIndice + "/indice.txt";
    ofstream fInd(rutaIndice.c_str());
    if (!fInd) {
        cerr << "ERROR: No se pudo crear el archivo de indice" << endl;
        return false;
    }
    fInd << indice.size() << '\n';
    for (unordered_map<string, InformacionTermino>::const_iterator it = indice.begin(); it != indice.end(); ++it) {
        fInd << it->first << '\n'; // Termino (en una sola linea por si tiene espacios)
        fInd << it->second.ftc << " " << it->second.l_docs.size() << '\n';
        
        // Iteramos sobre los documentos donde aparece el termino
        for (unordered_map<int, InfTermDoc>::const_iterator itDocs = it->second.l_docs.begin(); itDocs != it->second.l_docs.end(); ++itDocs) {
            fInd << itDocs->first << " " << itDocs->second.ft << " " << itDocs->second.posTerm.size() << " ";
            
            // Si las posiciones estan guardadas, iteramos la lista
            if (almacenarPosTerm) {
                for (list<int>::const_iterator itPos = itDocs->second.posTerm.begin(); itPos != itDocs->second.posTerm.end(); ++itPos) {
                    fInd << *itPos << " ";
                }
            }
            fInd << '\n';
        }
    }
    fInd.close();

    // 5. GUARDAR ÍNDICE DE LA PREGUNTA (Si hay alguna)
    string rutaPreg = directorioIndice + "/pregunta.txt";
    ofstream fPreg(rutaPreg.c_str());
    if (!fPreg) {
        cerr << "ERROR: No se pudo crear el archivo de pregunta" << endl;
        return false;
    }
    fPreg << pregunta << '\n';
    if (!pregunta.empty()) {
        fPreg << infPregunta.numTotalPal << " " 
              << infPregunta.numTotalPalSinParada << " " 
              << infPregunta.numTotalPalDiferentes << '\n';
        
        fPreg << indicePregunta.size() << '\n';
        for (unordered_map<string, InformacionTerminoPregunta>::const_iterator it = indicePregunta.begin(); it != indicePregunta.end(); ++it) {
            fPreg << it->first << '\n';
            fPreg << it->second.ft << " " << it->second.posTerm.size() << " ";
            
            if (almacenarPosTerm) {
                for (list<int>::const_iterator itPos = it->second.posTerm.begin(); itPos != it->second.posTerm.end(); ++itPos) {
                    fPreg << *itPos << " ";
                }
            }
            fPreg << '\n';
        }
    }
    fPreg.close();

    return true;
}

bool IndexadorHash::RecuperarIndexacion(const string& directorioIndexacion) {
    // Vaciamos todo el sistema actual para evitar solapamientos
    VaciarIndiceDocs();
    VaciarIndicePreg();
    stopWords.clear();
    directorioIndice = directorioIndexacion;

    // 1. CARGAR CONFIGURACIÓN
    string rutaConfig = directorioIndice + "/config.txt";
    ifstream fConfig(rutaConfig.c_str());
    if (!fConfig) {
        cerr << "ERROR: Directorio invalido o faltan archivos" << endl;
        return false;
    }

    getline(fConfig, ficheroStopWords);
    fConfig >> tipoStemmer >> almacenarPosTerm;
    fConfig.ignore(); // Limpiamos el buffer del enter
    
    string delims;
    getline(fConfig, delims);
    
    bool casosEsp, minuscSinAc;
    fConfig >> casosEsp >> minuscSinAc;
    fConfig.close();

    // Restauramos objetos base
    tok = Tokenizador(delims, casosEsp, minuscSinAc);
    CargarStopWords(ficheroStopWords);

    // 2. CARGAR COLECCIÓN DOCS
    string rutaColeccion = directorioIndice + "/coleccion.txt";
    ifstream fCol(rutaColeccion.c_str());
    if (!fCol) return false;
    
    fCol >> informacionColeccionDocs.numDocs 
         >> informacionColeccionDocs.numTotalPal 
         >> informacionColeccionDocs.numTotalPalSinParada 
         >> informacionColeccionDocs.numTotalPalDiferentes 
         >> informacionColeccionDocs.tamBytes;
    fCol.close();

    // 3. CARGAR ÍNDICE DE DOCUMENTOS
    string rutaDocs = directorioIndice + "/indiceDocs.txt";
    ifstream fDocs(rutaDocs.c_str());
    if (!fDocs) return false;
    
    int numDocumentosGuardados;
    fDocs >> numDocumentosGuardados;
    fDocs.ignore();
    
    for (int i = 0; i < numDocumentosGuardados; ++i) {
        string nomDoc;
        getline(fDocs, nomDoc);
        
        InfDoc docInfo;
        time_t tiempoGuardado; 
        fDocs >> docInfo.idDoc >> docInfo.numPal >> docInfo.numPalSinParada >> docInfo.numPalDiferentes >> docInfo.tamBytes >> tiempoGuardado;
        docInfo.fechaModificacion = Fecha(tiempoGuardado);
        fDocs.ignore();
        
        indiceDocs[nomDoc] = docInfo;
    }
    fDocs.close();

    // 4. CARGAR ÍNDICE PRINCIPAL
    string rutaIndice = directorioIndice + "/indice.txt";
    ifstream fInd(rutaIndice.c_str());
    if (!fInd) return false;
    
    int numTerminos;
    fInd >> numTerminos;
    fInd.ignore();
    
    for (int i = 0; i < numTerminos; ++i) {
        string termino;
        getline(fInd, termino);
        
        InformacionTermino infoTerm;
        int numDocsAsociados;
        fInd >> infoTerm.ftc >> numDocsAsociados;
        
        for (int j = 0; j < numDocsAsociados; ++j) {
            int idDoc, numPosiciones;
            InfTermDoc infoTermDoc;
            
            fInd >> idDoc >> infoTermDoc.ft >> numPosiciones;
            
            if (almacenarPosTerm) {
                for (int k = 0; k < numPosiciones; ++k) {
                    int pos;
                    fInd >> pos;
                    infoTermDoc.posTerm.push_back(pos);
                }
            }
            infoTerm.l_docs[idDoc] = infoTermDoc;
        }
        fInd.ignore(10000, '\n'); //limpiando el enter
        indice[termino] = infoTerm;
    }
    fInd.close();

    // 5. CARGAR ÍNDICE PREGUNTA (Solo si existe)
    string rutaPreg = directorioIndice + "/pregunta.txt";
    ifstream fPreg(rutaPreg.c_str());
    if (fPreg) {
        getline(fPreg, pregunta);
        if (!pregunta.empty()) {
            fPreg >> infPregunta.numTotalPal >> infPregunta.numTotalPalSinParada >> infPregunta.numTotalPalDiferentes;
            
            int numTermsPreg;
            fPreg >> numTermsPreg;
            fPreg.ignore();
            
            for (int i = 0; i < numTermsPreg; ++i) {
                string terminoPreg;
                getline(fPreg, terminoPreg);
                
                InformacionTerminoPregunta infoTermPreg;
                int numPosiciones;
                
                fPreg >> infoTermPreg.ft >> numPosiciones;
                if (almacenarPosTerm) {
                    for (int k = 0; k < numPosiciones; ++k) {
                        int pos;
                        fPreg >> pos;
                        infoTermPreg.posTerm.push_back(pos);
                    }
                }
                fPreg.ignore();
                indicePregunta[terminoPreg] = infoTermPreg;
            }
        }
        fPreg.close();
    }

    return true;
}
