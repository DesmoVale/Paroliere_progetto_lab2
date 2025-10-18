#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include "trie.h"
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
#define MSG_SHUTDOWN 'S'
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 32
#define MAX_LENGTH 16
#define MAX_USERNAME_LENGTH 10
#define ROWS 4
#define COLS 4
#define alphabet "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define chars_user "abcdefghijklmopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define MAX_MSG 8
#define MAX_SIZE_MSG 128

typedef struct {
    char name[MAX_LENGTH];
    int score;
    char set[BUFFER_SIZE][MAX_LENGTH];
} Player;

typedef struct {
    Player* players;
    char** matrix;
    int client_fd;
    int index;
    char* csv;
    char* matrix_filename;
    int seed;
    TrieNode* root;
    int terminate;
} DataHandler;

typedef struct {
    int length;
    char type;
    char* data;
} Message;

typedef struct {
    Player* players;
    int* client_fds;
    int* client_slots;
} DataScorer;

typedef struct {
    Message msg;
    char* buffer;
    char* serialized_msg;
    int* msg_size;
} DataClient;


