#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/types.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <getopt.h>

#include "strutture.h"
#include "format_parole.h"
#include "serializzazione_messaggi.h"
#include "classifica_finale.h"
#include "controllo_parole_matrix.h"
#include "matrix.h"
#include "macros.h"

#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'
#define MSG_QUIT 'Q'
#define MSG_PUNTI_PARZIALI 'C'
#define MSG_SERVER_SHUTDOWN 'B'
#define MSG_POST_BACHECA 'H'
#define MSG_SHOW_BACHECA 'S'

/*Bacheca*/
char bacheca[MAX_MSG][MAX_SIZE_MSG];
int tail = 0;
int count = 0;
pthread_mutex_t mutex_bacheca = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_control = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t client_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_ready_cond = PTHREAD_COND_INITIALIZER;
int clients_ready_for_winner_update = 0;

/*Aggiungere messaggio*/
void add_message(char bacheca[MAX_MSG][MAX_SIZE_MSG], char *message) {

    pthread_mutex_lock(&mutex_bacheca);

    int len = strlen(message);
    
    memset(bacheca[tail], 0, MAX_SIZE_MSG);
    strcpy(bacheca[tail], message);
    tail = (tail + 1) % MAX_MSG;
    count++;

    pthread_mutex_unlock(&mutex_bacheca);

}

/*Leggere bacheca*/
char* read_message(char bacheca[MAX_MSG][MAX_SIZE_MSG]) {

    char* csv = (char*)malloc(MAX_MSG * MAX_SIZE_MSG * sizeof(char));
    strcpy(csv, "\nBACHECA MESSAGGI\n");

    pthread_mutex_lock(&mutex_bacheca);

    if(count == 0) {
        strcat(csv, "Nessun messaggio in bacheca\n");
    } else {
        for(int i = 0; i < count; i++) {
            char msg[MAX_SIZE_MSG];
            sprintf(msg, "%s,\n", bacheca[i]);
            strcat(csv, msg);
        }
    }

    pthread_mutex_unlock(&mutex_bacheca);

    return csv;

}

/*******VARIABILI GLOBALI*******/
char** matrix;
Player* players;
TrieNode* root;

// Gestione delle opt riga di comando
int opt;
int rand_seed = 0;
char* dizionario_filename = NULL;
char dizionario_default[] = "dictionary_ita.txdizionario_fit";
char* matrix_filename = NULL;
FILE* file = NULL;
int durata_minuti = 0;
// Gestione dei client
pthread_t* ids;
int* client_fds;
int* client_slots;
int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_slots_mutex = PTHREAD_MUTEX_INITIALIZER;

// Gestione delle sessioni di gioco
typedef enum { ATTESA, PARTITA, START } Stato;
Stato stato = START;
int winner_updated = 0;
char* CSV_out;
int DURATA_ATTESA = 60;  // in secondi
int DURATA_PARTITA = 180; // in secondi
time_t inizio_attesa;
time_t inizio_partita;
pthread_mutex_t stato_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t winner_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;


/*******CONTROLLO SE UTENTE GIA' REGISTRATO*******/
int check_player(Player* players, char* name) {

    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(strcmp(players[i].name, name) == 0) {
            return 1;
        }
    }

    return 0;
}

/*******FUNZIONE DEL THREAD SCORER*******/
void* scorer(void* args) {
    DataScorer* scorerData = (DataScorer*) args;
    Message msg;
    msg.data = (char*)malloc(BUFFER_SIZE * sizeof(char));
    int max = -1;
    int msg_size;
    msg.type = MSG_PUNTI_FINALI;
    char serialized_msg[BUFFER_SIZE];

    // Trovare il vincitore
    pthread_mutex_lock(&players_mutex);
    pthread_mutex_lock(&client_slots_mutex);
    generateCSV(scorerData -> players, MAX_CLIENTS, CSV_out, client_slots);
    pthread_mutex_unlock(&client_slots_mutex);
    // Azzero i punteggi
    for(int i = 0; i < MAX_CLIENTS; i++) {
        scorerData -> players[i].score = 0;
    }
    pthread_mutex_unlock(&players_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        for(int j = 0; j < BUFFER_SIZE; j++) {
            strcpy(players[i].set[j], "\0");
        }
    }
    pthread_mutex_lock(&winner_mutex);
    winner_updated = 1;
    pthread_cond_broadcast(&cond1);
    pthread_mutex_unlock(&winner_mutex);

    return NULL;
}

