# SISOP-Modul3-2026-IT-040

# Welcome to The Wired :)
  Dua anak, Mado dan Sunny, ingin membuat sebuah chat system. Mereka menamainya "The Wired", berdasarkan network komputer kesadaran manusia dari anime Serial Experiments Lain dengan nama yang sama. Di anime itu, warga bisa masuk ke dalam "simulasi" dimana mereka bisa bertemu bersama (mungkin tinggal di sana juga). Para warga Wired disebut NAVI. Mado dan Sunny berpura-pura sebagai organisasi yang membangun Wired tersebut.
  Agar akurat, dua program akan dipakai, 1 untuk Server dan Klien masing-masing.

## 1. Bikin Koneksi
  Chat system perlu server. Server punya 2 komponen utama, yaitu IP dan Port. Setelah itu, disambungkan ke Client.
```c
//Dari program client
void read_config(char* ip, int* port){//User akan disambungkan ke IP dan Port server. 
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
    
    read_config(ip, &port); -> Port dan id dibaca supaya disambungin
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){ -> mastiin internet user itu gak jelek
        perror("Connection failed");
        return 1;
    }
}

//Dari server
#define PORT 8080 //IP diatur dalam config.txt yang berada di dalam terminal
#define MAX_CLIENTS 100
```
<img width="612" height="52" alt="image" src="https://github.com/user-attachments/assets/2d584758-7d65-4513-9960-269064b8de09" />
ID dan Port server

## 2. Fungsi Asinkron untuk mendengar transmisi
  Klien bisa mengirim input dan mendapat respon dari server secara bersamaan. Mengapa Asinkronus? Ini supaya bisa kirim dan menerima pesanan bersamaan. Kalau tak Asinkronus, jika kamu menunggu pesan dari orang lain, gak bisa ngetik.
 ```c
//Dari Client
while(1){
        fd_set fds;//mode asinkronus
        FD_ZERO(&fds);
        
        FD_SET(0, &fds);// -> supaya dapat input keyboard
        FD_SET(sock, &fds); //-> "Colokan" sock
        
        if(select(sock + 1, &fds, NULL, NULL, NULL) < 0){
            perror("select error");
            break;
        }
        if(FD_ISSET(0, &fds)){// Untuk mengirim pesan 
            char msg [1024];
            fgets(msg, sizeof(msg), stdin);
            
            send(sock, msg, strlen(msg), 0);
            
            if(strncmp(msg, "/exit", 5) == 0){
                printf("[System] disconnecting...\n");
                break;
            }
        }
        
        if(FD_ISSET(sock, &fds)){//Sambungan ke server, jadi bisa nerima pesanan dari server maupun orang lain
        int r = recv(sock, buffer, sizeof(buffer), 0);
            if(r<=0){
                printf("Disconnecting from server...\n");
                break;
            }
            buffer[r] = '\0';
            printf("%s", buffer);
        }
}
 ```
## 3. Skalabilitas
  Server harus bisa menangani banyak klien, beragam jaringan internet(lemah). Serta, bisa menangani exit 
```c
//Dari server
//Untuk Exit
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
}

while(1){//Untuk banyak cleint
        client_sock = accept(server_fd, (struct sockaddr*)&client, &c);

        if(client_sock < 0){
            perror("accept failed");-
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
```

## 4. ID Please, No Doppelgangers
  Setiap warga yang ingin menjadi NAVI harus memasukkan identitas. Identitas tak boleh ada duplikat.
```c
//Dari Client
int main(){
  char name[50];
      printf("Enter your name: ");
      scanf("%s", name);
      
      char reg[100];
      sprintf(reg, "REGISTER %s", name);
      send(sock, reg, strlen(reg), 0);
}

//Dari Server
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
}
```

## 5. Group Chat
  Dari anime Lain, Wired adalah sistem kesadaran kolektif dimana "batas" antar individu dihilangkan dan kesadaran para NAVI di "sinkronisasi".
Artinya, The Wired adalah Group Chat terbesar di sejarah manusia. Mungkin yang paling menyeramkan juga.
```c
//Dari server
void broadcast(char* message, int sender_sock){//Fungsi untuk broadcast massal
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++){
        if(clients[i].sock != sender_sock){
            if(send(clients[i].sock, message, strlen(message), 0) < 0){
                perror("broadcast failed");
            }
        }
    }

void* handle_client(void *arg){
  char message[1200];
          snprintf(message, sizeof(message), "[%s]: %s", name, buffer);
  
          broadcast(message, sock);//Supaya chat dan lainnya bisa ke semua user
          write_log("User", "Chat", message);
  
      pthread_mutex_unlock(&lock);
}
```

## 6. Admin
  Mado menyarankan ke Sunny untuk membuat sebuah sistem Admin untuk mereka berdua. Karena mereka adalah penciptanya, kan?
