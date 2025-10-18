#include "strutture.h"
#include "controllo_parole_matrix.h"


// Funzione ausiliaria per controllare se uno spostamento sia valido o meno
int is_valid_move(int row, int col, int visited[ROWS][COLS]) {
    return row >= 0 && row < ROWS && col >= 0 && col < COLS && !visited[row][col];
}

// Algoritmo di ricerca della parola all'interno della matrice
int search_from(char** board, int row, int col, char* word, int index, int visited[ROWS][COLS]) {

    if (index == strlen(word)) {
        return 1;
    }

    if (!is_valid_move(row, col, visited)) {
        return 0;
    }

    if (board[row][col] != word[index]) {
        return 0;
    }

    // Cella visitata
    visited[row][col] = 1;

    // Esploro tutte le possibili direzioni, includendo ora anche le diagonali
    if (search_from(board, row - 1, col, word, index + 1, visited) || // sopra
        search_from(board, row + 1, col, word, index + 1, visited) || // sotto
        search_from(board, row, col - 1, word, index + 1, visited) || // sinistra
        search_from(board, row, col + 1, word, index + 1, visited) || // destra
        search_from(board, row - 1, col - 1, word, index + 1, visited) || // diagonale alto-sinistra
        search_from(board, row - 1, col + 1, word, index + 1, visited) || // diagonale alto-destra
        search_from(board, row + 1, col - 1, word, index + 1, visited) || // diagonale basso-sinistra
        search_from(board, row + 1, col + 1, word, index + 1, visited)) { // diagonale basso-destra
        return 1;
    }

    // Backtracking: libero la cella visitata
    visited[row][col] = 0;
    return 0;
}

// Applico l'algoritmo di ricerca a partire da ogni cella
int exist(char** board, char* word) {

    int visited[ROWS][COLS];

    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            visited[i][j] = 0;
        }
    }

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (search_from(board, row, col, word, 0, visited)) {
                return 1;
            }
        }
    }

    return 0;
}


// Funzione per controllare se una parola è già stata usata in precedenza dall'utente
int check_word(char set[BUFFER_SIZE][MAX_LENGTH], char* word) {

    for(int i = 0; i < BUFFER_SIZE; i++) {
        if(set[i] != NULL && strcmp(set[i], word) == 0) {
            return 1;
        }
    }
    return 0;

}