/******INVIO ASINCRONO DEL MESSAGGIO********/
volatile sig_atomic_t control = 0;


void async_signal_handler(int signal) {
    if (signal == SIGUSR1) {
        pthread_mutex_lock(&mutex_control);
        control = 1;
        pthread_mutex_unlock(&mutex_control);
    }
}
void* async(void* args) {
    DataHandler* threadData = (DataHandler*)args;
    char serialized_msg[BUFFER_SIZE];
    int msg_size;
    Message msg;
    msg.data = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    ssize_t n_written;


    signal(SIGUSR1, async_signal_handler);

    while (1) {

        if (control == 1) {
            pthread_mutex_lock(&mutex_control);
            control = 0;
            pthread_mutex_unlock(&mutex_control);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&winner_mutex);
        pthread_cond_wait(&cond1, &winner_mutex);
        printf("%d:fd\n", client_fds[threadData->index]);
        if (client_fds[threadData->index] == -1) {
            pthread_mutex_unlock(&winner_mutex);
            break;
        }
        
        msg.type = MSG_PUNTI_FINALI;
        strcpy(msg.data, CSV_out);
        msg.length = strlen(msg.data);
        msg_size = sizeof(int) + sizeof(char) + msg.length;
        serialize_message(&msg, serialized_msg);
        SYSC(n_written, write(client_fds[threadData->index], serialized_msg, msg_size), "nella write");
        pthread_mutex_unlock(&winner_mutex);
        
    }

    pthread_exit(NULL);
}

