#define ALPHABET_SIZE 26

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    int isEndOfWord;
} TrieNode;

TrieNode* createNode(void);

void insert(TrieNode* root, char* word);

int search(TrieNode* root, char* word);

void initTrie(char* filename, TrieNode* root);

void freeTrie(TrieNode* node);