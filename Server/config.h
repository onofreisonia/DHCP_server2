#ifndef CONFIG_H
#define CONFIG_H
// The client uses "server_sock" in the local directory (relative path).
// We update this to match the client's expectation.
#define SERVER_PATH "server_sock"
#define CLIENT_PATH_TEMPLATE "client_sock_%d"
typedef struct {
    char subnet[16]; //192.168.1.0 
    char netmask[16]; //255.255.255.0
    char range_start[16]; //192.168.1.10 
    char range_end[16]; //192.168.1.100
    char router[16]; //192.168.1.1
    char dns[16]; //8.8.8.8
    int default_lease; //600
    int max_lease; //7200
} DHCP_ipconfig;
int load_config(const char *filename, DHCP_ipconfig *config);
#endif