#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
  int sockfd, portno = 80;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char *url, *arq;
  FILE *fp = NULL;

  if (argc >= 3) portno = atoi(argv[2]);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
     fprintf(stderr, "Erro ao abrir socket\n");
     exit(1);
  }

  url = argv[1] + + strlen("http:/") + 1;
  arq = argv[1] + strlen("http:/") + 1;
  while (*arq != '/')
    arq++;

  *arq = '\0';
  arq++;

  server = gethostbyname(url);
  if (server == NULL) {
      fprintf(stderr,"url não encontrada\n");
      exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
     fprintf(stderr, "Erro ao conectar\n");
     exit(1);
  }

  int n = snprintf(NULL, 0, "GET /%s HTTP/1.1\r\n\r\n", arq);
  char * buffer = (char *) malloc(n + 1);
  sprintf(buffer, "GET /%s HTTP/1.1\r\n\r\n", arq);

  if ((write(sockfd, buffer, n)) < 0){
     fprintf(stderr, "Erro ao enviar request\n");
     exit(1);
  }

  free(buffer);
  buffer = (char *) malloc(strlen("HTTP/1.1 000") + 500);

  int status = -1;
  int writing = 0;
  while((n = read(sockfd, buffer, strlen("HTTP/1.1 000") + 500))){
    if (n < 0) {
       fprintf(stderr, "Erro ao ler\n");
       exit(1);
    }

    int processed = 0;

    if(status == -1) {
      while (buffer[processed] != ' ' && processed < n)
        processed++;
      processed++;

      status = atoi(buffer + processed);

      while (buffer[processed] != ' ' && processed < n)
        processed++;
      processed++;

    }

    if(processed < n){
      if(status != 200) {
        printf("Erro %d ", status);
        while (buffer[processed] != '\r' && processed < n) {
          printf("%c", buffer[processed]);
          processed++;
        }

        if(buffer[processed] == '\r'){
          printf("\n");
          exit(1);
        }
      } else {
        while (writing < 4 && processed < n) {
          if(writing == 0) {
            while (buffer[processed] != '\r' && processed < n) {
              processed++;
            }
            if(buffer[processed] == '\r'){
              processed++;
              writing++;
            }
          }

          if(writing == 2) {
            if (processed < n) {
              if(buffer[processed] == '\r'){
                processed++;
                writing++;
              } else {
                writing = 0;
              }
            }
          }

          if(writing == 1 || writing == 3) {
            if (processed < n) {
              if(buffer[processed] == '\n'){
                processed++;
                writing++;
              } else {
                writing = 0;
              }
            }
          }
        }

        while (processed < n) {
          if(fp == NULL){

            fp = fopen(arq, "wb");
            if (fp == NULL){
              fprintf(stderr, "Erro ao enviar abrir arquivo\n");
              exit(1);
            }
          }

          fputc(buffer[processed], fp);
          processed++;
        }
      }
    }
  }

  free(buffer);
  if(fp != NULL) fclose(fp);

  return 0;
}
