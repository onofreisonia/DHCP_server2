# DHCP Server

Acest proiect implementează un server și un client DHCP simplificat.

---> Server

Inițializează un socket UDP și activează opțiunile:

1.  SO\_REUSEADDR și SO\_BROADCAST pentru reutilizarea portului și recepția broadcasturilor;

2.  Încarcă din ipconfig.txt adresele disponibile pentru alocare; ---> format ipconfig.txt: subrețeaua utilizată, intervalul de adrese IP disponibile, gateway-ul, serverul DNS și timpii de alocare a adreselor

3.  Primește pachete de tip broadcast trimise de clienți;

4.  Identifică mesajele DHCP Discover și le salvează în memorie;

5.  Răspunde cu DHCP Offer, gestionează DHCP Request și trimite DHCP Ack/Nak în funcție de disponibilitatea adreselor IP;

6.  Monitorizează timpii de lease și eliberează adresele expirate.

---> Client

1.  Creează un socket UDP și activează opțiunea SO_BROADCAST;

2.  Trimite mesaje de tip broadcast (DHCP Discover) către portul serverului;

3.  Utilizează structuri sockaddr\_in pentru trimiterea și recepționarea pachetelor;

4.  Afișează mesajele trimise, răspunsurile primite de la server (Offer, Ack, Nak) și confirmarea finală a configurării IP-ului.

---> Structura Pachetelor (dhcp_message.h)

Definește structura mesajelor DHCP utilizate pentru comunicarea între client și server, incluzând:
- Header-ul standard DHCP (op, htype, hlen, xid etc.);
- Tipurile de mesaje (DISCOVER, OFFER, REQUEST, ACK, NAK, INFORM);
- Câmpurile pentru IP oferit, Router, DNS și alte opțiuni.

---> Makefile

Permite compilarea atât a surselor C, cât și a fișierelor header (.h) și a modulelor auxiliare (config.c, client.c, server.c);

Include reguli separate pentru:

1.  make server – compilarea doar a serverului;

2.  make client – compilarea doar a clientului;

3.  make all – compilarea ambelor;

4.  make clean – ștergerea executabilelor și fișierelor obiect.