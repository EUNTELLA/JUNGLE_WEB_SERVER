#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3){ // 
        fprintf(stderr, "usage : %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); // <HOST> : <PORT> 에 BLOCKING CONNECT 수행 하고 소캣 FD 반환
    Rio_readinitb(&rio,clientfd); //버퍼 초기화 RIO_T 구조체(RIO)에 내부 읽기 버퍼를 붙 

    while(Fgets(buf,MAXLINE,stdin) != NULL){  //stdin(사용자 입력)에서 한 줄(\n까지)을 읽어서 buf에 저장.
        Rio_writen(clientfd,buf, strlen(buf)); //buf에 담긴 문자열을 소켓(=clientfd) 으로 보냄
                                            //일반 write()는 “짧은 쓰기(short count)”가 발생할 수 있는데, 
                                            //Rio_writen은 내부에서 반복 호출하여 요청한 바이트 수를 다 보낼 때까지 보장함.
        Rio_readlineb(&rio,buf,MAXLINE);
                                            //RIO 구조체 (버퍼포함) 를 이용해 소캐으로 한줄 읽어 BUF에 저장
                                            // 블로킹 호출 -> 서버가 응답 보내야만 리턴 됨
        Fputs(buf,stdout); // 읽어온 서버의 응답(BUF) 을 STOUT (화면) 에 출력
                            // 서버가 보낸 메시지 그대로 화면에 찍음
    }
    Close(clientfd);
    exit(0);
}