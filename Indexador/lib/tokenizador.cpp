#include "../include/tokenizador.h"

using namespace std;

  //////////////////////////////////////////////////////
 //     CONSTRUCTORES, DESTRUCTORES Y OPERADORES     //
//////////////////////////////////////////////////////

/**
 * @brief Constructor parametrizado.
 */
Tokenizador::Tokenizador(const string& delimitadoresPalabra, const bool& kcasosEspeciales, const bool& minuscSinAcentos) {
    delimiters.clear();
    DelimitadoresPalabra(delimitadoresPalabra);
    casosEspeciales      = kcasosEspeciales;
    pasarAminuscSinAcentos = minuscSinAcentos;
}

/**
 * @brief Constructor copia.
 */
Tokenizador::Tokenizador(const Tokenizador& other) {
    delimiters           = other.delimiters;
    casosEspeciales      = other.casosEspeciales;
    pasarAminuscSinAcentos = other.pasarAminuscSinAcentos;
}

/**
 * @brief Constructor por defecto.
 *        Los caracteres \xa1 (�) y \xbf (�) se escriben como
 *        hexadecimales para garantizar que son 1 byte en iso
 */
Tokenizador::Tokenizador() {
    delimiters           = ",;:.-/+*\\ '\"{}[]()<>\xa1!\xbf?&#=\t@";
    casosEspeciales      = true;
    pasarAminuscSinAcentos = false;
}

/**
 * @brief Destructor. Vacia los delimitadores.
 */
Tokenizador::~Tokenizador() {
    delimiters = "";
}

/**
 * @brief Operador de asignacion.
 */
Tokenizador& Tokenizador::operator=(const Tokenizador& tkz) {
    if (this != &tkz) {
        delimiters           = tkz.delimiters;
        casosEspeciales      = tkz.casosEspeciales;
        pasarAminuscSinAcentos = tkz.pasarAminuscSinAcentos;
    }
    return *this;
}

/**
 * @brief Operador de salida.
 */
ostream& operator<<(ostream& os, const Tokenizador& tkz) {
    os << "DELIMITADORES: "            << tkz.delimiters
       << " TRATA CASOS ESPECIALES: "  << tkz.casosEspeciales
       << " PASAR A MINUSCULAS Y SIN ACENTOS: " << tkz.pasarAminuscSinAcentos;
    return os;
}


  /////////////////////////////////////////
 //          GETTERS Y SETTERS          //
/////////////////////////////////////////

/**
 * @brief Setter de delimiters junto a AnyadirDelimitadoresPalabra.
 *         limpia delimiters y anyade todod lod nuevos. 
 */
void Tokenizador::DelimitadoresPalabra(const string& nuevoDelimiters) {
    delimiters.clear();
    AnyadirDelimitadoresPalabra(nuevoDelimiters);
}
void Tokenizador::AnyadirDelimitadoresPalabra(const string& nuevoDelimiters) {
    for (int j = 0; j < (int)nuevoDelimiters.length(); j++) {
        char c = nuevoDelimiters[j];
        if (delimiters.find(c) == string::npos) {
            delimiters.push_back(c);
        }
    }
}
/**
 * @brief getter delimiters
 */
string Tokenizador::DelimitadoresPalabra() const {
    return delimiters;
}
/**
 * @brief setter casosEspeciales
 */
void Tokenizador::CasosEspeciales(const bool& nuevoCasosEspeciales) {
    casosEspeciales = nuevoCasosEspeciales;
}
/**
 * @brief getter casosespeciales
 */
bool Tokenizador::CasosEspeciales() {
    return casosEspeciales;
}
/**
 * @brief setter pasarAminuscSinAcentos
 */
void Tokenizador::PasarAminuscSinAcentos(const bool& nuevoPasarAminuscSinAcentos) {
    pasarAminuscSinAcentos = nuevoPasarAminuscSinAcentos;
}
/**
 * @brief getter pasarAminuscSinAcentos
 */
bool Tokenizador::PasarAminuscSinAcentos() {
    return pasarAminuscSinAcentos;
}

  /////////////////////////////////////////////////////////////
 //   FUNCIONES AUXILIARES LIBRES (fuera de la clase)       //
/////////////////////////////////////////////////////////////

