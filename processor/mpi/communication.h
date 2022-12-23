#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define COMM_BUF_LEN 32768

void send_char_array(char *array, long size, int dest);
void recv_char_array(char *array, long *size, int src);

#endif // COMMMUNICATION_H
