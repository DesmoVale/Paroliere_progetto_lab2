#include "strutture.h"
#include "format_parole.h"


/***FUNZIONI PER FORMATTARE LE PAROLE PER CONTROLLI***/
// Rendo la parola tutta maiuscola
void to_upper_case(char *str) {
    while (*str) {
        // Controlla se il carattere è una lettera minuscola
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - ('a' - 'A');
        }
        str++;
    }
}
// Rimuovo la u per il corretto controllo della parola nella matrice
char* remove_u(char *str) {
    int length = strlen(str);
    char* res = (char*)malloc((length) * sizeof(char));
    int j = 0; // Indice per la nuova stringa res
    for(int i = 0; i < length; i++) {
        if(str[i] == 'Q' && (str[i + 1] == 'U' || str[i + 1] == 'u')) {
            res[j] = 'Q';
            j++;
            i++; // Salta la 'U'
        } else {
            res[j] = str[i];
            j++;
        }
    }

    return res;
}

void to_lower_case(char* str) {
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + 32;
        }
        str++;
    }
}

// Funzione che verifica se un carattere è uno spazio
int my_isspace(char c) {
    return (c == ' ');
}

// Funzione che verifica se un carattere è alfanumerico (lettera o numero)
int my_isalnum(char c) {

    if (c >= 'A' && c <= 'Z') {
        return 1;
    }

    if (c >= 'a' && c <= 'z') {
        return 1;
    }

    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

// Funzione principale per verificare le condizioni della stringa
int verifica_username(char *str) {
    int len = strlen(str);
    
    
    if (len == 0) {
        return 0;
    }

    if (len > 10) {
        return 0;
    }
    
    for (int i = 0; i < len; i++) {
        if (my_isspace(str[i]) || !my_isalnum(str[i])) {
            return 0;
        }
    }
    
    return 1;
}