```c
void* handle_client(void *arg){
  if(strcmp(name, "TheKnights") == 0){//Login admin
        send(sock, "Enter Password:\n", strlen("Enter Password:\n"), 0);

        len = recv(sock, buffer, sizeof(buffer), 0);//Kalo misalnya password gak diisi
        if(len <= 0){
            close(sock);
            return NULL;
        }

        buffer[len] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;

        if(!check_admin(name, buffer)){//kalau password salah
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
    clients[client_count].is_admin = is_admin;//klien baru yang masuk diperiksa kalau mereka memang admin
    client_count++;

    pthread_mutex_unlock(&lock);

    // connect
    char logmsg[100];
    snprintf(logmsg, sizeof(logmsg), "%s connected", name);
    write_log("System", "Status", logmsg);

    // ===== WELCOME =====
    if(!is_admin){//user biasa
        char welcome[100];
        snprintf(welcome, sizeof(welcome),
                 "--- Welcome to The Wired, %s ---\n", name);
        send(sock, welcome, strlen(welcome), 0);
    } else {// admin
        char menu[] =
            "=== THE KNIGHTS ===\n"
            "/users\n/uptime\n/shutdown\n/exit\n";
        send(sock, menu, strlen(menu), 0);
    }

  while(1){
    if(is_admin){//kekuatan admin
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
}
```
  Kekuatan admin:
1. Periksa jumlah NAVI yang aktif
2. Cek Uptime(lama aktif) Server
3. Matiin  server
4. Keluar

## 7. Catatan Log Chat
  Mado juga memberitahu Sunny untuk membuat sistem log, untuk mencatat semua pembicaraan yang ada. Juga supaya gak hilang di waktu.
 ```c
void write_log(const char* type, const char* category, const char* message){
    pthread_mutex_lock(&log_lock);

    FILE* f = fopen("history.log", "a");
    if(f){
        time_t now = time(NULL);
        struct tm* t = localtime(&now);

        char time_str[50];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);//Tanggal dan waktu tepat chat tersebut dibuat

        fprintf(f, "[%s] [%s] [%s] %s\n", time_str, type, category, message);
        fclose(f);
    }

    pthread_mutex_unlock(&log_lock);
}
 ```
<img width="807" height="538" alt="Chat history" src="https://github.com/user-attachments/assets/63ba6c39-90ce-4f90-b989-930ee7545efe" />
history.log dari terminal


## Kode Server
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
## Kode Client
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

## Foto The Wired
### Server berjalan
<img width="557" height="242" alt="Active server" src="https://github.com/user-attachments/assets/2c41025c-d91f-4d66-b5bc-c855e37ed84f" />

### Server mati
<img width="603" height="142" alt="Matiin server" src="https://github.com/user-attachments/assets/ce905bf5-8ff3-4529-8078-403382ab3e89" />

### Chat System
<img width="1076" height="617" alt="Chat system" src="https://github.com/user-attachments/assets/6ef286d0-d5e5-4806-b704-66b1ec22c64c" />

### Chat History
<img width="807" height="538" alt="Chat history" src="https://github.com/user-attachments/assets/12fa35fb-8569-44d4-98e6-b2886c3db8ec" />
Sunny sempat lupa screenshoot chat pertama mereka. Untungnya dicatat di sini.

### Admin
Masuk Sebagai Admin
<img width="1118" height="596" alt="Admin powers#3" src="https://github.com/user-attachments/assets/a2b4fb84-1316-4593-b99a-405cae7ac995" />
<img width="566" height="220" alt="Admin powers#2" src="https://github.com/user-attachments/assets/4201f3e0-3036-40b2-a91e-8f741da5fd1e" />

Kekuatan Admin dipakai
<img width="671" height="583" alt="Admin powers#1" src="https://github.com/user-attachments/assets/2b5f21e4-3a57-4626-8106-3ec6db9f3ce2" />
Keduanya lanjut bersenang-senang :D



# MOBIL LEGEN!11!! Main yUK!!!

    Setelah membuat sistem Wired, Sunny dan Mado, terinspirasi dari D&D, ingin mencoba membuat sistem game online. Gamenya akan text-based, karena mereka baru mulai. Mereka memutuskan untuk membuat Game Engine sendiri karena... kenapa tidak?
    Nama gamenya Eterion. Mereka akan roleplay sebagai karakter Pujo dan Eterion.

## 1. Arena Pertempuran
  Arena Pertempuran akan dibuat, bentuk awalnya seperti ini:
  <img width="537" height="142" alt="image" src="https://github.com/user-attachments/assets/1e12498b-4882-49c2-ac87-f9b9d553b6b8" />

  Di program, akan terlihat seperti ini:
```c

```

## 2. Register dan Login


## 3. Player nyambung ke server

## 4. Cek Username yang sudah ada dari Register & Login

## 5. Stat Awal

## 6. Matchmaking

## 7. Attack & Ultimate

## 8. Hasil Battle

## 9. Beli Senjata

## 10. Log Battle