/*******FUNZIONE THREAD GESTIONE CLIENT*******/
void* handler(void* args) {
    
    ssize_t n_written;
    ssize_t n_read;
    int retvalue;

    DataHandler* threadData = (DataHandler*)args;
    int client_fd = threadData -> client_fd;

    Message msg;
    msg.data = (char*)malloc(BUFFER_SIZE * sizeof(char));
    char buffer[BUFFER_SIZE];
    char serialized_msg[BUFFER_SIZE];
    int msg_size;
    int counter = 0;
    int q = 0;

    int is_registered = 0;

    pthread_t async_t;
    SYSC(retvalue, pthread_create(&async_t, NULL, async, threadData), "nella pthread_create");

    Stato stato_corrente;

    while (!q) {        

        memset(buffer, 0, BUFFER_SIZE);
        memset(msg.data, 0, BUFFER_SIZE);

        read(client_fd, buffer, BUFFER_SIZE);
        deserialize_message(buffer, &msg);

        switch (msg.type) {
            case MSG_REGISTRA_UTENTE:
                if(verifica_username(msg.data)) {
                    if (check_player(threadData -> players, msg.data)) {
                        // Utente già in uso rispondo con err
                        memset(msg.data, 0, msg.length);
                        strcpy(msg.data, "Username già in uso");
                        msg.length = strlen(msg.data);
                        msg.type = MSG_ERR;
                        msg_size = sizeof(int) + sizeof(char) + msg.length;
                        serialize_message(&msg, serialized_msg);
                        SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write utente già in uso");
                    } else {
                        is_registered = 1;
                        // Registro utenete e scrivo msg ok
                        pthread_mutex_lock(&players_mutex);
                        msg.type = MSG_OK;
                        strcpy(threadData -> players[threadData -> index].name, msg.data);
                        memset(msg.data, 0, msg.length);
                        strcpy(msg.data, "Registrazione avvenuta con successo");
                        msg.length = strlen(msg.data);
                        msg_size = sizeof(int) + sizeof(char) + msg.length;
                        serialize_message(&msg, serialized_msg);
                        SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write registrazione");
                        pthread_mutex_unlock(&players_mutex);
                    }
                } else {
                    memset(msg.data, 0, msg.length);
                    strcpy(msg.data, "Username non valido");
                    msg.length = strlen(msg.data);
                    msg.type = MSG_ERR;
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write utente non valido");
                }
                break;
            case MSG_MATRICE:
                // Invio la matrice
                if(!is_registered) {
                    memset(msg.data, 0, msg.length);
                    sprintf(msg.data, "Utente non registrato, utilizzare registra_utente <nome>");
                    msg.type = MSG_ERR;
                    msg.length = strlen(msg.data);
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio matrice (utente non registrato)");
                    break;
                }
                pthread_mutex_lock(&stato_mutex);
                stato_corrente = stato;
                pthread_mutex_unlock(&stato_mutex);
                if (stato_corrente != PARTITA) {
                    pthread_mutex_lock(&alarm_mutex);
                    int tempo_rimanente = DURATA_ATTESA - (time(NULL) - inizio_attesa);
                    pthread_mutex_unlock(&alarm_mutex);
                    memset(msg.data, 0, msg.length);
                    sprintf(msg.data, "%d", tempo_rimanente);
                    msg.length = strlen(msg.data) + 1;
                    msg.type = MSG_TEMPO_ATTESA;
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio matrice (tempo attesa)");
                    break;
                }
                int h = 0;
                for (int i = 0; i < ROWS; i++) {
                    for (int j = 0; j < COLS; j++) {
                        msg.data[h] = threadData -> matrix[i][j];
                        h++;
                    }
                }
                msg.length = MAX_LENGTH;
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_msg);
                SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio effettivo della matrice");
                // Ricevo msg ok se la matrice è arrivata
                memset(buffer, 0, BUFFER_SIZE);
                memset(msg.data, 0, msg.length);
                SYSC(n_read, read(client_fd, buffer, BUFFER_SIZE), "nella read risposta invio matrice");
                deserialize_message(buffer, &msg);
                if(msg.type == MSG_OK) {
                    int tempo_rimanente;
                    pthread_mutex_lock(&alarm_mutex);
                    if(durata_minuti) {
                        tempo_rimanente = (durata_minuti * 60) - (time(NULL) - inizio_partita);
                    } else {
                        tempo_rimanente = DURATA_PARTITA - (time(NULL) - inizio_partita);
                    }
                    pthread_mutex_unlock(&alarm_mutex);
                    memset(msg.data, 0, msg.length);
                    sprintf(msg.data, "%d", tempo_rimanente);
                    msg.length = strlen(msg.data) + 1;
                    msg.type = MSG_TEMPO_PARTITA;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write tempo partita");  
                } else {
                    memset(msg.data, 0, msg.length);
                    strcpy(msg.data, "La matrice non è stata correttamente inviata");
                    msg.length = strlen(msg.data);
                    msg.type = MSG_ERR;
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write errore invio matrice");
                }              
                break;
            case MSG_PAROLA:
                char* lower_word = (char*)malloc(msg.length * sizeof(char));
                strcpy(lower_word, msg.data);
                to_lower_case(lower_word);
                to_upper_case(msg.data);
                char* word = remove_u(msg.data);
                if(!is_registered) {
                    memset(msg.data, 0, msg.length);
                    sprintf(msg.data, "Utente non registrato, utilizzare registra_utente <nome>");
                    msg.type = MSG_ERR;
                    msg.length = strlen(msg.data);
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write parola utente non registrato");
                    break;
                }
                pthread_mutex_lock(&stato_mutex);
                stato_corrente = stato;
                pthread_mutex_unlock(&stato_mutex);
                if (stato_corrente != PARTITA) {
                    // Rispondo con errore se siamo in attesa
                    memset(msg.data, 0, msg.length);
                    strcpy(msg.data, "Non è possibile inviare parole durante l'attesa di una nuova partita");
                    msg.length = strlen(msg.data);
                    msg.type = MSG_ERR;
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write impossibile inviare parole in game");
                    break;
                }

                if (check_word(threadData -> players[threadData -> index].set, word)) {
                    // Parola già usata
                    memset(msg.data, 0, msg.length);
                    strcpy(msg.data, "0\0");
                    msg.length = 2;
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio parola già usata");
                } else {
                    if (exist(threadData -> matrix, word) == 1 && search(threadData -> root, lower_word) == 1) {
                        // Parola esistente, aggiorno punteggio e aggiungo a parole usate
                        strcpy(threadData -> players[threadData -> index].set[counter], word);
                        counter = counter + 1;
                        threadData -> players[threadData -> index].score += strlen(word);
                        ssize_t punti = strlen(word);
                        if(punti < 4) {
                            punti = 0;
                        }
                        memset(msg.data, 0, msg.length);
                        sprintf(msg.data, "%ld", punti);
                        msg.length = strlen(word) + 1;
                        msg_size = sizeof(int) + sizeof(char) + msg.length;
                        serialize_message(&msg, serialized_msg);
                        SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio punti parola");
                    } else {
                        memset(msg.data, 0, msg.length);
                        strcpy(msg.data, "Parola inesistente\0");
                        msg.length = strlen(msg.data);
                        msg.type = MSG_ERR;
                        msg_size = sizeof(int) + sizeof(char) + msg.length;
                        serialize_message(&msg, serialized_msg);
                        SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio parola inesistente");
                    }
                }
                break;
            case MSG_QUIT:
                memset(msg.data, 0, strlen(msg.data));
                strcpy(msg.data, "Arrivederci");
                msg.length = strlen(msg.data);
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_msg);
                SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write invio quit");
                pthread_mutex_lock(&players_mutex);
                threadData -> players[threadData -> index].name[0] = '\0';
                threadData -> players[threadData -> index].score = 0;
                pthread_mutex_unlock(&players_mutex);
                pthread_mutex_lock(&client_count_mutex);
                client_fds[threadData -> index] = -1;
                client_count--;
                pthread_mutex_lock(&client_slots_mutex);
                client_slots[threadData -> index] = 0;
                pthread_mutex_unlock(&client_slots_mutex);
                printf("Clients attualmente connessi: %d\n", client_count);
                pthread_mutex_unlock(&client_count_mutex);
                pthread_kill(async_t, SIGUSR1);
                // Wait for async thread to finish
                pthread_join(async_t, NULL);
                SYSC(retvalue, close(client_fd), "nella close handler");
                q = 1;
                break;
            case MSG_POST_BACHECA:
                add_message(bacheca, msg.data);
                memset(msg.data, 0, msg.length);
                msg.type = MSG_OK;
                strcpy(msg.data, "Messaggio inviato correttamente\n");
                msg.length = strlen(msg.data);
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_msg);
                SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write post bacheca");                
                break;
            case MSG_SHOW_BACHECA:
                memset(msg.data, 0, msg.length);
                strcpy(msg.data, read_message(bacheca));
                msg.type = MSG_OK;
                msg.length = strlen(msg.data);
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_msg);
                SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write show bacheca");              
                break;
            default:
                if(!is_registered) {
                    memset(msg.data, 0, msg.length);
                    sprintf(msg.data, "Utente non registrato, utilizzare registra_utente <nome>");
                    msg.type = MSG_ERR;
                    msg.length = strlen(msg.data);
                    msg_size = sizeof(int) + sizeof(char) + msg.length;
                    serialize_message(&msg, serialized_msg);
                    SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write default");
                    break;
                }
                memset(msg.data, 0, msg.length);
                strcpy(msg.data, "Messaggio non riconosciuto");
                msg.length = strlen(msg.data);
                msg.type = MSG_ERR;
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_msg);
                SYSC(n_written, write(client_fd, serialized_msg, msg_size), "nella write messaggio non riconosciuto");
                break;
        }
        

    }

    pthread_exit(NULL);
}

