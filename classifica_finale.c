#include "strutture.h"
#include "classifica_finale.h"

/***FUNZIONI PER STILARE LA CLASSIFICA*/
// Funzione che ordina per score i giocatori
void sortPlayers(Player* players, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (players[j].score < players[j + 1].score) {
                Player temp = players[j];
                players[j] = players[j + 1];
                players[j + 1] = temp;
            }
        }
    }
}
// Formatto in CSV la classifica
void generateCSV(Player* players, int count, char *output, int* client_slots) {
    sortPlayers(players, count);
    strcpy(output, "\nName,Score\n");
    for (int i = 0; i < count; i++) {
        if(client_slots[i] && players[i].name[0] != '\0') {
            char line[MAX_LENGTH + 10];
            sprintf(line, "%s,%d\n", players[i].name, players[i].score);
            strcat(output, line);
        }
    }
}
/***********************************/