/**
 * @brief Devuelve true si 'c' es un delimitador especial de URL.
 *        URLs pueden contener estos caracteres aunque sean delimitadores.
 */
bool EsDelimURL(char c) {
    return (c == '_' || c == ':' || c == '/' || c == '.' ||
            c == '?' || c == '&' || c == '-' || c == '=' ||
            c == '#' || c == '@');
}

/**
 * @brief Devuelve true si 'c' es un delimitador especial de e-mail.
 */
bool EsDelimEmail(char c) {
    return (c == '.' || c == '-' || c == '_');
}

/**
 * @brief Devuelve true si el token acumulado es el inicio de una URL.
 */
bool EsInicioURL(const string& token) {
    return (token == "http" || token == "https" || token == "ftp");
}

/**
 * @brief Devuelve true si el string 's' (token acumulado) es un numero(contiene solo digitos, puntos y comas)
 */
bool EsNumero(const string& s) {
    if (s.empty()) return false;
    for (int j = 0; j < (int)s.length(); j++) {
        if (!isdigit((unsigned char)s[j]) && s[j] != '.' && s[j] != ',')
            return false;
    }
    return true;
}

/**
 * @brief Si no esta vacio guarda el token actual en la lista y lo vacia.
 */
void GuardarToken(string& token, list<string>& tokens) {
    if (!token.empty()) {
        tokens.push_back(token);
        token = "";
    }
}


/**
 * @brief Mira hacia adelante desde la posicion 'desde' para comprobar si
 *        el resto de la palabra (hasta el proximo espacio o
 *        delimitador que no sea '.' o ',') contiene solo digitos, puntos y comas.
 */
bool restoEsNumerico(const string& str, int desde, const string& delimiters) {
    int n = str.length();
    for (int j = desde; j < n; j++) {
        char sig        = str[j];
        bool esEspacio  = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool esDelim    = (delimiters.find(sig) != string::npos);

        if (esEspacio) break;                      // fin de la palabra: salimos
        if (esDelim && sig != '.' && sig != ',') break; // delimitador no-numerico: fin de scope
        if (!isdigit((unsigned char)sig) && sig != '.' && sig != ',')
            return false;                          // letra u otro caracter: NO es numerico
    }
    return true;
}


  /////////////////////////////////////////
 //   FUNCIONES DE CADA CASO ESPECIAL   //
/////////////////////////////////////////

/**
 * @brief Tokeniza una URL completa.
 *  Se activa cuando:
 *    - El delimitador encontrado es un caracter especial de URL (_ : / . ? & - = # @)
 *    - El token acumulado hasta ese momento es exactamente "http", "https" o "ftp"
 * @return true si todo ha salido bien (i ya apunta al siguiente caracter a analizar)
 * @return false no se ha modificado nada
 */
bool Tokenizador::TokenizarURL(const string& str, int& i, string& token, list<string>& tokens) const {
    char c = str[i];
    int  n = str.length();

    // Condicion de activacion
    if (!EsDelimURL(c) || !EsInicioURL(token)) return false;

    // Si no hay nada tras el "http:" no es una URL valida: guardamos token y avanzamos
    if (i + 1 >= n || str[i+1] == ' ' || str[i+1] == '\t' ||
        str[i+1] == '\n' || str[i+1] == '\r') {
        GuardarToken(token, tokens);
        i++;
        return true;
    }

    // Incluimos el delimitador activador (como ':' en "http:") y seguimos
    token += c;
    i++;

    while (i < n) {
        char sig           = str[i];
        bool sigEsEspacio  = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool sigEsDelim    = (delimiters.find(sig) != string::npos);
        bool sigEsDelimURL = EsDelimURL(sig);

        if (sigEsEspacio) {
            break;                        // Fin de URL: espacio
        } else if (sigEsDelim && !sigEsDelimURL) {
            break;                        // Fin de URL: delimitador no-URL
        } else {
            token += sig;                 // Caracter de URL: seguimos acumulando
            i++;
        }
    }

    GuardarToken(token, tokens);
    return true;
}

/**
 * @brief Intenta tokenizar un numero con puntos y/o comas.
 *  Se activa cuando:
 *    - El delimitador encontrado es '.' o ','
 *    - El token acumulado esta vacio O contiene solo digitos/puntos/comas
 *    - El siguiente caracter NO es otro punto/coma (para evitar "1..2")
 * @return true  si ha gestionado este caso
 * @return false si no aplica
 */
