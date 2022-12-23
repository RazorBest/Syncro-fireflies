#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <string>
#include <netinet/in.h> // sockaddr_in

#include "./server_connection.h"
#include "../cells/cells.h"

class ServerConnection {
public:
    ServerConnection(const std::string &hostname, int port) : hostname(hostname), port(port) {}

    void start_connection();
    void send_config_data(int n, int m, int n_cells);
    void send_state(area_matrix &grid);

    ~ServerConnection();

private:
    std::string hostname;
    int port;
    int sock;
    sockaddr_in destination;
};

#endif // SERVER_CONNECTION_H
