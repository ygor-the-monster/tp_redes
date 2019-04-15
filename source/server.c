#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include"sds.h"
#include"sdsalloc.h"

sds folder;

char *tratahttp(char *menssagem_cliente, int client_sock);
int findFileSize(FILE *arq);
int file_exist (char *filename);
void lidaComHTTP(int sock);
static void *execucao_thread(void *arg);

int main(int argc , char *argv[]){
    //declaracao de variaveis
    int socket_desc, c;
    struct sockaddr_in server, client;

    folder = sdsnew(argv[1]);

    //cria um socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Nao foi possivel criar socket");
    }
    puts("Socket criado");

    //criacao do servidor
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    if (argc < 3){
      server.sin_port = htons(80);
    }else{
      server.sin_port = htons(atoi(argv[2]));
    }

    //ligando o socket ao servidor
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        perror("Erro");
        return 1;
    }
    puts("ligacao feita");

    //seta o socket para aceitar conexoes
    listen(socket_desc , 10);

    puts("Esperando por conexoes...");
    c = sizeof(struct sockaddr_in);


    int *client_sock;
    pthread_t threadID; //declaração da thread

    while(1){
        client_sock = (int *) malloc(sizeof(int));
        *client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c); /* iptr aceita a escuta do cliente */
        pthread_create(&threadID, NULL, &execucao_thread, client_sock);
    }

    return 0;
}

static void *execucao_thread(void *arg){
    int sock;

    sock = *((int *) arg);
    pthread_detach(pthread_self());
    lidaComHTTP(sock);
    close(sock);

    return NULL;
}

void lidaComHTTP(int sock){

    int read_size;
    char menssagem_cliente[2000];
    sds resposta;
    if( (read_size = recv(sock , menssagem_cliente , 2000 , 0)) <= 0 ){ //escuta no sock para receber msg do cliente
       perror("falha ao receber dados do cliente");
    }
    menssagem_cliente[read_size]='\0';
    printf("%s\n", menssagem_cliente);

    printf("------------------Resposta-----------------------\n");

    int tokens_size;
    int aux, fileSize;

    sds xablau = sdsnew(menssagem_cliente);
    sds *tokens = sdssplitlen(xablau, sdslen(xablau), " ", 1, &tokens_size);

    if(strcmp("GET", tokens[0]) != 0){
        resposta = sdsnew("HTTP/1.1 501 Not Implemented\r\n");
        send(sock, resposta, strlen(resposta), 0);
        return;
    }

    sds *tipo = sdssplitlen(tokens[2], sdslen(tokens[2]), "\r\n", 1, &aux);

    if(strcmp("HTTP/1.1", tipo[0]) != 0){
        resposta = sdsnew("HTTP/1.1 505 HTTP Version Not Supported\r\n");
        send(sock, resposta, strlen(resposta), 0);
        return;
    }
    sds fname = sdsdup(folder);
    fname = sdscatsds(fname, tokens[1]);

    printf("%s\n", fname);

    if (!file_exist(fname)){
       resposta = sdsnew("HTTP/1.1 404 Not Found\r\n");
       send(sock, resposta, strlen(resposta), 0);
       return;
    }

    FILE *arq = fopen(fname, "rb");
    fileSize = findFileSize(arq);

    resposta = sdsnew("HTTP/1.1 200 OK\r\n");

    resposta = sdscatprintf(resposta, "Content-Length: %d\r\n", fileSize);

    sds *type = sdssplitlen(tokens[1], sdslen(tokens[1]), ".", 1, &aux);

    if (strcmp(type[1], "html") == 0 || strcmp(type[1], "htm") == 0){
        resposta = sdscat(resposta, "Content-Type: text/html\r\n");
    }
    if (strcmp(type[1], "jpg") == 0 || strcmp(type[1], "jpeg") == 0){
        resposta = sdscat(resposta, "Content-Type: image/jpeg\r\n");
    }
    if (strcmp(type[1], "gif") == 0){
        resposta = sdscat(resposta, "Content-Type: image/gif\r\n");
    }
    if (strcmp(type[1], "png") == 0){
        resposta = sdscat(resposta, "Content-Type: image/png\r\n");
    }
    if (strcmp(type[1], "css") == 0){
        resposta = sdscat(resposta, "Content-Type: text/css\r\n");
    }
    if (strcmp(type[1], "au") == 0){
        resposta = sdscat(resposta, "Content-Type: audio/basic\r\n");
    }
    if (strcmp(type[1], "wav") == 0){
        resposta = sdscat(resposta, "Content-Type: audio/wav\r\n");
    }
    if (strcmp(type[1], "avi") == 0){
        resposta = sdscat(resposta, "Content-Type: video/x-msvideo\r\n");
    }
    if (strcmp(type[1], "mpeg") == 0 || strcmp(type[1], "mpg") == 0){
        resposta = sdscat(resposta, "Content-Type: video/mpeg\r\n");
    }
    if (strcmp(type[1], "mp3") == 0){
        resposta = sdscat(resposta, "Content-Type: audio/mpeg\r\n");
    }
    if (strcmp(type[1], "js") == 0){
        resposta = sdscat(resposta, "Content-Type: text/javascript\r\n");
    }
    if (strcmp(type[1], "ico") == 0){
        resposta = sdscat(resposta, "Content-Type: image/x-icon\r\n");
    }

    resposta = sdscat(resposta, "Connection: close\r\n\r\n");

    printf("%s\n", resposta);
    send(sock, resposta, strlen(resposta), 0);

    char *buffer = (char *)malloc(fileSize*sizeof(char));
    fread(buffer, 1, fileSize, arq);
    printf("%d\n", fileSize);

    send(sock, buffer, fileSize, 0);
    fclose(arq);
    free(buffer);

    printf("------------------------FIM--------------------------\n");

    return;
}


int findFileSize(FILE *arq){
    struct stat size;
    fstat(fileno(arq), &size);
    return size.st_size;
}

int file_exist (char *filename){
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}
