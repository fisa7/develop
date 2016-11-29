#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>

#define SERVICE "7777"

int main(int argc, char const *argv[]) {

  int sock_fd;
  struct sockaddr_in6 server_addres;
  int ret;
  struct addrinfo hints, *result, *rp;
  char str_addr[INET6_ADDRSTRLEN];
  int len6 = sizeof(struct sockaddr_in6);
  int len4 = sizeof(struct sockaddr_in);
  char nick[20];
  char buffer[1022];

  setlocale(LC_CTYPE,"en_US.UTF-8");

  if (argc != 3) {
    printf("Syntax: %s server_address nick_name\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (strlen(argv[2]) > 30) {
    printf("Nick length exceeded! (max 20 letters)\n");
    return EXIT_FAILURE;
  } else {
    strcpy(nick, argv[2]);
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Allow stream protocol */
  hints.ai_flags = 0;              /* No flags required */
  hints.ai_protocol = IPPROTO_TCP; /* Allow TCP protocol only */

  if ((ret = getaddrinfo(argv[1], SERVICE, &hints, &result)) != 0) {
    perror("getaddrinfo(): ");
    return EXIT_FAILURE;
  }

  /* Try to use addrinfo from getaddrinfo() */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (rp->ai_family == AF_INET) {
      if (inet_ntop(AF_INET, &((struct sockaddr_in *)rp->ai_addr)->sin_addr,
                    str_addr, len4) != NULL)
        printf("Trying IPv4 address: %s:%s ...\n", str_addr, SERVICE);
      else
        printf("Not valid IPv4 address.\n");
    } else if (rp->ai_family == AF_INET6) {
      if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr,
                    str_addr, len6) != NULL)
        printf("Trying IPv6 address: [%s]:%s ...\n", str_addr, SERVICE);
      else
        printf("Not valid IPv6 address\n");
    }

    /* Do TCP handshake */
    if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    else
      printf("Failed\n");
    close(sock_fd);
    sock_fd = -1;
  }
  if (rp == NULL) {
    printf("Could not connect to the [%s]:%s\n", argv[1], SERVICE);
    freeaddrinfo(result);
    return EXIT_FAILURE;
  }

  printf("Connected\n");

  write(sock_fd, nick, sizeof(nick));
  read(sock_fd, &buffer, sizeof(buffer));
  printf("Connected as: %s\n",
         buffer);

  while (1) {
    char *line = NULL;
    size_t size = 0;
    size_t pocet_znaku = 0;
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(sock_fd, &rfds);
    FD_SET(0,&rfds);
    int retval = 1;

      memset(buffer, 0, 1000); 
      retval = select(4, &rfds, NULL, NULL, &tv);

      if (retval == -1)
        perror("select()");
      else if (retval > 0) {
               if (FD_ISSET(0, &rfds)){
                   free(line);
                 if ((pocet_znaku = getline(&line, &size, stdin)) == -1) {

                 } else {
                   if (pocet_znaku > 1000) {
                     printf("Message length exceeded! (max 1000 chars)\n");
                   } else {
                     if (strlen(line)>1)
                       write(sock_fd, line, pocet_znaku);
                   }
                 }
               }
               if (FD_ISSET(sock_fd, &rfds)){
                 read(sock_fd, &buffer, sizeof(buffer));
                 printf("%s", buffer);
               }
      }
  }

  close(sock_fd);
  return EXIT_SUCCESS;
}
