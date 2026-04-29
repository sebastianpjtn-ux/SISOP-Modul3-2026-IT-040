#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include "arena.h"

void menu() {
    printf("1. Register\n");
    printf("2. Login\n");
    printf("3. Exit\n");
    printf("4. Battle\n");
    printf("5. Armory\n");
    printf("6. History\n");
    printf("Choice: ");
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
     if(shmid == -1){
        printf("Orion are you there?\n");
        return 0;
    }
    SharedData *data = (SharedData*) shmat(shmid, NULL, 0);
    if (data == (void*) -1){
        perror("shmat gagal");
        return 1;
    }
    int msgid = msgget(MSG_KEY, 0666);
    if(msgid == -1){
        printf("Message queue tidak tersedia\n");
        return 0;
    }
  
    int choice;
    int logged = 0;

    while (1) {
        menu();
        scanf("%d", &choice);
        if(choice < 1 || choice > 6){
            printf("Pilihan tidak valid!\n");
            continue;
        }
        if (choice == 3){//exit
            break;
        }
        if((choice == 4 || choice == 5 || choice == 6) && !logged){//kalo user blm login, bakal di kick
            printf("Login dulu!\n");
            continue;
        }
        if(choice == 5){
            data->choice = 7; // mapping ke server
        }else if(choice == 6){
            data->choice = 8;
        }else{
            data->choice = choice;
        }
        
        
        if(choice == 1){//proses register
            printf("CREATE ACCOUNT\n");
        }else if(choice == 2){
            printf("LOGIN\n");
        }
        
        if(choice == 1 || choice == 2){
            printf("Username: ");
            scanf("%49s", data->username);
    
            printf("Password: ");
            scanf("%49s", data->password);

        }
        
        data->status = 1;

        while (data->status == 1){
            usleep(10000);
        }
        
        Message reply;
        msgrcv(msgid, &reply, sizeof(reply.text), 2, 0);//match ditemukan
        printf("%s\n", reply.text);
        if(choice == 2 && strstr(reply.text,"Login berhasil")){
            logged = 1;
        }
        
        if(data->battle.active){
            while(data->battle.active){
                printf("HP-mu: %d | Musuh: %d\n", data->battle.hp1, data->battle.hp2);
                
                for(int i = 0; i < 5; i++){
                    printf("%s\n", data->battle.logs[i]);
                }
                char cmd;
                scanf(" %c", &cmd);
                
                if(cmd == 'a'){
                    data->choice = 5;
                }
                else if(cmd == 'u'){
                    data->choice = 6;
                }else{
                    printf("Input tidak valid!\n");
                    continue;
                }
                
                data->status = 1;
                
                while(data->status == 1){
                    usleep(10000);
                }
                
                Message reply2;
                msgrcv(msgid, &reply2, sizeof(reply2.text), 2, 0);
                printf("%s\n", reply2.text);
                
            }
	}
        printf("Battle selesai!\n");
        printf("\n-------------------\n");
    }

    return 0;
}
