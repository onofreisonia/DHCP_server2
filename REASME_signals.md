//Semnale 

Am adaugat suport pentru `SIGINT` (Ctrl+C).
- Daca opresti clientul cu Ctrl+C si ai un IP alocat, clientul trimite automat `DHCP RELEASE`
- Clientul sterge socket-ul de pe disc inainte sa iasa.
- Am folosit `sigaction` ca sa fie totul sigur.