/*******GESTIONE DEI SEGNALI*******/
void handle_sigint(int sig) {

    printf("Chiusura del server...\n");

    Message msg;
    char serialized_msg[BUFFER_SIZE];
    msg.type = MSG_SERVER_SHUTDOWN;
    msg.data = "Server in chiusura";
    msg.length = strlen(msg.data);

    ssize_t n_written;
    int retvalue;

    int msg_size = sizeof(int) + sizeof(char) + msg.length;
    serialize_message(&msg, serialized_msg);

    pthread_mutex_lock(&client_slots_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_slots[i]) {
            SYSC(n_written, write(client_fds[i], serialized_msg, msg_size), "nella write sigint");
            pthread_cancel(ids[i]); 
        }
    }  
    pthread_mutex_unlock(&client_slots_mutex);
    for (int i = 0; i < ROWS; i++) {
        free(matrix[i]);
    }
    free(matrix);
    printf("Memoria occupata dalla matrice deallocata\n");
    freeTrie(root);
    printf("Memoria occupata dal trie deallocata\n");
    if(file != NULL) {
        fclose(file);
        printf("File chiuso\n");
    }
    free(CSV_out);
    free(ids);
    free(client_fds);
    free(client_slots);
    printf("Server chiuso correttamente.\n");

    exit(0);
}

