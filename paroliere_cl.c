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
#define MSG_SERVER_SHUTDOWN 'B'
#define MSG_POST_BACHECA 'H'
#define MSG_SHOW_BACHECA 'S'

int client_fd;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char serialized_msg_t[BUFFER_SIZE];
char buffer_t[BUFFER_SIZE];
pthread_t async_reader;

Message *msg;

char prompt[] = "[PROMPT PAROLIERE]--> ";

//Buffer
char buffer[BUFFER_SIZE];
// Buffer che conterrà il messaggio serializzato da inviare al server
char serialized_msg[BUFFER_SIZE];
// Buffer che contiene il comando esteso inserito dall'utente
char command_buffer[BUFFER_SIZE];
// Token che contiene le sezioni del messaggio
char* token;
// Buffer con il tipo di comando esteso
char* command;
// Flag per l'uscita e la disconnessione
int q = 0;
// Flag che consente di non inviare il messaggio nel caso in cui venga digitato aiuto
int is_help = 0;
// Dimensione del messaggio da inviare
int msg_size;

/*******GESTIONE DEI SEGNALI*******/
void handle_sigint(int sig) {
    ssize_t n_written;
    char serialized_message_s[BUFFER_SIZE];
    msg -> type = MSG_QUIT;
    serialize_message(msg, serialized_message_s);
    SYSC(n_written, write(client_fd, serialized_message_s, sizeof(serialized_message_s)), "nella write sigint");
    pthread_join(async_reader, NULL);
    free(msg -> data);
    free(msg);
    exit(0);
}

/*******FUNZIONE THREAD MESSAGGI ASINCRONI********/
void* async(void* args) {

    ssize_t n_written;
    ssize_t n_read;
    int retvalue;
    int control = 0;
    DataClient* asyncData = (DataClient*)args;

    while(1) {

        memset(buffer_t, 0, BUFFER_SIZE);
        SYSC(n_read, read(client_fd, buffer_t, BUFFER_SIZE), "nella read");

        pthread_mutex_lock(&mutex);

        deserialize_message(buffer_t, msg);

        if(msg -> type == MSG_PUNTI_FINALI) {
            SYSC(n_written, write(STDOUT_FILENO, msg -> data, msg -> length), "nella write punti finali");
            SYSC(n_written, write(STDOUT_FILENO, prompt, strlen(prompt)), "nella write prompt");
            memset(msg -> data, 0, strlen(msg -> data));
        } else if (msg -> type == MSG_MATRICE) {
            if(print_matrix(msg -> data)) {
                msg -> type = MSG_OK;
            } else {
                msg -> type = MSG_ERR;
            }
            serialize_message(msg, serialized_msg_t);
            SYSC(n_written, write(client_fd, serialized_msg_t, sizeof(serialized_msg_t)), "nella write matrice");

            // Ricevo il tempo
            memset(buffer_t, 0, BUFFER_SIZE);
            memset(msg -> data, 0, BUFFER_SIZE);
            SYSC(n_read, read(client_fd, buffer_t, BUFFER_SIZE), "nella read tempo");
            deserialize_message(buffer_t, msg);
        } else if (msg -> type == MSG_QUIT) {
            printf("\n\n%s\n", msg -> data);
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
            break;
        } else if (msg -> type == MSG_SERVER_SHUTDOWN) {
            printf("\n\n%s\n\n", msg -> data);
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&mutex);
            free(msg -> data);
            free(msg);
            free(asyncData);
            exit(0);
            break;
        }

        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&mutex);

    }

}


