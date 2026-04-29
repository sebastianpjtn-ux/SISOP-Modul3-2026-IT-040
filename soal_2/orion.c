#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include "arena.h"

char logged_in[100][50], waiting_user[50];
int logged_count = 0;
int waiting = 0;

int is_logged_in(const char *username){
    for(int i=0;i<logged_count;i++){
        if(strcmp(logged_in[i], username)==0) return 1;
    }
    return 0;
}

int getUserDataFull(const char *username, int *gold, int *lvl, int *xp, int *weapon){
    FILE *f = fopen("users.txt", "r");
    if(!f) return 0;

    char u[50], p[50];

    while(fscanf(f, "%[^:]:%[^:]:%d:%d:%d:%d\n", u, p, gold, lvl, xp, weapon) != EOF){
        if(strcmp(u, username) == 0){
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int user_exists(const char *username){
    int g,l,x,w;
    return getUserDataFull(username, &g,&l,&x,&w);
}

void register_user(const char *username, const char *password){
    FILE *f = fopen("users.txt", "a");
    if(!f) return;
    fprintf(f, "%s:%s:%d:%d:%d:%d\n", username, password, 150, 1, 0, 0);
    fclose(f);
}

int login_user(const char *username, const char *password, int *gold, int *lvl, int *xp){
    int weapon;
    FILE *f = fopen("users.txt", "r");
    if(!f) return 0;

    char u[50], p[50];

    while(fscanf(f, "%[^:]:%[^:]:%d:%d:%d:%d\n", u, p, gold, lvl, xp, &weapon) != EOF){
        if(strcmp(u, username)==0 && strcmp(p, password)==0){
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void update_user(const char *username, int win){
    FILE *f = fopen("users.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if(!f || !temp) return;

    char u[50], p[50];
    int gold,lvl,xp,weapon;

    while(fscanf(f, "%[^:]:%[^:]:%d:%d:%d:%d\n", u,p,&gold,&lvl,&xp,&weapon) != EOF){
        if(strcmp(u, username)==0){
            if(win){
                xp += 50;
                gold += 120;
            }else{
                xp += 15;
                gold += 30;
            }
            lvl = (xp/100)+1;
        }
        fprintf(temp,"%s:%s:%d:%d:%d:%d\n", u,p,gold,lvl,xp,weapon);
    }

    fclose(f);
    fclose(temp);
    remove("users.txt");
    rename("temp.txt","users.txt");
}

void buy_weapon(const char *username){
    FILE *f = fopen("users.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if(!f || !temp) return;

    char u[50], p[50];
    int gold,lvl,xp,weapon;

    while(fscanf(f,"%[^:]:%[^:]:%d:%d:%d:%d\n", u,p,&gold,&lvl,&xp,&weapon)!=EOF){
        if(strcmp(u,username)==0){
            if(gold>=100){
                gold-=100;
                weapon+=10;
            }
        }
        fprintf(temp,"%s:%s:%d:%d:%d:%d\n", u,p,gold,lvl,xp,weapon);
    }

    fclose(f);
    fclose(temp);
    remove("users.txt");
    rename("temp.txt","users.txt");
}

void save_history(const char *username, const char *opponent, int win){
    char filename[100];
    sprintf(filename, "history_%s.txt", username);

    FILE *f = fopen(filename, "a");
    if(!f) return;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    int xp = win ? 50 : 15;

    fprintf(f, "%02d:%02d|%s|%s|+%d\n",
            tm->tm_hour, tm->tm_min,
            opponent,
            win ? "WIN" : "LOSS",
            xp);

    fclose(f);
}

int main(){
    int gold,lvl,xp;

    int shmid = shmget(SHM_KEY,sizeof(SharedData),IPC_CREAT|0666);
    SharedData *data = shmat(shmid,NULL,0);

    if(data == (void*) -1) return 1;

    int msgid = msgget(MSG_KEY,IPC_CREAT|0666);

    data->status = 0;

    while(1){
        if(data->status == 1){
            Message reply;
            reply.type = 2;
            strcpy(reply.text, "");

            if(data->choice == 1){
                if(user_exists(data->username))
                    sprintf(reply.text,"Username sudah ada!");
                else{
                    register_user(data->username,data->password);
                    sprintf(reply.text,"Register berhasil!");
                }
            }

            else if(data->choice == 2){
                if(login_user(data->username,data->password,&gold,&lvl,&xp)){
                    if(is_logged_in(data->username))
                        sprintf(reply.text,"User sudah login!");
                    else{
                        if(logged_count < 100)
                            strcpy(logged_in[logged_count++], data->username);
                        sprintf(reply.text,"Login berhasil!\nGold:%d\nLvl:%d\nXP:%d",gold,lvl,xp);
                    }
                }else
                    sprintf(reply.text,"Login gagal!");
            }

            else if(data->choice == 4){
                if(waiting==0){
                    waiting=1;
                    strcpy(waiting_user,data->username);
                    sprintf(reply.text,"Mencari lawan...");
                    msgsnd(msgid,&reply,sizeof(reply.text),0);

                    sleep(35);

                    if(waiting == 1 && strcmp(waiting_user, data->username) == 0){
                        waiting = 0;

                        strcpy(data->battle.player1, data->username);
                        strcpy(data->battle.player2, "Monster");

                        data->battle.hp1 = 100;
                        data->battle.hp2 = 100;

                        data->battle.active = 1;
                        data->battle.log_index = 0;

                        sprintf(reply.text,"Tidak ada lawan. Melawan monster!");
                    }
                }else{
                    int g1,l1,xp1,w1;
                    int g2,l2,xp2,w2;

                    getUserDataFull(waiting_user,&g1,&l1,&xp1,&w1);
                    getUserDataFull(data->username,&g2,&l2,&xp2,&w2);

                    data->battle.hp1 = 100 + (xp1/10);
                    data->battle.hp2 = 100 + (xp2/10);

                    strcpy(data->battle.player1,waiting_user);
                    strcpy(data->battle.player2,data->username);

                    data->battle.active = 1;
                    data->battle.log_index = 0;

                    waiting = 0;

                    sprintf(reply.text,"Match ditemukan!");
                }
            }

            else if(data->choice == 5 || data->choice == 6){
                int g,l,xp_now,weapon;
                getUserDataFull(data->username,&g,&l,&xp_now,&weapon);

                int base = 10 + (xp_now/50) + weapon;
                int damage;

                if(data->choice == 6 && weapon > 0)
                    damage = base * 3;
                else
                    damage = base;

                sleep(1);

                if(strcmp(data->username,data->battle.player1)==0)
                    data->battle.hp2 -= damage;
                else
                    data->battle.hp1 -= damage;

                snprintf(data->battle.logs[data->battle.log_index%5],100,"%s hit %d dmg",data->username,damage);
                data->battle.log_index++;

                if(data->battle.hp1<=0 || data->battle.hp2<=0){
                    data->battle.active=0;

                    if(data->battle.hp1<=0){
                        update_user(data->battle.player2,1);
                        update_user(data->battle.player1,0);
                        save_history(data->battle.player2, data->battle.player1, 1);
                        save_history(data->battle.player1, data->battle.player2, 0);
                    }else{
                        update_user(data->battle.player1,1);
                        update_user(data->battle.player2,0);
                        save_history(data->battle.player1, data->battle.player2, 1);
                        save_history(data->battle.player2, data->battle.player1, 0);
                    }
                }

                sprintf(reply.text,"Attack!");
            }

            else if(data->choice == 7){
                buy_weapon(data->username);
                sprintf(reply.text,"Weapon dibeli!");
            }

            else if(data->choice == 8){
                FILE *f;
                char filename[100];
                sprintf(filename, "history_%s.txt", data->username);

                f = fopen(filename, "r");

                if(!f){
                    sprintf(reply.text, "Belum ada history.");
                } else {
                    char line[100];
                    char lines[5][100];
                    int count = 0;

                    while(fgets(line, sizeof(line), f)){
                        strcpy(lines[count % 5], line);
                        count++;
                    }
                    fclose(f);

                    if(count == 0){
                        sprintf(reply.text, "History kosong.");
                    } else {
                        strcpy(reply.text, "");
                        int start = (count > 5) ? count - 5 : 0;

                        for(int i = start; i < count; i++){
                            strncat(reply.text, lines[i % 5], sizeof(reply.text) - strlen(reply.text) - 1);
                        }
                    }
                }
            }

            msgsnd(msgid,&reply,sizeof(reply.text),0);
            data->status = 0;
        }

        usleep(10000);
    }
}
