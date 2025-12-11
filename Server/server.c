#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "config.h"
#include "dhcp_message.h"

#define SERVER_DHCP_PORT 1067
#define POOL_SIZE 100

int main()
{
    DHCP_ipconfig configuratie;
    if (load_config("Server/ipconfig.txt", &configuratie) == -1) {
        fprintf(stderr, "Nu s-a incarcat configuratia serverului\n");
        return 1;
    }

    printf("Server DHCP pornit.\n");

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Eroare la crearea socketului server");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_DHCP_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Eroare la bind server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server asculta pe portul %d...\n", SERVER_DHCP_PORT);

    // IP pool
    IP_Entry ip_pool[POOL_SIZE];
    struct in_addr base_ip;
    inet_aton(configuratie.range_start, &base_ip);

    for (int i = 0; i < POOL_SIZE; i++)
    {
        ip_pool[i].allocated = 0;
        ip_pool[i].lease_start = 0;
        ip_pool[i].lease_time = 0;

        struct in_addr tmp;
        tmp.s_addr = htonl(ntohl(base_ip.s_addr) + i);
        strcpy(ip_pool[i].ip, inet_ntoa(tmp));
    }

    while (1)
    {
        DHCP_Message msg_recv;
        int n = recvfrom(sockfd, &msg_recv, sizeof(msg_recv), 0,
                         (struct sockaddr *)&client_addr, &addr_len);

        if (n < sizeof(DHCP_Message))
            continue;

        switch (msg_recv.msg_type)
        {
        case DHCP_DISCOVER:
            printf("[SERVER] DISCOVER primit (xid=%u)\n", msg_recv.header.xid);
            handle_dhcp_discover(sockfd, &client_addr, &msg_recv,
                                 &configuratie, ip_pool, POOL_SIZE);
            break;

        case DHCP_REQUEST:
            printf("[SERVER] REQUEST primit\n");
            handle_dhcp_request(sockfd, &client_addr, &msg_recv,
                                &configuratie, ip_pool, POOL_SIZE);
            break;

        case DHCP_RELEASE:
            printf("[SERVER] RELEASE primit\n");
            handle_dhcp_release(&msg_recv, ip_pool, POOL_SIZE);
            break;

        case DHCP_INFORM:
            printf("[SERVER] INFORM primit\n");
            send_dhcp_ack_inform(sockfd, &client_addr, &msg_recv, &configuratie);
            break;

        default:
            printf("[SERVER] Mesaj necunoscut: %d\n", msg_recv.msg_type);
            break;
        }
    }

    close(sockfd);
    return 0;
}