/********GESTORE ALARM***********/
void gestore_alarm(int signum) {

    int retvalue;

    pthread_mutex_lock(&stato_mutex);
    if (stato == ATTESA || stato == START) {
        pthread_mutex_lock(&winner_mutex);
        winner_updated = 0;
        pthread_mutex_unlock(&winner_mutex);
        if(durata_minuti) {
            alarm(durata_minuti * 60);
        } else {
            alarm(DURATA_PARTITA);
        }
        pthread_mutex_lock(&alarm_mutex);
        inizio_partita = time(NULL);
        pthread_mutex_unlock(&alarm_mutex);
        printf("Inizio partita\n");
        // Genera nuova matrice
        stato = PARTITA;
        randomMatrixGenerator(matrix, file, rand_seed);
    } else if (stato == PARTITA) {
        pthread_t thread_scorer;
        DataScorer scorerData;
        scorerData.client_fds = client_fds;
        scorerData.players = players;
        scorerData.client_slots = client_slots;
        SYSC(retvalue, pthread_create(&thread_scorer, NULL, scorer, &scorerData), "nella pthreadcreate dello scorer");
        pthread_detach(thread_scorer);
        alarm(DURATA_ATTESA);
        pthread_mutex_lock(&alarm_mutex);
        inizio_attesa = time(NULL);
        pthread_mutex_unlock(&alarm_mutex);
        printf("Fine partita\n");
        stato = ATTESA;
    }
    pthread_mutex_unlock(&stato_mutex);
}