bool Tokenizador::TokenizarNumero(const string& str, int& i, string& token, list<string>& tokens) const {
    char c = str[i];
    int  n = str.length();

    // Condicion de activacion
    if (c != '.' && c != ',')              return false;
    if (!EsNumero(token) && !token.empty()) return false;

    // Dos separadores seguidos -> no es numero ("1..2")
    if (i + 1 < n && (str[i+1] == '.' || str[i+1] == ',')) return false;

    // El siguiente debe ser un digito
    if (i + 1 >= n || !isdigit((unsigned char)str[i+1])) return false;

    // LOOKAHEAD: comprobamos que el resto de la palabra (desde i+1) contiene dijitos puntos comas
    if (!restoEsNumerico(str, i + 1, delimiters)) return false;

    // Si el token estaba vacio anyadimos el "0" inicial (0.35)
    if (token.empty()) token = "0";

    token += c;  // Anyadimos el punto/coma al numero
    i++;

    // Seguimos leyendo el resto del numero
    while (i < n) {
        char sig          = str[i];
        bool sigEsEspacio = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool sigEsDelim   = (delimiters.find(sig) != string::npos);

        if (sigEsEspacio) {
            break;  // Fin por espacio

        } else if (sigEsDelim && (sig == '.' || sig == ',')) {
            // Otro punto/coma: lo incluimos solo si le sigue un digito
            if (i + 1 < n && isdigit((unsigned char)str[i+1])) {
                token += sig;
                i++;
            } else {
                break;  // Punto/coma al final del numero: no pertenece
            }

        } else if (sigEsDelim && (sig == '%' || sig == '$') &&
                   i + 1 < n && (str[i+1] == ' ' || str[i+1] == '\t')) {
            // % o $ seguido de espacio: guardamos el numero y el simbolo por separado
            GuardarToken(token, tokens);
            token += sig;
            GuardarToken(token, tokens);
            i++;
            break;

        } else if (sigEsDelim) {
            break;  // Otro delimitador: fin del numero

        } else if (isdigit((unsigned char)sig)) {
            token += sig;
            i++;

        } else {
            break;  // Letra u otro caracter: fin del numero
        }
    }

    GuardarToken(token, tokens);
    return true;
}

/**
 * @brief Intenta tokenizar un e-mail.
 *  Se activa cuando:
 *    - El delimitador encontrado es '@'
 *    - El token acumulado no esta vacio
 *    - No hay otro '@' en la misma palabra
 * @return true  si ha gestionado este caso
 * @return false si no
 */
bool Tokenizador::TokenizarEmail(const string& str, int& i, string& token, list<string>& tokens) const {
    char c = str[i];
    int  n = str.length();

    // Condicion de activacion
    if (c != '@' || token.empty()) return false;

    // Si no hay nada tras '@': no es email valido, guardamos y avanzamos
    if (i + 1 >= n || str[i+1] == ' ' || str[i+1] == '\t' || str[i+1] == '\n' || str[i+1] == '\r') {
        GuardarToken(token, tokens);
        i++;
        return true;
    }

    // Miramos hacia adelante: si hay otro '@' en la misma palabra, no es email
    bool hayOtroArroba = false;
    for (int j = i + 1; j < n; j++) {
        if (str[j] == ' ' || str[j] == '\t' || str[j] == '\n' || str[j] == '\r') break;
        if (str[j] == '@') { hayOtroArroba = true; break; }
        if (delimiters.find(str[j]) != string::npos && !EsDelimEmail(str[j])) break; //string::npos si no encuentra el char
    }

    if (hayOtroArroba) {
        GuardarToken(token, tokens);
        i++;  // Saltamos el '@' que no era de email
        return true;
    }

    // Es email: incluimos el '@' y seguimos leyendo el dominio
    token += c;
    i++;

    while (i < n) {
        char sig             = str[i];
        bool sigEsEspacio    = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool sigEsDelim      = (delimiters.find(sig) != string::npos);
        bool sigEsDelimEmail = EsDelimEmail(sig);

        if (sigEsEspacio) {
            break;

        } else if (sigEsDelim && sigEsDelimEmail) {
            // Punto/guion/guion-bajo: solo los incluimos si van seguidos de no-delimitador
            if (i + 1 < n && delimiters.find(str[i+1]) == string::npos && str[i+1] != ' ') {
                token += sig;
                i++;
            } else {
                break;  // Punto o guion al final: no pertenece al email
            }

        } else if (sigEsDelim) {
            break;  // Otro delimitador: fin del email

        } else {
            token += sig;
            i++;
        }
    }

    GuardarToken(token, tokens);
    return true;
}

