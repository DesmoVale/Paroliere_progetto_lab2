// Funzione ausiliaria per controllare se uno spostamento sia valido o meno
int is_valid_move(int row, int col, int visited[ROWS][COLS]);

// Funzione che applica l'algoritmo di ricerca a partire da una cella
int search_from(char** board, int row, int col, char* word, int index, int visited[ROWS][COLS]);

// Funzione che applica l'algoritmo di ricerca a partire da ogni cella
int exist(char** board, char* word);

// Funzione per controllare se una parola è già stata usata in precedenza dall'utente
int check_word(char set[BUFFER_SIZE][MAX_LENGTH], char* word);