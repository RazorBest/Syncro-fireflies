#include <cstring>

#include <arpa/inet.h> // htons, inet_addr
#include <sys/types.h> // uint16_t
#include <sys/socket.h> // socket, sendto
#include <unistd.h> // close

#include "./server_connection.h"

#define SEND_SERVER_BUF_LEN 4096

void ServerConnection::start_connection() {
	sock = ::socket(AF_INET, SOCK_DGRAM, 0);

	destination.sin_family = AF_INET;
	destination.sin_port = htons(port);
	destination.sin_addr.s_addr = inet_addr(hostname.c_str());
}

void ServerConnection::send_config_data(int n, int m, int n_cells) {
    char buf[SEND_SERVER_BUF_LEN];
    int len = 0;

    static_assert(SEND_SERVER_BUF_LEN >= sizeof(n) + sizeof(m) + sizeof(n_cells) + sizeof(int));

    memcpy(buf + len, &n, sizeof(n));
    len += sizeof(n);
    memcpy(buf + len, &m, sizeof(m));
    len += sizeof(m);

    memcpy(buf + len, &n_cells, sizeof(n_cells));
    len += sizeof(n_cells);

    int pd_len = sizeof(sendable_point_data);
    memcpy(buf + len, &pd_len, sizeof(pd_len));
    len += sizeof(pd_len);

    ::sendto(sock, buf, sizeof(*buf) * len, 0,
            reinterpret_cast<sockaddr*>(&destination), 
            sizeof(destination)
    );
}

void ServerConnection::send_state(area_matrix &grid) {
    char buf[SEND_SERVER_BUF_LEN];
    int len = 0;

    int count = 0;

    for (int i = 0; i < grid.n; i++) {
        for (int j = 0; j < grid.m; j++) {
            for (int k = 0; k < grid[i][j].v.size(); k++) {
                auto &cell = grid[i][j].v[k];

                count++;

                point_data_to_sendable(buf + len, cell);
                len += sizeof(sendable_point_data);

                if (len >= SEND_SERVER_BUF_LEN) {
                    ::sendto(sock, buf, sizeof(*buf) * len, 0,
                            reinterpret_cast<sockaddr*>(&destination), 
                            sizeof(destination)
                    );

                    len = 0;
                }
            }
        }
    }

    ::sendto(sock, buf, sizeof(*buf) * len, 0,
            reinterpret_cast<sockaddr*>(&destination), 
            sizeof(destination)
    );

}

ServerConnection::~ServerConnection() {
    ::close(sock);
}
