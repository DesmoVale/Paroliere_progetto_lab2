#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "trie.h"

// Funzione per creare un nuovo nodo del trie
TrieNode* createNode(void) {
    TrieNode* node = (TrieNode* )malloc(sizeof(TrieNode));
    node -> isEndOfWord = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        node -> children[i] = NULL;
    }
    return node;
}

// Inserimento di una nuova parola
void insert(TrieNode* root, char* word) {
    TrieNode *node = root;
    int length = strlen(word);  

    for (int i = 0; i < length; i++) {
        int index = word[i] - 'a';  
        if (!node -> children[index]) {
            node -> children[index] = createNode();  
        }
        node = node -> children[index];  
    }
    node -> isEndOfWord = 1;
}

// Funzione per cercare una parola nel trie
int search(TrieNode* root, char *word) {
    TrieNode *node = root;
    int length = strlen(word);

    for (int i = 0; i < length; i++) {
        int index = word[i] - 'a';
        if (!node -> children[index]) {
            return 0;
        }
        node = node -> children[index];
    }
    
    return node -> isEndOfWord;
}

// Funzione per leggere il dizionario da un file e costruire il trie
void initTrie(char* filename, TrieNode* root) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Impossibile aprire il file");
        exit(EXIT_FAILURE);
    }

    char word[256];
    while (fscanf(file, "%s", word) != EOF) {
        insert(root, word);
    }

    fclose(file);
}

// Funzione per deallocare il trie
void freeTrie(TrieNode* node) {
    if (node == NULL) {
        return;
    }

    // Dealloca tutti i figli del nodo corrente
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        freeTrie(node -> children[i]);
    }

    // Dealloca il nodo corrente
    free(node);
}