/**
 * @brief Intenta tokenizar un acronimo
 *  Se activa cuando:
 *    - El delimitador encontrado es '.'
 *    - El token acumulado no esta vacio
 *    - El siguiente caracter NO es otro punto
 * @return true  si ha gestionado este caso
 * @return false si no 
 */
bool Tokenizador::TokenizarAcronimo(const string& str, int& i, string& token, list<string>& tokens) const {
    char c = str[i];
    int  n = str.length();

    // Condicion de activacion
    if (c != '.' || token.empty()) return false;

    // Dos puntos seguidos: no es acronimo
    if (i + 1 < n && str[i+1] == '.') return false;

    // El siguiente debe ser un caracter no-delimitador (hay algo despues del punto)
    if (i + 1 >= n || delimiters.find(str[i+1]) != string::npos ||
        str[i+1] == ' ' || str[i+1] == '\t' || str[i+1] == '\n' || str[i+1] == '\r') {
        return false;
    }

    // Es acronimo: incluimos el punto y seguimos acumulando
    token += c;
    i++;

    while (i < n) {
        char sig          = str[i];
        bool sigEsEspacio = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool sigEsDelim   = (delimiters.find(sig) != string::npos);

        if (sigEsEspacio) {
            break;

        } else if (sigEsDelim && sig == '.') {
            // Otro punto: lo incluimos solo si no es doble y le sigue algo
            if (i + 1 < n && str[i+1] != '.' &&
                delimiters.find(str[i+1]) == string::npos &&
                str[i+1] != ' ' && str[i+1] != '\t') {
                token += sig;
                i++;
            } else {
                break;  // Dos puntos o punto al final: fin del acronimo
            }

        } else if (sigEsDelim) {
            break;  // Otro delimitador: fin del acronimo

        } else {
            token += sig;
            i++;
        }
    }

    // Quitamos el punto final si lo hay
    if (!token.empty() && token.back() == '.') {
        token.pop_back();
    }

    GuardarToken(token, tokens);
    return true;
}

/**
 * @brief Intenta tokenizar una multipalabra con guion (tipo MS-DOS, UN-DOS-TRES).
 *  Se activa cuando:
 *    - El delimitador encontrado es '-'
 *    - El token acumulado NO esta vacio
 *    - El siguiente caracter es un caracter normal
 * @return true  si ha gestionado este caso
 * @return false si no
 */
bool Tokenizador::TokenizarGuion(const string& str, int& i, string& token, list<string>& tokens) const {
    char c = str[i];
    int  n = str.length();

    // Condicion de activacion: guion con algo antes y algo no-delimitador despues
    if (c != '-' || token.empty()) return false;

    bool sigEsNormal = (i + 1 < n &&
                        delimiters.find(str[i+1]) == string::npos &&
                        str[i+1] != ' '  && str[i+1] != '\t' &&
                        str[i+1] != '\n' && str[i+1] != '\r');

    if (!sigEsNormal) return false;

    // Es multipalabra: incluimos el guion y seguimos
    token += c;
    i++;

    while (i < n) {
        char sig          = str[i];
        bool sigEsEspacio = (sig == ' ' || sig == '\t' || sig == '\n' || sig == '\r');
        bool sigEsDelim   = (delimiters.find(sig) != string::npos);

        if (sigEsEspacio) {
            break;

        } else if (sigEsDelim && sig == '-') {
            // Otro guion: lo incluimos solo si le sigue un caracter normal
            if (i + 1 < n && delimiters.find(str[i+1]) == string::npos && str[i+1] != ' ') {
                token += sig;
                i++;
            } else {
                break;  // Guion doble o guion al final: fin de multipalabra
            }

        } else if (sigEsDelim) {
            break;  // Otro delimitador: fin de multipalabra

        } else {
            token += sig;
            i++;
        }
    }

    GuardarToken(token, tokens);
    return true;
}


  /////////////////////////////////////////
 //       METODO PRINCIPAL: Tokenizar   //
