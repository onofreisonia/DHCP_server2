# Modificări pentru Unix Domain Sockets

Clientul a fost modificat să folosească socket-uri Unix (AF_UNIX) în loc de socket-uri INET (AF_INET).

## Modificări importante

### Client (`client.c` și `config.h`)
- Nu se mai folosesc porturi UDP.
- Se folosesc path-uri din `config.h`:
  - Server: `/tmp/dhcp_server`
  - Client: `/tmp/dhcp_client_<PID>`
- Clientul își creează un socket unic și îl șterge la ieșire.

### DHCP Message (`dhcp_message.c` / `.h`)
> **Important:** Verificați funcțiile din `dhcp_message.c`.
> Semnăturile au fost modificate pentru a accepta `struct sockaddr*` generic, deoarece `sockaddr_in` nu mai este folosit exclusiv.

Vă rugăm să vă asigurați că serverul ascultă pe calea specificată și este compatibil cu aceste structuri.