int main(int argc, char* argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int retvalue;
    ssize_t n_written, n_read;
    struct sockaddr_in server_addr;

    // Creazione del socket
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    char srv_name[10];

    if(strcmp(argv[1], "localhost") == 0){
        strcpy(srv_name, "127.0.0.1");
    } else {
        strcpy(srv_name, argv[1]);
    }

    // Inizializzazione della struttura server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(srv_name);

    signal(SIGINT, handle_sigint);

    DataClient* asyncData = malloc(sizeof(DataClient));

    asyncData -> buffer = buffer;
    asyncData -> serialized_msg = serialized_msg;
    asyncData -> msg_size = &msg_size;

    printf("******COMANDI E REGOLE******\n");
    printf("Durante la sessione di attesa inizio partita\n");
    printf("Sarà possibile inviare solo messaggi di tipo:\n");
    printf("-> registra_utente <nome>\n");
    printf("-> matrice: restituisce il tempo restante prima che inizi la partita\n");
    printf("-> punteggio_finale\n");
    printf("-> fine: effettua il logout\n");
    printf("-> msg <messaggio>: posta in bacheca un messaggio\n");
    printf("-> show_msg: visualizza bacheca messaggi\n");
    printf("Durante la sessione di gioco sarà possibile\n");
    printf("Inviare messaggi di tipo:\n");
    printf("-> registra_utente <nome>\n");
    printf("-> matrice: restituisce la matrice e il tempo di gioco residuo\n");
    printf("-> p <parola>: comando per inviare una parola\n");
    printf("-> fine: effettua il logout\n");
    printf("-> msg <messaggio>: posta in bacheca un messaggio\n");
    printf("-> show_msg: visualizza bacheca messaggi\n");
    printf("********************************\n\n");

    printf("Connessione al server...\n");

    // Connect
    SYSC(retvalue, connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "nella connect");

    // Allocazione memoria per msg
    msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        perror("Errore nell'allocazione di msg");
        exit(EXIT_FAILURE);
    }
    msg -> data = (char*)malloc(BUFFER_SIZE * sizeof(char));
    if (msg -> data == NULL) {
        perror("Errore nell'allocazione di msg->data");
        free(msg);
        exit(EXIT_FAILURE);
    }

    // Ricezione del messaggio di benvenuto del server
    SYSC(n_read, read(client_fd, buffer, BUFFER_SIZE), "nella read welcome");
    deserialize_message(buffer, msg);
    printf("%s\n", msg -> data);
    if(msg -> type == MSG_SERVER_SHUTDOWN) {
        free(msg -> data);
        free(msg);
        free(asyncData);
        exit(0);
    }

    pthread_create(&async_reader, NULL, async, asyncData);
    

    while(!q) {

        memset(buffer, 0, BUFFER_SIZE);
        memset(msg -> data, 0, BUFFER_SIZE);
        memset(command_buffer, 0, BUFFER_SIZE);

        SYSC(n_written, write(STDOUT_FILENO, prompt, strlen(prompt)), "nella write prompt");
        SYSC(n_read, read(STDIN_FILENO, command_buffer, BUFFER_SIZE), "nella read comando");

        pthread_mutex_lock(&mutex); // Blocca il mutex prima di attendere la condizione

        token = strtok(command_buffer, " \n");
        int c = 0;
        command = NULL;
        while(token != NULL) {
            if(!c) {
                command = token;
            } else {
                strcpy(msg -> data, token);
            }
            token = strtok(NULL, " \n");
            c++;
        }

        // Se il comando è vuoto, inviamo un messaggio di no-op (inutile)
        if (command == NULL) {
            command = "noop";  // Comando fittizio
            msg -> type = 'X'; // Tipo di messaggio inutile
            strcpy(msg -> data, "");  // Nessun dato da inviare
        } else {
            msg -> length = strlen(msg -> data);

            // Determino il tipo di messaggio
            if(strcmp(command, "registra_utente") == 0) {
                msg -> type = MSG_REGISTRA_UTENTE;
            } else if(strcmp(command, "matrice") == 0) {
                msg -> type = MSG_MATRICE;
            } else if(strcmp(command, "fine") == 0) {
                msg -> type = MSG_QUIT;
            } else if(strcmp(command, "p") == 0) {
                msg -> type = MSG_PAROLA;
            } else if(strcmp(command, "punteggio_finale") == 0) {
                msg -> type = MSG_PUNTI_FINALI;
            } else if(strcmp(command, "aiuto") == 0) {
                printf("\n\nLista dei comandi disponibili e uso:\n");
                printf("registra_utente <nome_utente> : permette di registrare il proprio user, in caso di riutilizzo l'username verrà sovrascritto\n");
                printf("matrice : permette di vedere la matrice in cui ricercare le parole assieme al tempo di gioco rimanente\n");
                printf("p <parola>: inserisci una parola\n");
                printf("punteggio_finale: stampa la classifica del game precedente\n");
                printf("fine: consente di disconnettersi dalla sessione\n");
                printf("msg <messaggio>: posta in bacheca un messaggio\n");
                printf("show_msg: visualizza bacheca messaggi\n");
                printf("aiuto\n\n");

                is_help = 1;
            } else if (strcmp(command, "msg") == 0) {
                msg -> type = MSG_POST_BACHECA;
            } else if (strcmp(command, "show_msg") == 0) {
                msg -> type = MSG_SHOW_BACHECA;
            } else {
                msg -> type = 'X'; // Messaggio sconosciuto o fittizio
            }
        }

        if(!is_help) {
            // Serializzazione del messaggio
            serialize_message(msg, serialized_msg);

            // Invia il messaggio serializzato al server
            SYSC(n_written, write(client_fd, serialized_msg, sizeof(serialized_msg)), "nella write invio messaggio");

            pthread_cond_wait(&cond1, &mutex);

            switch (msg -> type) {
                case MSG_OK:
                    printf("\n%s\n\n", msg -> data);
                    break;
                case MSG_ERR:
                    printf("\n%s\n\n", msg -> data);
                    break;
                case MSG_PAROLA:
                    printf("\n%s\n\n", msg -> data);
                    break;
                case MSG_QUIT:
                    q = 1;
                    break;
                case MSG_TEMPO_ATTESA:
                    printf("\n%s secondi\n\n", msg -> data);
                    break;
                case MSG_TEMPO_PARTITA:
                    printf("\n%s secondi\n\n", msg -> data);
                    break;
                default:
                    printf("Risposta sconosciuta\n");
                    break;
            }
        } else {
            is_help = 0;
        }

        pthread_mutex_unlock(&mutex);
    }

    pthread_join(async_reader, NULL);

    printf("\n[MAIN] Chiudo il socket e libero la memoria\n\n");
    SYSC(retvalue, close(client_fd), "nella close fd");
    free(msg -> data);
    free(msg);
    free(asyncData);
    return 0;
}