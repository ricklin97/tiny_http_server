#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define BUF_SIZE 8192

int startup(const int port);                // 创建套接字函数
void error_handling(char *msg);             // 错误处理函数
void *accept_request(void *arg);            // 相当于线程的main函数
void unimplemented(FILE *clnt_write);       // 处理未支持请求方式函数
void get_ct(char *file, char *ct);          // 获得content_type
void send_data(FILE *fp, char *ct, char *file_name);    // 处理Get请求，给客户端发送数据
void send_error(FILE *clnt_write);          // 处理错误请求
void not_found(FILE *fp);                   // 处理文件找不到

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in clnt_addr;
    int clnt_addr_sz;
    pthread_t pid;
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = startup(atoi(argv[1]));
    printf("server is running...\n");

    while (1){
        // 使用线程处理http请求
        clnt_addr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_sz);
        printf("Connection Request : %s:%d\n", inet_ntoa(clnt_addr.sin_addr), 
            ntohs(clnt_addr.sin_port));
        pthread_create(&pid, NULL, accept_request, &clnt_sock);
        pthread_detach(pid);
    }
    
    return 0;
}

int startup(const int port){
    int serv_sock;
    struct sockaddr_in serv_addr;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if(listen(serv_sock, 5) == -1)
        error_handling("listen() error");
    
    return serv_sock;
}

void *accept_request(void *arg){
    int clnt_sock = *((int*)arg);
    char req_line[BUF_SIZE];
    FILE *clnt_read, *clnt_write;

    char method[10];
    char ct[64];
    char file_name[255];
    char path[512]; 

    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    fgets(req_line, BUF_SIZE, clnt_read);

    if(!strstr(req_line, "HTTP/")){
        send_error(clnt_write);
        fclose(clnt_write);
        fclose(clnt_read);
        return NULL;
    }

    strcpy(method, strtok(req_line, " "));
    strcpy(file_name, strtok(NULL, " "));

    for(int i = 0; i < strlen(file_name); ++i){
        // 字体文件请求会有?v=4.7.0这样的结尾
        // 因此这段代码用来去掉
        if(file_name[i] == '?'){
            file_name[i] = '\0';
            break;
        }
    }

    get_ct(file_name, ct);

    // 文件放在htdocs里面
    if(file_name[0] == '/')
        strcpy(path, "htdocs");
    else
        strcpy(path, "htdocs/");
    strcat(path, file_name);

    if(strcasecmp(method, "GET") == 0){
        // strcasecmp()忽略大小写
        send_data(clnt_write, ct, path);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }else {
        // 该服务器暂时只支持GET
        unimplemented(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
}

void send_data(FILE *fp, char *ct, char *file_name){
    // fgets()/fputs()函数传不了图片，暂时用fgetc()/fputc()
    char buf[BUF_SIZE], c;
    FILE *send_file = fopen(file_name, "r");
    if(send_file == NULL){
        not_found(fp);
        return;
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    fputs(buf, fp);
    sprintf(buf, "Server:Linux Web Server\r\n");
    fputs(buf, fp);
    sprintf(buf, "Content-type:%s\r\n\r\n", ct);
    fputs(buf, fp);
    fflush(fp);

    while(1){
        c = fgetc(send_file);
        if(feof(send_file))
            break;
        fputc(c, fp);
    }
    fflush(fp);
    fclose(send_file);
}

void not_found(FILE *fp){
    char buf[BUF_SIZE];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    fputs(buf, fp);
    sprintf(buf, "Server:Linux Web Server\r\n");
    fputs(buf, fp);
    sprintf(buf, "Content-Type: text/html\r\n\r\n");
    fputs(buf, fp);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    fputs(buf, fp);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    fputs(buf, fp);
    sprintf(buf, "your request because the resource specified\r\n");
    fputs(buf, fp);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    fputs(buf, fp);
    sprintf(buf, "</BODY></HTML>\r\n");
    fputs(buf, fp);
    fflush(fp);
}

void unimplemented(FILE *clnt_write){
    char buf[BUF_SIZE];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "Server:Linux Web Server\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "Content-Type: text/html\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "</BODY></HTML>\r\n");
    fputs(buf, clnt_write);
    fflush(clnt_write);
}

void get_ct(char *file, char *ct){
    // 常用的几种后缀
    if(strstr(file, "html") || strstr(file, "htm")){
        strcpy(ct, "text/html");
        return;
    } else if(strstr(file, "css")){
        strcpy(ct, "text/css");
        return;
    } else if(strstr(file, "js")){
        strcpy(ct, "application/x-javascript");
        return;
    } else if(strstr(file, "jpg")){
        strcpy(ct, "image/jpeg");
        return;
    } else if(strstr(file, "png")){
        strcpy(ct, "image/png");
        return;
    } else if(strstr(file, "svg")){
        strcpy(ct, "text/xml");
        return;
    } else if(strstr(file, "gif")){
        strcpy(ct, "image/gif");
        return;
    }

    FILE *fp = fopen("content_type_form.txt", "r");
    char buf[64];
    while(fgets(buf, 64, fp) != NULL)
        if(strstr(buf, file)){
            strtok(buf, " ");
            strcpy(ct, strtok(NULL, " "));
            ct[strlen(ct) - 1] = '\0';
            fclose(fp);
            return;
        }
    fclose(fp);
}

void send_error(FILE *clnt_write){
    char buf[BUF_SIZE];

    sprintf(buf, "HTTP/1.0 400 Bad Request\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "Server:Linux Web Server\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "Content-length:2048\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "Content-type:text/html\r\n");
    fputs(buf, clnt_write);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    fputs(buf, clnt_write);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    fputs(buf, clnt_write);

    fflush(clnt_write);
}

void error_handling(char *msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}