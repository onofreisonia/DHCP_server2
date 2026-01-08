# Actualizări Proiect & Ghid de Utilizare

## 🚀 Modificări Recente

### 1. Sincronizare Avansată (Client & Monitor)
-   **Semafor Binar (Acces Exclusiv)**:
    -   Aplicatia `client_app` initializeaza un semafor binar (valoare 1).
    -   Acesta impune acces strict exclusiv la segmentul de Memorie Partajată. Utilizarea `sem_lock` (wait) și `sem_unlock` (signal) asigură faptul că doar un singur proces (Client sau Monitor) poate accesa memoria la un moment dat.
-   **Mutex Partajat între Procese**:
    -   Un `pthread_mutex_t` a fost adăugat în structura de Memorie Partajată (`ClientState`).
    -   Acesta este configurat cu `PTHREAD_PROCESS_SHARED` pentru a permite blocarea corectă între procese independente (Client vs Monitor).
    -   **Critical Section**: Actualizarea IP-ului și a statusului în Memoria Partajată este acum strict protejată de acest Mutex, în combinație cu semaforul.

`Detaliu`:
    Implementarea logicii de Client (inclusiv mecanismele de sincronizare și gestionarea memoriei partajate) a fost realizată de `Sonia`. Eu am efectuat doar mici modificări în fișierul `monitor.c`, fără a schimba arhitectura de bază.

### 2. Monitorizare Multi-Client
-   **Identificare Bazată pe PID**: Clienții folosesc ID-ul de proces (PID) pentru a genera chei IPC unice (Memorie Partajată & Semafor).
-   **Monitor Dinamic**: Aplicația `monitor_app` scanează automat directorul pentru socket-uri de client active (`client_sock_<PID>`), se atașează la Memoria Partajată corespunzătoare fiecărui client și afișează statusul acestora într-un tabel actualizat în timp real.

### 3. Persistență
-   **Format Text**: Serverul salvează informațiile despre lease-uri în fișierul `leases.txt` (ușor de citit de către utilizator).

### 4. Arhitectură Internă Îmbunătățită
-   **Cozi de Mesaje System V**:
    -   Folosite pentru logare thread-safe. Funcția `logger_thread_func` consumă mesaje trimise de thread-ul principal, thread-ul de rețea și chiar de procesul copil creat prin fork.
-   **Reînnoire Lease prin Fork/Pipe**:
    -   Clientul creează un proces copil dedicat exclusiv gestionării timerelor pentru reînnoirea lease-ului.
    -   **Pipe**: Procesul părinte utilizează un pipe pentru a trimite semnale de control („STOP”) către copil atunci când lease-ul este eliberat sau aplicația se închide.

---

## 🛠️ Cum Rulezi Proiectul

### A. Simulare Automată (Recomandată)
Acest script recompilă întregul proiect din directorul root și pornește simultan 3 clienți pentru a demonstra funcționalitatea multi-client.

```bash
cd Client
./simulare_clienti.sh
```
*(Alternativ, poți rula `make test` din directorul root)*

### B. Rulare Manuală cu Monitor

1.  **Start Server**:
    ```bash
    ./server_app
    ```

2.  **Start Monitor**:
    Deschide un terminal nou. Inițial, tabelul va fi gol.
    ```bash
    ./monitor_app
    ```

3.  **Start Clients**:
    Deschide terminale noi sau rulează scriptul de simulare.
    ```bash
    # Option 1: Manual single client
    ./client_app
    
    # Option 2: Run simulation of 3 clients
    cd Client && ./simulare_clienti.sh
    ```

4.  **Observă Rezultatul**:
    Urmărește terminalul `monitor_app`. Acesta va detecta noile procese (PID-uri), va bloca Mutex-ul, va citi starea și va actualiza tabelul în timp real.

---

## ✅ Verification Checklist
-    **Server**: Alocă IP-uri și le salvează în `leases.txt`.
-    **Client**: Se conectează, primește IP, protejează memoria cu Mutex + Semafor.
-    **Monitor**: Afișează statusul „BOUND” pentru mai mulți clienți simultan.