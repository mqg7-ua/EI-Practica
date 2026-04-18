#include "../include/indexadorHash.h"
using namespace std; 

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
        cerr << "ERROR: no se ha podido abrir lista documentos";
        return false; 
    }
    bool estadoFinal = false; 
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
            cerr <<"ERROR: no se ha podido acceder al documento a indexar"; 
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
        cerr << "ERROR: no se puede abrir el documento";
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

            //si el stemmer esta activo lo aplico 
            string terminoProcesado = termino; 
            if(tipoStemmer == 1){
                miStemmer.stemmer(terminoProcesado,1); //espanol
            }else if(tipoStemmer ==2){
                miStemmer.stemmer(terminoProcesado,2);//ingles
            }

            //es una palabra de parada? 
            if(stopWords.find(terminoProcesado) != stopWords.end()){
                //SI es palabra de parada, la ignoramos pero la posicion avanza
                posicionActual++; 
                continue; 
            }

            //palabra valida
            numPalSinParada++; 
            terminosUnicosDoc.insert(terminoProcesado);

            //miro si el termino es nuevo o existe
            bool esTerminoNuevo = !Existe(terminoProcesado);

            //aqui accedo al unordered_map, si el objeto no existe lo crea, si existe lo devuelve
            //con anadirocurrencia anado el nuevo termino. 
            indice[terminoProcesado].AnadirOcurrenciaDoc(idDoc,posicionActual,almacenarPosTerm);

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
        cerr<< "ERROR: no exixte el directorio";
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
    string cmdBorrar = "rm -f" + ficheroListaTmp;
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
        cerr<<"ERROR: pregunta vacia";
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
    for(list<string>::const_iterator it = tokensPregunta.begin();it != tokensPregunta; it++){
        string termino = *it; 
        numPal; 

        //aplico el stemmer si toca
        string terminoProcesado =termino; 
        if(tipoStemmer == 1){
            miStemmer.stemmer(terminoProcesado,1);
        }else if(tipoStemmer == 2){
            miStemmer.stemmer(terminoProcesado,2);
        }

        //palabra parada? 
        if(stopWords.find(terminoProcesado) != stopWords.end()){
            posicionActual++; 
            continue; 
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
        cerr << "ERROR: La pregunta no contiene terminos validos";
        return false;
    }

    //guardo toda la informacion 
    pregunta = preg; 
    infPregunta.Inicializar(numPal,numPalSinParada,terminosUnicosPregunta.size());
    return true; 
}


