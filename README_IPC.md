

Procese, Thread-uri, IPC (Pipe, Message Queues, Shared Memory, Semafoare) si Semnale

# 1. Arhitectura Noua a Clientului

Clientul nu mai este un simplu proces secvential. Acum este structurat astfel:

# Procesul Principal (Client Parent)
Rol: Gestioneaza initializarea si coordonarea.
Thread-uri:
    1.  Input Thread: Asteapta comenzi de la tastatura (status, release, exit) fara sa blocheze receptia de mesaje
    2.  Network Thread: Asculta continuu socket-ul pentru mesaje de la server (ACK, NAK, etc)
    3.  Logger Thread: Primeste log-uri de la procesul copil prin *Message Queue* si le afiseaza
Sincronizare: 
    Foloseste mutex pentru a proteja datele partajate (current_ip, lease_time) intre thread-uri

# Procesul Copil
Creat prin: fork() dupa obtinerea IP-ului
Rol: Monitorizeaza timpul de lease si trimite automat cererea de RENEW la jumatatea timpului
Comunicare (IPC):
    - Pipe (Parinte -> Copil): Primeste semnalul de STOP daca utilizatorul da release manual
    - Message Queue (Copil -> Parinte): Trimite mesaje de log (Am porni, Trimit renew) catre Logger Thread, evitand scrierea dezordonata la consola

# Procesul Monitor (Dashboard)
- Program separat: monitor
- Rol: Afiseaza in timp real starea clientului (IP, Lease) intr-un terminal separat
- Comunicare (IPC):
    - Shared Memory: Clientul scrie starea aici
    - Semafor: Sincronizeaza accesul (Reader-Writer) pentru a evita citirea datelor incomplete

# 2. Adaugate

 Thread-uri : Interfata neblocanta
 Fork: Separarea logicii de renewal
 Pipe: Control parinte-copil
 Message Queue : Logging asincron intre procese
 Shared Memory : Exportarea starii catre Monitor
 Semafoare : Protectie memorie partajata
 Semnale : Gestionare Ctrl+C pentru Release automat



#PT ANDREEA

- Am modificat config.h in client pentru a cauta socket-ul serverului la `../Server/server_sock`.
- Important: Serverul trebuie sa asculte pe un socket (fișier) creat relativ la folderul de executie sau sa ne asiguram ca path-urile coincid
- Daca rulezi serverul, ar trebui sa nu aiba conflicte, dar daca vrei sa rulezi si clientul meu, trebuie sa ai bibliotecile de pthread instalate 
