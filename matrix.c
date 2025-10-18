#include "strutture.h"
#include "matrix.h"
#define LINE_LENGTH 16

/***FUNZIONI DI VERIFICA ESISTENZA PAROLA***/

// Funzione che genera la matrice casualmente
void randomMatrixGenerator(char** matrix, FILE* file, int seed) {
    int fine = 0;
    if(seed) {
        srand(seed);
    } else {
        srand(time(NULL));
    }

    if(file != NULL) {

        char buffer[LINE_LENGTH * 2 + 2];  // +2 per includere terminatore e nuova linea

        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            rewind(file);
            fgets(buffer, sizeof(buffer), file);
        }

        // Rimuovo nuova linea
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        char* line = malloc(LINE_LENGTH);

        int j = 0;
        for(int i = 0; i < LINE_LENGTH; i++) {
            if(buffer[j] == 'Q') {
                line[i] = buffer[j];
                j = j + 3;
            } else {
                line[i] = buffer[j];
                j = j + 2;
            }
        }

        int k = 0;

        for(int i = 0; i < ROWS; i++) {
            for(int j = 0; j < COLS; j++) {
                matrix[i][j] = line[k];
                k++;
            }
        }
    
    } else {
        for(int i = 0; i < ROWS; i++) {
            for(int j = 0; j < COLS; j++) {
                matrix[i][j] = alphabet[rand() % 26];
            }
        }
    }

}

// Funzione per stampare la matrice nel client
int print_matrix(char* matrix) {

    int printed = 0;
    
    printf("\n");
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            if(matrix[i * 4 + j] == 'Q') {
                printed = printed + printf("Qu ");
            } else {
                printed = printed + printf("%c  ", matrix[i * 4 + j]);
            }
        }
        printf("\n");
    }

    if(printed > 0) {
        return 1;
    } else {
        return 0;
    }

}