int main(int argc, char* argv[]) {

    ssize_t n_written;
    int retvalue;

    if(argc < 3) {
        printf("Uso: %s nome_server porta_server [--matrix data_filename] [--durata durata_minuti] [--seed rand_seed] [--diz dizionario_filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    char srv_name[10];


    if(strcmp(argv[1], "localhost") == 0){
        strcpy(srv_name, "127.0.0.1");
    } else {
        strcpy(srv_name, argv[1]);
    }

    // Creazione del socket
    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    // Inizializzazione della struttura server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(srv_name);

    // Binding
    SYSC(retvalue, bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)), "nella bind");

    // Listen
    SYSC(retvalue, listen(server_fd, MAX_CLIENTS), "nella listen");
    client_addr_len = sizeof(client_addr);

    // Opzioni lunghe
    struct option long_options[] = {
        {"matrix", required_argument, 0, 'm'},
        {"durata", required_argument, 0, 'd'},
        {"seed", required_argument, 0, 's'},
        {"diz", required_argument, 0, 'z'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "m:d:s:z:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                matrix_filename = optarg;
                break;
            case 'd':
                durata_minuti = atoi(optarg);
                break;
            case 's':
                rand_seed = atoi(optarg);
                break;
            case 'z':
                dizionario_filename = optarg;
                break;
            default:
                printf("Uso: %s nome_server porta_server [--matrix data_filename] [--durata durata_minuti] [--seed rand_seed] [--diz dizionario_filename]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    CSV_out = (char*)malloc(BUFFER_SIZE * sizeof(char));

    signal(SIGINT, handle_sigint);

    if(matrix_filename != NULL) {
        file = fopen(matrix_filename, "r");
        if(file != NULL) {
            printf("File aperto correttamente\n");
        }
    }

    // Inizializzo struttura dati che conterrà le info dei giocatori
    players = (Player*)malloc(MAX_CLIENTS * sizeof(Player));
    for(int i = 0; i < MAX_CLIENTS; i++) {
        players[i].score = 0;
    }

    // Inizializzo il trie se è stato inserito un dizionario
    root = createNode();
    if(dizionario_filename) {
        initTrie(dizionario_filename, root);
    } else {
        initTrie(dizionario_default, root); // Dizionario di default
    }

    printf("Server in attesa di giocatori...\n");

    ids = (pthread_t*)malloc(MAX_CLIENTS * sizeof(pthread_t));
    client_fds = (int*)malloc(MAX_CLIENTS * sizeof(int));
    client_slots = (int*)malloc(MAX_CLIENTS * sizeof(int));


    if(durata_minuti) {
        alarm(durata_minuti * 60);
    } else {
        alarm(DURATA_ATTESA);
    }
    // Inizializzo gli slot per i giocatori
    for(int i = 0; i < MAX_CLIENTS; i++) {
        client_slots[i] = 0;
    }

    // Impostare il gestore del segnale
    signal(SIGALRM, gestore_alarm);

    pthread_mutex_lock(&alarm_mutex);
    inizio_attesa = time(NULL);
    pthread_mutex_unlock(&alarm_mutex);

    // Impostare il primo allarme per l'attesa
    alarm(DURATA_ATTESA);
    
    matrix = (char**)malloc(ROWS * sizeof(char*));
    for(int i = 0; i < ROWS; i++) {
        matrix[i] = (char*)malloc(4 * sizeof(char));
    }

    for(int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = 0;
    }

    char serialized_message[BUFFER_SIZE];
    int msg_size;
    Message msg;
    msg.data = (char*)malloc(BUFFER_SIZE * sizeof(char));

    int client_fdt;

    int available_index;

    while (1) {
        
        SYSC(client_fdt, accept(server_fd, (struct sockaddr*) &client_addr, &client_addr_len), "nella accept");
        available_index = -1;
        pthread_mutex_lock(&client_slots_mutex);
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if (!client_slots[i]) {
                available_index = i;
                break;
            }
        }
        pthread_mutex_unlock(&client_slots_mutex);
        if (available_index != -1) {
            client_fds[available_index] = client_fdt;
            for(int i = 0; i < MAX_CLIENTS; i++) {
                printf("fd: %d\n", client_fds[i]);
            }
            DataHandler* threadData = malloc(sizeof(DataHandler));
            threadData -> players = players;
            threadData -> matrix = matrix;
            threadData -> client_fd = client_fds[available_index];
            threadData -> index = available_index;
            threadData -> csv = CSV_out;
            threadData -> seed = rand_seed;
            threadData -> root = root;
            pthread_mutex_lock(&client_slots_mutex);
            client_slots[available_index] = 1;
            pthread_mutex_unlock(&client_slots_mutex);
            for(int i = 0; i < MAX_CLIENTS; i++) {
                printf("cs: %d\n", client_slots[i]);
            }
            if(stato == ATTESA || stato == START) {
                msg.type = MSG_ERR;
                pthread_mutex_lock(&alarm_mutex);
                sprintf(msg.data, "Fase di attesa, registrarsi; inzio prossima partita: %ld secondi\n", DURATA_ATTESA - (time(NULL) - inizio_attesa));
                pthread_mutex_unlock(&alarm_mutex);
                msg.length = strlen(msg.data);
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_message);
                SYSC(n_written, write(client_fds[available_index], serialized_message, msg_size), "nella write attesa");
            } else {
                msg.type = MSG_ERR;
                strcpy(msg.data, "Partita in corso, attendere");
                msg.length = strlen(msg.data);
                msg_size = sizeof(int) + sizeof(char) + msg.length;
                serialize_message(&msg, serialized_message);
                SYSC(n_written, write(client_fds[available_index], serialized_message, msg_size), "nella write partita in corso");
            }
            // Crea un thread per gestire il client
            printf("Creo il thread per gestire il client\n");
            SYSC(retvalue, pthread_create(&ids[available_index], NULL, handler, threadData), "nella pthreadcreate client");
            pthread_mutex_lock(&client_count_mutex);
            client_count++;
            pthread_mutex_unlock(&client_count_mutex);
        } else {
            msg.type = MSG_SERVER_SHUTDOWN;
            sprintf(msg.data, "\nServer pieno, riprova più tardi...\n");
            msg.length = strlen(msg.data);
            msg_size = sizeof(int) + sizeof(char) + msg.length;
            serialize_message(&msg, serialized_message);
            SYSC(n_written, write(client_fdt, serialized_message, msg_size), "nella write full");
        }
    }

    return 0;

}