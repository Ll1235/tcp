#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <fcntl.h>

using namespace std;

#define PORT 1234
#define MAX_SIZE 10000
#define BUF_SIZE 1024

map <int,string> room;
map <int,string> login;
map <int,string>::iterator it;

int sendall(int s,const char *buf,int len, int flags){

    int total = 0;
    int n;

    while(total < len) {
        int n = send(s, buf + total, len - total, 0);
        if (n == -1) { break; }
        total += n;
    } return (n == -1 ? -1 : total);
}

int set_non_blocking(int fd)
{
    int flags;

    if((flags = fcntl(fd,F_GETFL,0)==-1))
        return -1;
    flags |= O_NONBLOCK;
    if (fcntl(fd,F_SETFL,0)==-1)
        return  -1;
    return 0;
}

int close_client(int s){

    if(close(s) == -1 ){
        perror("error close");
        exit(EXIT_FAILURE);
    }
    room.erase(s);
    login.erase(s);
}


int main(){

    char buf[BUF_SIZE];
    string your_room = "\nВведите название комнаты\n";
    string your_login = "\nВведите имя пользователя\n";

    int Sock = socket(AF_INET,SOCK_STREAM,0);
    if(Sock == -1)
    {
        perror("error sock");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

  if(bind(Sock,(struct sockaddr*) &addr,sizeof addr)==-1)
  {
      perror("error bind");
      exit(EXIT_FAILURE);
  }

  listen(Sock,SOMAXCONN);
  set_non_blocking(Sock);

  int Epoll = epoll_create(1);

  struct epoll_event Event;
  Event.data.fd = Sock;
  Event.events = EPOLLIN | EPOLLET;

    if(epoll_ctl(Epoll,EPOLL_CTL_ADD,Sock,&Event)==-1)
    {
        perror("error ctl");
        exit(EXIT_FAILURE);
    }

    while(1){

        struct epoll_event events[MAX_SIZE];
        int count = epoll_wait(Epoll,events,MAX_SIZE, -1);

        for (int i = 0; i < count; i++){
            if(events[i].data.fd == Sock){

                int SlaveSock = accept(Sock,0,0);

                set_non_blocking(SlaveSock);

                Event.data.fd = SlaveSock;
                Event.events = EPOLLIN | EPOLLET;

                if(epoll_ctl(Epoll,EPOLL_CTL_ADD,SlaveSock,&Event) ==-1)
                {
                    perror("errror ctl");
                    exit(EXIT_FAILURE);
                }
                if(sendall(SlaveSock,your_room.c_str(),your_room.length()+1,0)==-1){
                    close_client(SlaveSock);
                }
            } else {

                int SlaveSock = events[i].data.fd;

                if(events[i].events & EPOLLIN){
                    int len = recv(SlaveSock,buf,BUF_SIZE,0);

                    string mes(buf);
                    mes = mes.substr(0,len);

                    if (len == 0)
                    {
                        close_client(SlaveSock);
                    }
                    if (room.count(SlaveSock) == 0){

                        room[SlaveSock] = mes.substr(0,mes.length()-2);

                        if(sendall(SlaveSock,your_login.c_str(),your_login.length()+1,0)== -1){
                            close_client(SlaveSock);
                        }
                        cout << "регистрация в комнате :" << room[SlaveSock] << endl;
                    } else
                    {
                        if (login.count(SlaveSock) == 0){
                            login[SlaveSock] = mes.substr(0,mes.length()-2);
                            cout << "регистрация пользователя : " << login[SlaveSock] << endl;

                        } else{

                            cout << "сообщение от пользователя: " << login[SlaveSock] << " в комнате > "
                            << room[SlaveSock] << " >> " << mes;
                            string send = login[SlaveSock] + " >> " + mes;

                            for(it = room.begin(); it != room.end(); ++it){

                                if (it-> second == room[SlaveSock] && it-> first != SlaveSock){

                                    if(sendall(it->first,send.c_str(),send.length()+1,0)==-1){
                                        close_client(it->first);
                                    }
                                }
                            }
                        }
                    }

                }else if (events[i].events & EPOLLHUP | EPOLLET){
                    close_client(SlaveSock);
                    exit(0);
                }
            }
        }
    }
}
