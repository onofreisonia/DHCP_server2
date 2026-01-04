#ifndef DHCP_MESSAGE_H
#define DHCP_MESSAGE_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "config.h"

// Codurile pentru tipurile de mesaje DHCP
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7
#define DHCP_INFORM   8

typedef struct {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    unsigned char chaddr[16];
    char sname[64];
    char file[128];
} DHCP_Header;

typedef struct {
    DHCP_Header header;
    int msg_type;
    char offered_ip[16];
    char router[16];
    char dns[16];
    int lease_time;
} DHCP_Message;

typedef struct {
    char ip[16];
    int allocated;
    time_t lease_start;
    int lease_time;
} IP_Entry;

void handle_dhcp_discover(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                          DHCP_Message *discover, DHCP_ipconfig *conf,
                          IP_Entry *ip_pool, int pool_size);

void send_dhcp_offer(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                     DHCP_Message *discover, DHCP_ipconfig *conf,
                     IP_Entry *ip_pool, int pool_size);

int allocate_ip(IP_Entry *ip_pool, int pool_size,
                char *assigned_ip, int default_lease);

// === Mesaje suplimentare ===
void handle_dhcp_request(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                         DHCP_Message *request, DHCP_ipconfig *conf,
                         IP_Entry *ip_pool, int pool_size);

void send_dhcp_ack(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                   DHCP_Message *req, DHCP_ipconfig *conf);

void send_dhcp_nak(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                   DHCP_Message *req);

void handle_dhcp_release(DHCP_Message *msg, IP_Entry *ip_pool,
                         int pool_size);

void send_dhcp_ack_inform(int sockfd, struct sockaddr *client_addr, socklen_t addr_len,
                          DHCP_Message *inform, DHCP_ipconfig *conf);

#endif
