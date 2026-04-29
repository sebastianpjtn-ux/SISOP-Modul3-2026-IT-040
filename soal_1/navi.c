#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

void read_config(char* ip, int* port){//config supaya bisa diubah/update tanpa ubah source code
    FILE* file = fopen("config.txt", "r");
    if(!file){
        perror("config error");
        exit(1);
    }
    fscanf(file, "%s %d", ip, port);
    fclose(file);
}

int main(){
    int sock;
    struct sockaddr_in server;
    char ip[50];
    int port;
    
    read_config(ip, &port);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
        perror("Connection failed");
        return 1;
    }
    
    char name[50];
    printf("Enter your name: ");
    scanf("%s", name);
    
    char reg[100];
    sprintf(reg, "REGISTER %s", name);
    send(sock, reg, strlen(reg), 0);
    
    char buffer[1024];//respon server
    int len = recv(sock, buffer, sizeof(buffer), 0);
    if(len <= 0){
        printf("Server rejected\n");
        close(sock);
        return 0;
    }
    
    buffer[len] = '\0';
    printf("%s", buffer);
    
    if(strstr(buffer, "Enter Password")){
        char pass[50];
        
        while(getchar() != '\n');//mencegah \n dari masuk ke password
        fgets(pass, sizeof(pass), stdin);
        
        send(sock, pass, strlen(pass), 0);
        
        len = recv(sock, buffer, sizeof(buffer), 0);
        if(len <= 0){
            printf("Auth failed or server closed\n");
            close(sock);
            return 0;
        }
        buffer[len] = '\0';
        printf("%s", buffer);
    }    
    while(1){
        fd_set fds;//mode asinkronus
        FD_ZERO(&fds);
        
        FD_SET(0, &fds);
        FD_SET(sock, &fds);
        
        if(select(sock + 1, &fds, NULL, NULL, NULL) < 0){
            perror("select error");
            break;
        }
        
        if(FD_ISSET(0, &fds)){
            char msg [1024];
            fgets(msg, sizeof(msg), stdin);
            
            send(sock, msg, strlen(msg), 0);
            
            if(strncmp(msg, "/exit", 5) == 0){
                printf("[System] disconnecting...\n");
                break;
            }
        }
        
        if(FD_ISSET(sock, &fds)){
        int r = recv(sock, buffer, sizeof(buffer), 0);
            if(r<=0){
                printf("Disconnecting from server...\n");
                break;
            }
            buffer[r] = '\0';
            printf("%s", buffer);
        }
    }
    
    close(sock);
    return 0;
}
