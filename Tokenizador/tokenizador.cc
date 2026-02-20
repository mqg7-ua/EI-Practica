#include <iostream>
#include "tokenizador.h"

using namespace std; 

  //////////////////////////////////////////////////////
 //     CONSTRUCTORES Y DESTRUCTORES Y OPERADORES    //
//////////////////////////////////////////////////////
/**
*@brief Constructor parametrizado, inicializa delimiters con delimitadores palabra sin repetir caracteres. 
*@param delimitadoresPalabra String de delimitadores
*@param kcasosEspeciales Flag : si true detectar casos especiales
*@param minuscSinAcentos Flag : si true pasar todo a minusculas y sin hacentos. 
*/
Tokenizador::Tokenizador (const string& delimitadoresPalabra, const& boolkcasosEspeciales, const bool& minuscSinAcentos){
    delimiters.clear();
    for (char c : delimitadoresPalabra){
        if(delimiters.find(c) = std::string::npos){
            delimiters.push_back(c);
        }
    }
    casosEspeciales = kcasosEspeciales; 
    pasarAminuscSinAcentos = minuscSinAcentos;
}

/**
*@brief Constructor copia
*@param Tokenizador 
*/
Tokenizador::Tokenizador (const Tokenizador &other){
    this->casosEspeciales = other.casosEspeciales;
    this->minuscSinAcentos = other.minuscSinAcentos;
    this->delimiters = other.delimiters
}

/**
*@brief Constructor por defecto 
*/
Tokenizador::Tokenizador (){
    delimiters = ";:.-/+*\\ '\"{}[]()<>¡!¿?&#=\t@";
    casosEspeciales = true; 
    pasarAminuscSinAcentos = false; 
}
/**
* @brief Destructor
*/
Tokenizador::~Tokenizador (){
    delimiters = "";
}


  /////////////////////////////////////////
 //     FUNCIONES DE TOKENIZACION       //
/////////////////////////////////////////
Tokenizador& Tokenizador::operator= (const Tokenizador &tkz){
    if(this!= &tkz){
        delimiters = tkz.delimiters; 
        casosEspeciales = tkz.casosEspeciales; 
        pasarAminuscSinAcentos = tkz.pasarAminuscSinAcentos;
    }
    return *this;   
}

/**
*@brief Tokeniza str devolviendo el resultado en tokens. La lista tokens se
*vaciará antes de almacenar el resultado de la tokenización.
*@param str: string a tokenizar
*@param tokens: elemento a devolver
*/
void Tokenizador::Tokenizar (const string& str, list<string>& tokens) const{

    string::size_type lastPos = str.find_first_not_of(delimiters,0);
    string::size_type pos = str.find_first_of(delimiters,lastPos);

    while(string::npos != pos || string::npos != lastPos){
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

//Optimizar eficiencia //////////////////////////////////////////////////////////7
/**
/@brief // Tokeniza el fichero i guardando la salida en el fichero f (una
        palabra en cada línea del fichero). Devolverá true si se realiza la
        tokenización de forma correcta; false en caso contrario enviando a cerr
        el mensaje correspondiente (p.ej. que no exista el archivo i)
/@param i:fichero entrada
/@param f:fichero salida
/@return true: tokenizacion correcta
/@return false: tokenizacion incorrecta 
*/
bool Tokenizador::Tokenizar (const string& i, const string& f) const{
    ifstream i;
    ofstream f;
    string cadena;
    list<string> tokens;
    i.open(NomFichEntr.c_str());

    if(!i) {
        cerr << "ERROR: No existe el archivo: " << NomFichEntr << endl;
        return false;

    }else{
        while(!i.eof()){
            cadena="";
            getline(i, cadena);
            if(cadena.length()!=0){
                Tokenizar(cadena, tokens);
            }
        }
    }

    i.close();
    f.open(NomFichSal.c_str());
    list<string>::iterator itS;

    for(itS= tokens.begin();itS!= tokens.end();itS++){
        f << (*itS) << endl;
    }

    f.close();
    return true;
}

/**
*@brief: // Tokeniza el fichero i guardando la salida en un fichero de nombre i
            añadiéndole extensión .tk (sin eliminar previamente la extensión de i
            por ejemplo, del archivo pp.txt se generaría el resultado en pp.txt.tk),
            y que contendrá una palabra en cada línea del fichero. Devolverá true si
            se realiza la tokenización de forma correcta; false en caso contrario enviando a cerr el mensaje correspondiente (p.ej. que no exista el
            archivo i)
*@param i: 
*@return true: 
*@return false: 
*/
bool Tokenizador::Tokenizar (const string & i) const{

}

//La eficiencia es importante en esta (y lo que cuelgue de aqui)
bool Tokenizador::TokenizarListaFicheros (const string& i) const{

}

bool Tokenizador::TokenizarDirectorio (const string& i) const{
    struct stat dir;
    // Compruebo la existencia del directorio
    int err=stat(dirAIndexar.c_str(), &dir);
    
    if(err==-1 || !S_ISDIR(dir.st_mode))
        return false;
    else {
        // Hago una lista en un fichero con find>fich
        string cmd="find "+dirAIndexar+" -follow |sort > .lista_fich";
        system(cmd.c_str());
        return TokenizarListaFicheros(".lista_fich");
    }
}


  /////////////////////////////////////////
 //             DELIMITADORES           //
/////////////////////////////////////////

void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters){

}

void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters){

}

string Tokenizador::DelimitadoresPalabra() const{

}


  /////////////////////////////////////////
 //          CASOS ESPECIALES           //
/////////////////////////////////////////

void Tokenizador::CasosEspeciales (const bool& nuevoCasosEspeciales){

}

bool Tokenizador::CasosEspeciales (){

}


  ///////////////////////////
 //     MIN Y MAYUS       //
///////////////////////////
void Tokenizador::PasarAminuscSinAcentos (const bool& nuevoPasarAminuscSinAcentos){

}

bool Tokenizador::PasarAminuscSinAcentos (){

}


  /////////////////////////////////////////
 //            OP SALIDA                //
/////////////////////////////////////////
ostream& operator<<(ostream&, const Tokenizador&){



}