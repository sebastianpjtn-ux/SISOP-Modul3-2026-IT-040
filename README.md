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

