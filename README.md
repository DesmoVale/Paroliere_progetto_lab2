Paroliere — Multiplayer Word Game (Client/Server in C)

This project implements a multiplayer word game called Paroliere (similar to Boggle) using a client-server architecture in C.
It was developed as part of the Laboratorio II Summer Project at the Department of Computer Science, University of Pisa.

Project Structure

The project consists of two main programs:

paroliere_srv.c — Implements the multithreaded server, responsible for managing connected clients, handling requests, generating game matrices, and maintaining player rankings.

paroliere_cl.c — Implements the client, which connects to the server, sends commands, receives updates, and displays the game state and final scores.

Additional modules improve modularity and maintainability:

File	Description
strutture.h / strutture.c	Data structures required for gameplay.
matrix.h / matrix.c	Functions for matrix generation and display.
serializzazione_messaggi.h / serializzazione_messaggi.c	Serialization/deserialization of messages between server and client.
controllo_parole_matrix.h / controllo_parole_matrix.c	Word validation logic (check if a word exists in the matrix).
format_parole.h / format_parole.c	Word formatting utilities.
macros.h	Macros for syscall error handling.
trie.h / trie.c	Implementation of the Trie data structure for dictionary lookup.
Data Structures

Player — Contains username, score, and words used during a match. Managed via a shared array across threads.

DataHandler — Parameters passed to the thread managing each client connection.

Message — Defines message format (type, length, and content).

DataScorer — Parameters used by the ranking thread (scorer).

DataClient — Used by the asynchronous client thread to receive final ranking updates.

TrieNode — Node structure for the dictionary Trie.

Bacheca — Shared message board protected by mutexes for safe concurrent access.

Algorithms and Logic
Word Search in Matrix

Implements recursive depth-first search using:

is_valid_move — Validates matrix boundaries and visited cells.

search_from — Recursively searches for the word in 8 directions.

exist — Initiates the search from every matrix cell.

Dictionary Search

Implemented using a Trie with search() to check if a word exists in the dictionary.

Communication Mechanism

Communication between client and server is based on serialized messages with three fields:

[length][type][content]


This allows efficient data transfer with a single read/write operation per message.
Messages are serialized/deserialized automatically for reuse.

Server Logic

Multithreaded design: a new thread is spawned for each connected client.

Synchronization: handled through mutexes and condition variables.

Game Phases: managed via signals (alarm) that trigger round transitions.

Ranking: a dedicated scorer thread compiles the leaderboard and sends it asynchronously to all clients.

Signal Handling: SIGINT safely terminates the server.

Client Logic

Connects to the server and displays available commands.

Sends serialized requests and receives responses asynchronously.

Dedicated thread processes matrix and ranking messages (MSG_MATRICE, MSG_PUNTI_FINALI).

Handles SIGINT gracefully on exit.

Testing and Compilation

A Makefile is provided to build and test both the server and client.

Build Commands
make all         # Compile all executables
make testServer  # Run server with default settings
make testClient  # Run client
make clean       # Remove executables and object files

Advanced Test Targets
Command	Description
make testServer1	Run server with --matrix and --duration
make testServer2	Run server with --matrix, --duration, and --seed
make testServer3	Run server with --duration and --seed
make testServer4	Run server with --matrix, --duration, and --dict

Testing was performed locally with multiple terminal windows to simulate several connected clients.

Author

Leone Valerio
Department of Computer Science, University of Pisa
Student ID: 656932
Course A — Laboratory II Summer Project
