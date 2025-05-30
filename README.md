# C Multi-Client Mirror Server

This project implements a basic TCP server-client system in C where:
* Multiple clients can connect to the server (up to a defined maximum).
* The server sends an initial file to each new client.
* Each client's keystrokes (including arrow keys) are mirrored to all other connected clients.
* Clients also receive their own input echoed back for confirmation.

🚀 Features
* Concurrent client support using poll().
* Keystroke mirroring between all connected clients.
* Initial file transmission on connect (e.g. welcome message or map).
* Graceful client disconnect handling.
* Uses only standard C libraries and POSIX sockets.

🧰 Requirements
* GCC (or any C99-compatible compiler)
* Linux/Unix environment
* Terminal that supports arrow key input (e.g., xterm, GNOME Terminal)

⚙️ Building:
~~~~~~~~~~~~~~~~~~~~
gcc server.c -o server
gcc client.c -o client
~~~~~~~~~~~~~~~~~~~~~

🖥️ How to run: 
~~~~~~~~~~~~~~~
./server file_name <port_number>
./client <hostname> <port_number>
~~~~~~~~~~~~~~~

Example: 
~~~~~~~~~~~~~~~~~
./server file.txt 5555
./client localhost 5555
~~~~~~~~~~~~~~~~

⌨️ Controls
Type letters to broadcast them to all clients.

Use arrow keys (← ↑ → ↓) — they will appear as [ARROW:<dir>] messages.

Press Ctrl+C to quit a client or terminate the server.
