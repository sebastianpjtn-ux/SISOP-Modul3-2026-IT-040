#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 100

typedef struct{
    int sock;
    char name[50];
    int is_admin;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t lock;
pthread_mutex_t log_lock;
time_t start_time;

// Login
void write_log(const char* type, const char* category, const char* message){
    pthread_mutex_lock(&log_lock);

    FILE* f = fopen("history.log", "a");
    if(f){
        time_t now = time(NULL);
        struct tm* t = localtime(&now);

        char time_str[50];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

        fprintf(f, "[%s] [%s] [%s] %s\n", time_str, type, category, message);
        fclose(f);
    }

    pthread_mutex_unlock(&log_lock);
}

// UTIL
int is_name_taken(char* name){
    for(int i = 0; i < client_count; i++){
        if(strcmp(clients[i].name, name) == 0){
            return 1;
        }
    }
    return 0;
}

int check_admin(char* name, char* pass){
    FILE* f = fopen("admin.txt", "r");
    if(!f) return 0;

    char n[50], p[50];
    while(fscanf(f, "%s %s", n, p) != EOF){
        if(strcmp(n, name) == 0 && strcmp(p, pass) == 0){
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// message bisa dikirim ke semua orang diserver
void broadcast(char* message, int sender_sock){
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++){
        if(clients[i].sock != sender_sock){
            if(send(clients[i].sock, message, strlen(message), 0) < 0){
                perror("broadcast failed");
            }
        }
    }

    pthread_mutex_unlock(&lock);
}

// remove client
void remove_client(int sock){
    for(int i = 0; i < client_count; i++){
        if(clients[i].sock == sock){
            for(int j = i; j < client_count - 1; j++){
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
}

// login
void* handle_client(void* arg){
    int sock = *(int*)arg;
    free(arg);

    char buffer[1024], name[50];
    int is_admin = 0;

    int len = recv(sock, buffer, sizeof(buffer), 0);
    if(len <= 0){
        close(sock);
        return NULL;
    }

    buffer[len] = '\0';

    if(sscanf(buffer, "REGISTER %s", name) != 1){
        close(sock);
        return NULL;
    }

    // periksa nama
    pthread_mutex_lock(&lock);
    if(is_name_taken(name)){
        send(sock, "[System] name taken\n", 21, 0);
        pthread_mutex_unlock(&lock);
        close(sock);
        return NULL;
    }
    pthread_mutex_unlock(&lock);
    

    if(strcmp(name, "TheKnights") == 0){//Login admin
        send(sock, "Enter Password:\n", strlen("Enter Password:\n"), 0);

        len = recv(sock, buffer, sizeof(buffer), 0);
        if(len <= 0){
            close(sock);
            return NULL;
        }

        buffer[len] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;

        if(!check_admin(name, buffer)){
            send(sock, "Auth Failed\n", 12, 0);
            close(sock);
            return NULL;
        }

        is_admin = 1;
        send(sock, "[System] Authentication Successful\n", 34, 0);
    }

    // Tambah klien
    pthread_mutex_lock(&lock);

    if(client_count >= MAX_CLIENTS){
        pthread_mutex_unlock(&lock);
        close(sock);
        return NULL;
    }

    clients[client_count].sock = sock;
    strcpy(clients[client_count].name, name);
    clients[client_count].is_admin = is_admin;
    client_count++;

    pthread_mutex_unlock(&lock);

    // connect
    char logmsg[100];
    snprintf(logmsg, sizeof(logmsg), "%s connected", name);
    write_log("System", "Status", logmsg);

    // ===== WELCOME =====
    if(!is_admin){
        char welcome[100];
        snprintf(welcome, sizeof(welcome),
                 "--- Welcome to The Wired, %s ---\n", name);
        send(sock, welcome, strlen(welcome), 0);
    } else {
        char menu[] =
            "=== THE KNIGHTS ===\n"
            "/users\n/uptime\n/shutdown\n/exit\n";
        send(sock, menu, strlen(menu), 0);
    }

    // Sistem chat
    while(1){
        int r = recv(sock, buffer, sizeof(buffer), 0);

        if(r <= 0){
            snprintf(logmsg, sizeof(logmsg), "%s disconnected", name);
            write_log("System", "Status", logmsg);
            break;
        }

        buffer[r] = '\0';

        if(strncmp(buffer, "/exit", 5) == 0){
            snprintf(logmsg, sizeof(logmsg), "%s disconnected", name);
            write_log("System", "Status", logmsg);
            break;
        }

        // Kekuatan admin
        if(is_admin){
            if(strncmp(buffer, "/users", 6) == 0){
                write_log("Admin", "Command", "/users");

                int count = 0;
                pthread_mutex_lock(&lock);
                for(int i = 0; i < client_count; i++){
                    if(!clients[i].is_admin) count++;
                }
                pthread_mutex_unlock(&lock);

                char msg[50];
                sprintf(msg, "Active NAVI: %d\n", count);
                send(sock, msg, strlen(msg), 0);
                continue;
            }

            if(strncmp(buffer, "/uptime", 7) == 0){
                write_log("Admin", "Command", "/uptime");

                int sec = (int)(time(NULL) - start_time);
                char msg[50];
                sprintf(msg, "Uptime: %d sec\n", sec);
                send(sock, msg, strlen(msg), 0);
                continue;
            }

            if(strncmp(buffer, "/shutdown", 9) == 0){
                write_log("Admin", "Command", "/shutdown");
                printf("Shutdown by admin\n");
                exit(0);
            }
        }

        //broadcast ke server
        char message[1200];
        snprintf(message, sizeof(message), "[%s]: %s", name, buffer);

        broadcast(message, sock);
        write_log("User", "Chat", message);
    }

    pthread_mutex_lock(&lock);
    remove_client(sock);
    pthread_mutex_unlock(&lock);

    close(sock);
    return NULL;
}


int main(){
    int server_fd, client_sock;
    struct sockaddr_in server, client;
    socklen_t c = sizeof(client);

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&log_lock, NULL);
    start_time = time(NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0){
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 10);

    printf("Server running...\n");

    while(1){
        client_sock = accept(server_fd, (struct sockaddr*)&client, &c);

        if(client_sock < 0){
            perror("accept failed");
            continue;
        }

        pthread_t t;
        int* new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if(pthread_create(&t, NULL, handle_client, new_sock) != 0){
            perror("thread failed");
            close(client_sock);
            free(new_sock);
        }

        pthread_detach(t);
    }

    return 0;
}