/////////////////////////////////////////

/**
 * @brief Tokeniza una cadena de texto y almacena los tokens en la lista.
 *  1. Se vacia la lista de tokens.
 *  2. Si pasarAminuscSinAcentos es true, se convierte la cadena a
 *     minusculas y sin acentos ANTES de tokenizar.
 *  3. Si casosEspeciales es false -> algoritmo que ha en el enunciado.
 *
 *     SI casosEspeciales = true:
 *      recorre la cadena caracter a caracter con i, va acumulando en token 
 *       cuando hay un delimitador se miran casos especiales, si es se llama metodo.
 */
void Tokenizador::Tokenizar(const string& str, list<string>& tokens) const {

    tokens.clear();

    //copio la cadena por si hay que quitarle acentos
    string cadena = str;

    if (pasarAminuscSinAcentos) {
        for (int j = 0; j < (int)cadena.length(); j++) {

            unsigned char c = (unsigned char)cadena[j];  // unsigned para que no los detecte como negativos 

            // Mayusculas ASCII normales (A-Z): las pasamos a minuscula
            if (c >= 'A' && c <= 'Z') {
                cadena[j] = (char)tolower(c);
            }
            // Mayusculas acentuadas y especiales ISO-8859-1
            else if (c == 0xC0 || c == 0xC1 || c == 0xC2 || c == 0xC3 || c == 0xC4 || c == 0xC5) cadena[j] = 'a'; 
            else if (c == 0xC8 || c == 0xC9 || c == 0xCA || c == 0xCB)                             cadena[j] = 'e'; 
            else if (c == 0xCC || c == 0xCD || c == 0xCE || c == 0xCF)                             cadena[j] = 'i'; 
            else if (c == 0xD2 || c == 0xD3 || c == 0xD4 || c == 0xD5 || c == 0xD6)               cadena[j] = 'o'; 
            else if (c == 0xD9 || c == 0xDA || c == 0xDB || c == 0xDC)                             cadena[j] = 'u';
            else if (c == 0xD1)                                                                     cadena[j] = (char)0xF1; 
            // Minusculas acentuadas ISO-8859-1 (solo vocales: quitar la tilde)
            else if (c == 0xE0 || c == 0xE1 || c == 0xE2 || c == 0xE3 || c == 0xE4 || c == 0xE5) cadena[j] = 'a'; 
            else if (c == 0xE8 || c == 0xE9 || c == 0xEA || c == 0xEB)                             cadena[j] = 'e'; 
            else if (c == 0xEC || c == 0xED || c == 0xEE || c == 0xEF)                             cadena[j] = 'i'; 
            else if (c == 0xF2 || c == 0xF3 || c == 0xF4 || c == 0xF5 || c == 0xF6)               cadena[j] = 'o'; 
            else if (c == 0xF9 || c == 0xFA || c == 0xFB || c == 0xFC)                             cadena[j] = 'u'; 
        }
    }

    //casos especiales false: uso algoritmo de clase. 
    if (!casosEspeciales) {
        string delimitersConSalto = delimiters + "\n\r";
        string::size_type lastPos = cadena.find_first_not_of(delimitersConSalto, 0);
        string::size_type pos     = cadena.find_first_of(delimitersConSalto, lastPos);

        while (string::npos != pos || string::npos != lastPos) {
            tokens.push_back(cadena.substr(lastPos, pos - lastPos));
            lastPos = cadena.find_first_not_of(delimitersConSalto, pos);
            pos     = cadena.find_first_of(delimitersConSalto, lastPos);
        }

    //Casosespeciales == true
    // Recorremos la cadena caracter a caracter con el indice 'i'.
    // 'token' acumula el texto del token actual.
    // Cuando encontramos un delimitador o espacio, se mira cada caso especial
    } else {

        string token = "";
        int i = 0;
        int n = cadena.length();

        while (i < n) {
            char c         = cadena[i];
            bool esDelim   = (delimiters.find(c) != string::npos);
            bool esEspacio = (c == ' ' || c == '\t' || c == '\n' || c == '\r');

            if (!esDelim && !esEspacio) {
                // Caracter normal
                token += c;
                i++;

            } else {
                // Delimitador o espacio-> comprobamos casos especiales en orden
                if      (esDelim && TokenizarURL     (cadena, i, token, tokens)) { /* se gestiona dentro del metodo*/ }
                else if (esDelim && TokenizarNumero  (cadena, i, token, tokens)) {}
                else if (esDelim && TokenizarEmail   (cadena, i, token, tokens)) {}
                else if (esDelim && TokenizarAcronimo(cadena, i, token, tokens)) {}
                else if (esDelim && TokenizarGuion   (cadena, i, token, tokens)) {}
                else {
                    // Ningun caso especial: delimitador normal
                    GuardarToken(token, tokens);
                    i++;
                }
            }
        }

        // Si al terminar la cadena queda algun token sin guardar, lo guardamos
        GuardarToken(token, tokens);
    }
}


  ////////////////////////////////////////////////////
 //   METODOS QUE TOKENIZAN FICHEROS Y DIRECTORIOS //
