#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <locale.h>
#include "lib.h"

#define QUEUE_LEN 10
#define NOT_DEFINED ""

int main(void) {

  int listen_sock_fd, conn_sock_fd;
  struct sockaddr_in6 server_address;
  struct sockaddr_in6 client_address;
  unsigned int len = sizeof(client_address);
  char str_addr[INET6_ADDRSTRLEN];
  fd_set set, test_set;
  struct timeval tv;
  int ret, flag, i;
  char nick[20];
  char nicks[QUEUE_LEN+4][20];
  char buffer[1000];
  char message[1022];

  setlocale(LC_CTYPE,"en_US.UTF-8");

  for (i = 4; i < QUEUE_LEN+4; i++) {
    strcpy(nicks[i], NOT_DEFINED);
  }

  listen_sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

  /* Set listen socket non-blocking */
  flag = fcntl(listen_sock_fd, F_GETFL, 0);
  fcntl(listen_sock_fd, F_SETFL, flag | O_NONBLOCK);

  server_address.sin6_family = AF_INET6;
  server_address.sin6_addr = in6addr_any;
  server_address.sin6_port = htons(PORT);

  bind(listen_sock_fd, (struct sockaddr *)&server_address,
       sizeof(server_address));

  listen(listen_sock_fd, QUEUE_LEN);

  FD_ZERO(&set);
  FD_SET(listen_sock_fd, &set);

  while (1) {
    test_set = set;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    ret = select(FD_SETSIZE, &test_set, NULL, NULL, &tv);

    if (ret == -1) {
      perror("select(): ");
    } else if (ret > 0) {
      int sock_fd;
      for (sock_fd = 0; sock_fd < FD_SETSIZE; sock_fd++) {
        if (FD_ISSET(sock_fd, &test_set)) {
          if (sock_fd == listen_sock_fd) {
            conn_sock_fd = accept(listen_sock_fd,
                                  (struct sockaddr *)&client_address, &len);
            /* Set connection socket non-blocking too */
            flag = fcntl(conn_sock_fd, F_GETFL, 0);
            fcntl(listen_sock_fd, F_SETFL, flag | O_NONBLOCK);
            /* Get client address */
            inet_ntop(AF_INET6, &client_address.sin6_addr, str_addr,
                      sizeof(client_address));
            FD_SET(conn_sock_fd, &set);

          } else {
            memset(buffer, 0, 1000);
            memset(message,0,1022);
            if (strcmp(nicks[sock_fd], NOT_DEFINED) == 0) {
              read(sock_fd, &nick, sizeof(nick));
              printf("User: %s connected\n", nick);
              strcpy(nicks[sock_fd], nick);
              write(sock_fd, &nick, sizeof(nick));
            } else {
              if (read(sock_fd, &buffer, sizeof(buffer)) > 0) {
                strcpy(message, nicks[sock_fd]);
                strcat(message, ": ");
                strcat(message, buffer);
                int sock_others;
                for (sock_others = 4; sock_others < QUEUE_LEN + 4;
                     sock_others++) {
                  if (strcmp(nicks[sock_others],NOT_DEFINED)!=0 && sock_others != sock_fd ) {
                    write(sock_others, message, sizeof(message));
                    printf("Message from user: %s to: %s\n",nicks[sock_fd],nicks[sock_others]);
                  }
                }
              } else {
                strcpy(nicks[sock_fd], NOT_DEFINED);
                close(sock_fd);
                FD_CLR(sock_fd, &set);
              }
            }

          }
        }
      }
    } else {
    }
  }
  return EXIT_SUCCESS;
}
