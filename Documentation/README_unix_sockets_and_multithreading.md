# DHCP Server - Modificări Multithreading & Unix Sockets

Această ramură conține implementarea serverului DHCP, adaptată pentru a utiliza **Unix Domain Sockets** și o arhitectură **Multithreaded**.

## Modificări aduse:
* **Sincronizare cu Mutexuri**: Utilizate pentru a proteja accesul concurent la baza de date de IP-uri (`ipconfig.txt`), prevenind alocarea aceluiași IP către mai mulți clienți.
* **Semafoare**: Implementate pentru a limita numărul de thread-uri active și a gestiona fluxul de cereri.
* **Arhitectură AF_UNIX**: Comunicarea se realizează local prin socket-uri Unix, eliminând overhead-ul protocolului UDP.
* **Script de Testare (`test_clienti.sh`)**: Simulează **3 clienți simultani** pentru a valida comportamentul serverului în condiții de concurență (multithreading).

## Modificari necesare pentru rulare:
1. Fisierele `Makefile` si `test_clienti.sh` trebuie mutate din folderul Server pentru a fi la comun.
2. Dupa se poate trece la procesul de executie explicat de mai jos.

## Cum se rulează:
1. `make`
2. `./server`
3. Într-un alt terminal: `./test_clienti.sh`
