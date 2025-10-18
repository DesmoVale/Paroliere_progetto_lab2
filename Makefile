# Nome del compilatore
CC = gcc

# Opzioni del compilatore
CFLAGS = -lpthread

# File di sorgente comuni
COMMON_SRCS = classifica_finale.c controllo_parole_matrix.c format_parole.c serializzazione_messaggi.c trie.c matrix.c

# File header comuni
COMMON_HDRS = classifica_finale.h strutture.h serializzazione_messaggi.h controllo_parole_matrix.h format_parole.h trie.h matrix.h

# File di sorgente specifici per il server e il client
SRV_SRCS = paroliere_srv.c $(COMMON_SRCS)
CL_SRCS = paroliere_cl.c $(COMMON_SRCS)

# File oggetto generati
SRV_OBJS = $(SRV_SRCS:.c=.o)
CL_OBJS = $(CL_SRCS:.c=.o)

# Eseguibili
SRV_EXEC = paroliere_srv
CL_EXEC = paroliere_cl 

# Regola di default
all: $(SRV_EXEC) $(CL_EXEC)

# Regola per creare l'eseguibile del server
$(SRV_EXEC): $(SRV_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regola per creare l'eseguibile del client
$(CL_EXEC): $(CL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regola per compilare i file oggetto comuni
%.o: %.c $(COMMON_HDRS)
	$(CC) $(CFLAGS) -c $< -o $@
# Test
testServer:
	./$(SRV_EXEC) localhost 8080

testServer1:
	./$(SRV_EXEC) localhost 8080 --matrix matrix.txt --durata 1

testServer2:
	./$(SRV_EXEC) localhost 8080 --matrix matrix.txt --durata 1 --seed 14

testServer3:
	./$(SRV_EXEC) localhost 8080 --durata 3 --seed 123

testServer4:
	./$(SRV_EXEC) localhost 8080 --matrix matrix.txt --durata 1 --diz dictionary_ita.txt

testClient:
	./$(CL_EXEC) localhost 8080
# Regola di pulizia
clean:
	rm -f $(SRV_OBJS) $(CL_OBJS) $(SRV_EXEC) $(CL_EXEC)

