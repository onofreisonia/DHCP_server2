#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

int load_config(const char *filename, DHCP_ipconfig *config) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Eroare la deschiderea fisierului de configurare");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        if (strstr(line, "subnet")) {
            sscanf(line, "subnet %15s netmask %15s", config->subnet, config->netmask);
        } else if (strstr(line, "range")) {
            sscanf(line, "range %15s %15s", config->range_start, config->range_end);
        } else if (strstr(line, "router")) {
            sscanf(line, "option router %15s", config->router);
        } else if (strstr(line, "domain-name-servers")) {
            sscanf(line, "option domain-name-servers %15s", config->dns);
        } else if (strstr(line, "default-lease-time")) {
            sscanf(line, "default-lease-time %d", &config->default_lease);
        } else if (strstr(line, "max-lease-time")) {
            sscanf(line, "max-lease-time %d", &config->max_lease);
        }
    }

    fclose(f);
    return 0;
}
