# 🎮 Paroliere — Multiplayer Word Game

A multiplayer word game similar to Boggle, built with a client-server architecture in C. Developed as part of the **Laboratorio II Summer Project** at the Department of Computer Science, University of Pisa.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [Data Structures](#data-structures)
- [Core Algorithms](#core-algorithms)
- [Communication Protocol](#communication-protocol)
- [Server Architecture](#server-architecture)
- [Client Architecture](#client-architecture)
- [Building and Testing](#building-and-testing)
- [Author](#author)

---

## 🎯 Overview

Paroliere is a competitive word-finding game where players search for words in a randomly generated letter matrix. The implementation features:

- **Multithreaded server** handling multiple concurrent clients
- **Efficient communication** through message serialization
- **Real-time ranking system** with asynchronous updates
- **Trie-based dictionary** for fast word validation

---

## 📁 Project Structure

### Main Programs

- **`paroliere_srv.c`** — Multithreaded server managing clients, game matrices, and player rankings
- **`paroliere_cl.c`** — Client handling user interaction, commands, and game state display

### Supporting Modules

| File | Description |
|------|-------------|
| `strutture.h` / `strutture.c` | Core game data structures |
| `matrix.h` / `matrix.c` | Matrix generation and display functions |
| `serializzazione_messaggi.h` / `.c` | Message serialization/deserialization |
| `controllo_parole_matrix.h` / `.c` | Word validation in matrix |
| `format_parole.h` / `format_parole.c` | Word formatting utilities |
| `macros.h` | System call error handling macros |
| `trie.h` / `trie.c` | Trie data structure for dictionary lookup |

---

## 🗂️ Data Structures

| Structure | Purpose |
|-----------|---------|
| **Player** | Stores username, score, and words used during a match |
| **DataHandler** | Parameters for client connection threads |
| **Message** | Defines message format (type, length, content) |
| **DataScorer** | Parameters for the ranking thread |
| **DataClient** | Handles asynchronous ranking updates |
| **TrieNode** | Node structure for dictionary Trie |
| **Bacheca** | Shared message board with mutex protection |

---

## 🧮 Core Algorithms

### 🔍 Word Search in Matrix

Implements recursive depth-first search with:

- **`is_valid_move`** — Validates matrix boundaries and visited cells
- **`search_from`** — Recursively searches in 8 directions
- **`exist`** — Initiates search from every matrix cell

### 📚 Dictionary Lookup

Uses a **Trie data structure** with `search()` function for O(n) word validation, where n is the word length.

---

## 📡 Communication Protocol

Messages between client and server follow a three-field format:

```
[length][type][content]
```

- **Single read/write operation** per message
- **Automatic serialization/deserialization** for reusability
- **Type-based routing** for different message handlers

---

## 🖥️ Server Architecture

### Design Features

- **Multithreading** — New thread spawned per client connection
- **Synchronization** — Mutexes and condition variables ensure thread safety
- **Game Phases** — Managed via `alarm` signals for round transitions
- **Ranking System** — Dedicated scorer thread compiles and broadcasts leaderboards
- **Graceful Shutdown** — `SIGINT` handler for safe termination

---

## 💻 Client Architecture

### Features

- Connects to server and displays available commands
- Sends serialized requests and receives asynchronous responses
- **Dedicated thread** processes matrix and ranking messages
- Handles `SIGINT` for graceful exit

---

## 🛠️ Building and Testing

A `Makefile` is provided for easy compilation and testing.

### Basic Commands

```bash
make all         # Compile all executables
make testServer  # Run server with default settings
make testClient  # Run client
make clean       # Remove executables and object files
```

### Advanced Test Targets

| Command | Description |
|---------|-------------|
| `make testServer1` | Run server with `--matrix` and `--duration` |
| `make testServer2` | Run server with `--matrix`, `--duration`, and `--seed` |
| `make testServer3` | Run server with `--duration` and `--seed` |
| `make testServer4` | Run server with `--matrix`, `--duration`, and `--dict` |

### Testing Setup

Testing was performed locally using multiple terminal windows to simulate concurrent client connections.
