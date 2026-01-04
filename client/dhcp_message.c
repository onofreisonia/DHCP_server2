#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dhcp_message.h"

// ======================
// FUNCȚII EXISTENTE
// ======================

int allocate_ip(IP_Entry *ip_pool, int pool_size,
                char *assigned_ip, int default_lease)
{
    for (int i = 0; i < pool_size; i++)
    {
        if (ip_pool[i].allocated == 0)
        {
            strcpy(assigned_ip, ip_pool[i].ip);
            ip_pool[i].allocated = 1;
            ip_pool[i].lease_start = time(NULL);
            ip_pool[i].lease_time = default_lease;
            return 1;
        }
    }
    return 0;
}

void send_dhcp_offer(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                     DHCP_Message *discover, DHCP_ipconfig *conf,
                     IP_Entry *ip_pool, int pool_size)
{
    DHCP_Message offer;
    memset(&offer, 0, sizeof(offer));

    offer.header.op = 2; // reply
    offer.header.htype = 1;
    offer.header.hlen = 6;
    offer.header.xid = discover->header.xid;

    offer.msg_type = DHCP_OFFER;
    strcpy(offer.router, conf->router);
    strcpy(offer.dns, conf->dns);
    offer.lease_time = conf->default_lease;

    // aloc IP
    if (!allocate_ip(ip_pool, pool_size, offer.offered_ip,
                     conf->default_lease))
    {
        printf("[SERVER] Nu mai sunt IP-uri disponibile!\n");
        return;
    }

    sendto(sockfd, &offer, sizeof(offer), 0,
           client_addr, addr_len);

    printf("[SERVER] DHCP OFFER → %s\n", offer.offered_ip);
}

void handle_dhcp_discover(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                          DHCP_Message *discover, DHCP_ipconfig *conf,
                          IP_Entry *ip_pool, int pool_size)
{
    send_dhcp_offer(sockfd, client_addr, addr_len,
                    discover, conf, ip_pool, pool_size);
}

// ==========================
//    DHCP REQUEST
// ==========================

void handle_dhcp_request(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                         DHCP_Message *request, DHCP_ipconfig *conf,
                         IP_Entry *ip_pool, int pool_size)
{
    for (int i = 0; i < pool_size; i++)
    {
        if (strcmp(ip_pool[i].ip, request->offered_ip) == 0)
        {
            if (ip_pool[i].allocated == 1)
            {
                send_dhcp_ack(sockfd, client_addr, addr_len, request, conf);
            }
            else
            {
                send_dhcp_nak(sockfd, client_addr, addr_len, request);
            }
            return;
        }
    }

    send_dhcp_nak(sockfd, client_addr, addr_len, request);
}

// ==========================
//         DHCP ACK
// ==========================

void send_dhcp_ack(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                   DHCP_Message *req, DHCP_ipconfig *conf)
{
    DHCP_Message ack;
    memset(&ack, 0, sizeof(ack));

    ack.header.op = 2;
    ack.header.htype = 1;
    ack.header.hlen = 6;
    ack.header.xid = req->header.xid;

    ack.msg_type = DHCP_ACK;

    strcpy(ack.offered_ip, req->offered_ip);
    strcpy(ack.router, conf->router);
    strcpy(ack.dns, conf->dns);
    ack.lease_time = conf->default_lease;

    sendto(sockfd, &ack, sizeof(ack), 0,
           client_addr, addr_len);

    printf("[SERVER] DHCP ACK → %s\n", ack.offered_ip);
}

// ==========================
//         DHCP NAK
// ==========================

void send_dhcp_nak(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                   DHCP_Message *req)
{
    DHCP_Message nak;
    memset(&nak, 0, sizeof(nak));

    nak.header.op = 2;
    nak.header.htype = 1;
    nak.header.hlen = 6;
    nak.header.xid = req->header.xid;
    nak.msg_type = DHCP_NAK;

    sendto(sockfd, &nak, sizeof(nak), 0,
           client_addr, addr_len);

    printf("[SERVER] DHCP NAK trimis\n");
}

// ==========================
//      DHCP RELEASE
// ==========================

void handle_dhcp_release(DHCP_Message *msg, IP_Entry *ip_pool,
                         int pool_size)
{
    for (int i = 0; i < pool_size; i++)
    {
        if (strcmp(ip_pool[i].ip, msg->offered_ip) == 0)
        {
            ip_pool[i].allocated = 0;
            ip_pool[i].lease_start = 0;
            ip_pool[i].lease_time = 0;
            printf("[SERVER] IP %s eliberat\n", msg->offered_ip);
            return;
        }
    }
}

// ==========================
//      DHCP INFORM
// ==========================

void send_dhcp_ack_inform(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                          DHCP_Message *inform, DHCP_ipconfig *conf)
{
    DHCP_Message ack;
    memset(&ack, 0, sizeof(ack));

    ack.header.op = 2;
    ack.header.htype = 1;
    ack.header.hlen = 6;
    ack.header.xid = inform->header.xid;
    ack.msg_type = DHCP_ACK;

    strcpy(ack.router, conf->router);
    strcpy(ack.dns, conf->dns);

    sendto(sockfd, &ack, sizeof(ack), 0,
           client_addr, addr_len);

    printf("[SERVER] DHCP INFORM-ACK → Router=%s DNS=%s\n",
           ack.router, ack.dns);
}
