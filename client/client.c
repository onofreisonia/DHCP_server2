#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "dhcp_message.h"

// Eliminam porturile, folosim path-uri din config.h
#define Buffer_size 1024

int main()
{
    int sockfd;
    struct sockaddr_un server_addr, my_addr;

    // creez socket-ul unix
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("Eroare la crearea socketului");
        exit(EXIT_FAILURE);
    }

    // configurez  adresa clientului
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sun_family = AF_UNIX;
    sprintf(my_addr.sun_path, CLIENT_PATH_TEMPLATE, getpid());

    unlink(my_addr.sun_path); // Sterg fisierul daca exista
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Eroare la bind client");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //conf adresa server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_PATH);

    // DHCP DISCOVER
    DHCP_Message discover;
    memset(&discover, 0, sizeof(discover));
    discover.header.op = 1;
    discover.header.htype = 1;
    discover.header.hlen = 6;
    discover.header.xid = rand();
    discover.msg_type = DHCP_DISCOVER;

    sendto(sockfd, &discover, sizeof(DHCP_Message), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[CLIENT] DHCP DISCOVER trimis (xid=%u)\n", discover.header.xid);

    // DHCP OFFER
    DHCP_Message offer;
    int n = recvfrom(sockfd, &offer, sizeof(offer), 0, NULL, NULL);

    if (n < sizeof(DHCP_Message))
    {
        printf("[CLIENT] Mesaj prea scurt\n");
        return 0;
    }
    else if (offer.msg_type == DHCP_OFFER)
    {
        printf("[CLIENT] DHCP OFFER primit: IP=%s Router=%s DNS=%s\n",
               offer.offered_ip, offer.router, offer.dns);
    }

    // DHCP REQUEST
    DHCP_Message req;
    memset(&req, 0, sizeof(req));
    req.header.op = 1;
    req.header.htype = 1;
    req.header.hlen = 6;
    req.header.xid = discover.header.xid;

    req.msg_type = DHCP_REQUEST;
    strcpy(req.offered_ip, offer.offered_ip);

    sendto(sockfd, &req, sizeof(req), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[CLIENT] DHCP REQUEST trimis pentru IP %s\n", req.offered_ip);

    // primire ACK/NAK
    DHCP_Message ack_msg;
    n = recvfrom(sockfd, &ack_msg, sizeof(ack_msg), 0, NULL, NULL);

    if (ack_msg.msg_type == DHCP_ACK)
    {
        printf("[CLIENT] DHCP ACK → IP CONFIRMAT: %s\n", ack_msg.offered_ip);
    }
    else if (ack_msg.msg_type == DHCP_NAK)
    {
        printf("[CLIENT] DHCP NAK → IP REFUZAT\n");
    }

    // DHCP INFORM (exemplu)
    DHCP_Message inform;
    memset(&inform, 0, sizeof(inform));
    inform.header.op = 1;
    inform.header.htype = 1;
    inform.header.hlen = 6;
    inform.header.xid = rand();
    inform.msg_type = DHCP_INFORM;

    sendto(sockfd, &inform, sizeof(inform), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[CLIENT] DHCP INFORM trimis\n");

    DHCP_Message inform_ack;
    recvfrom(sockfd, &inform_ack, sizeof(inform_ack), 0, NULL, NULL);

    if (inform_ack.msg_type == DHCP_ACK)
    {
        printf("[CLIENT] DHCP INFORM-ACK primit: Router=%s DNS=%s\n",
               inform_ack.router, inform_ack.dns);
    }

    close(sockfd);
    // Cleanup socket file
    unlink(my_addr.sun_path);
    return 0;
}
