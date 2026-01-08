# Update-uri la Client 

Salut, am facut cateva modificari in `client/` ca sa respectam cerintele legate de scalabilitate si simulare de erori 

## 1. Arhitectura
Proiectul merge pe pattern-ul **MVCS (Model-View-Controller-Service)**, care e super ok pt faza asta:
- **Model**: Structurile de date (`dhcp_message.h` si ce e in shared memory).
- **View**: Printurile  din consola
- **Controller**: `client.c` (se ocupa de flow)
- **Service**: Restul functiilor auxiliare

## 2. Scalabilitate (Semnale)
Am updatat partea de semnale, acum stie sa trateze si **SIGTERM**, nu doar SIGINT. Ideea e ca handler-ul sa fie scalabil daca mai vrem sa bagam semnale pe viitor
Indiferent cum opresti clientul (Ctrl+C sau kill), el o sa trimita un **DHCP RELEASE** la server inainte sa moara, ca sa elibereze IP-ul din IP Pool

## 3. Simulare Erori 
Am bagat o constanta `SIMULATE_ERRORS` direct in cod. 
- E setata sa "piarda" cam 20% din pachetele primite 
- O sa vezi in loguri: `[SIMULARE] Pachet pierdut/ignorat...`

Ca sa nu ramana blocat cand "pierde" un pachet, am pus un **timeout de 2 secunde** pe socket. Daca nu primeste raspuns in 2 secunde, retrimite cererea (DISCOVER sau REQUEST). Deci nu crapa, doar dureaza putin mai mult pana isi ia IP

## 4. Testare Multi-Client (Script)
Am facut si un script `simulate_clients.sh` ca sa testam scenariul cu mai multi clienti simultan.
Ruleaza 3 clienti in paralel 
Il rulezi cu `./simulate_clients.sh` 