////////////////////////////////////////////////////

/**
 * @brief Tokeniza el fichero 'i' y guarda los tokens en el fichero 'f'.
 *        Cada token ocupa una linea en el fichero de salida.
 */
bool Tokenizador::Tokenizar(const string& i, const string& f) const {
    ifstream fichEntrada;
    ofstream fichSalida;
    string cadena;
    list<string> tokens;

    fichEntrada.open(i.c_str());
    if (!fichEntrada) {
        cerr << "ERROR: No existe el archivo: " << i << endl;
        return false;
    }

    while (!fichEntrada.eof()) {
        cadena = "";
        getline(fichEntrada, cadena);
        if (cadena.length() != 0) {
            Tokenizar(cadena, tokens);
        }
    }
    fichEntrada.close();

    fichSalida.open(f.c_str());
    //Recorro todos los tokens
    for (list<string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        fichSalida << (*it) << endl;
    }
    fichSalida.close();

    return true;
}

/**
 * @brief Tokeniza el fichero 'i' y guarda el resultado en 'i.tk'.
 *        Simplemente llama a la version anterior con la extension a�adido.
 */
bool Tokenizador::Tokenizar(const string& i) const {
    return Tokenizar(i, i + ".tk");
}

/**
 * @brief Tokeniza todos los ficheros cuyo nombre aparece en el fichero 'i'.
 *        Lee el fichero linea a linea y tokeniza cada fichero.
 */
bool Tokenizador::TokenizarListaFicheros(const string& i) const {
    struct stat comprobacion;

    // Comprobamos que 'i' existe y no es un directorio
    if (stat(i.c_str(), &comprobacion) == -1 || S_ISDIR(comprobacion.st_mode)) {
        cerr << "ERROR: No existe el archivo: " << i << endl;
        return false;
    }

    ifstream lista(i.c_str());
    string nombre_fichero;
    bool todo_ok = true;

    while (getline(lista, nombre_fichero)) {
        if (nombre_fichero.empty()) continue;

        struct stat comp2;
        if (stat(nombre_fichero.c_str(), &comp2) == -1 || S_ISDIR(comp2.st_mode)) {
            cerr << "ERROR: No existe el archivo: " << nombre_fichero << endl;
            todo_ok = false;
            continue;  // Seguimos con el siguiente
        }

        if (!Tokenizar(nombre_fichero)) {
            todo_ok = false;
        }
    }

    lista.close();
    return todo_ok;
}

/**
 * @brief Tokeniza todos los ficheros dentro del directorio 'i'
 */
bool Tokenizador::TokenizarDirectorio(const string& i) const {
    struct stat dir;
    //i.c_strr() numbre del archivo, &dir es el directorio, si devuelve -1 es que no existe en ese directorio. 
    //S_ISDIR te dice si es directorio, como quiero que sea fichero pues !
    if (stat(i.c_str(), &dir) == -1 || !S_ISDIR(dir.st_mode)) {
        cerr << "ERROR: No existe el directorio: " << i << endl;
        return false;
    }

    // Generamos lista de ficheros con 'find', excluyendo los .tk ya creados
    string cmd = "find " + i + " -follow | grep -v '\\.tk$' | sort > .lista_fich";
    system(cmd.c_str());

    return TokenizarListaFicheros(".lista_fich");
}