#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUF_LEN 128 

int main(int argc, char *argv[]) {
    /* 
        argc : 메인 함수에 전달되는 정보의 갯수 ,
        argv : 메인 함수에 전달되는 실질적인 정보로 문자열의 배열을 의미. 첫번째 문자열은 프로그램 실행경로임.
    */

    struct sockaddr_in server_addr, client_addr; 
    int server_fd, client_fd; /* 소켓번호 */ 
    int len, len_out; 
    int port; /* 포트번호 */ 
    char buf[BUF_LEN+1];

    if(argc != 2) { 
        printf("usage: %s port\n", argv[0]); 
        return -1; 
    }

    port = atoi(argv[1]); /* 포트번호는 명령 인자로 입력 */ 

    /* 소켓 생성 */ 
    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("Server: Can't open stream socket."); 
        return 0; 
    } 

    /* server_addr을 '\0'으로 초기화 */ 
    bzero((char *)&server_addr, sizeof(server_addr)); 

    /* server_addr 세팅 */ 
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(port); 

    /* bind() 호출 */ 
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { 
        printf("Server: Can't bind local address.\n"); 
        return 0; 
    } 

    /* 소켓을 수동 대기모드로 세팅 */ 
    listen(server_fd, 5); 

    printf("Server : waiting connection request.\n"); 
    len = sizeof(client_addr); 

    /* 연결요청을 기다림 */ 
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len); 

    if(client_fd < 0) { 
        printf("Server: accept failed.\n"); 
        return 0; 
    }else {
        printf("Server : client connected.\n"); 
    }

    /* iterative echo 서비스 수행 */ 
    while(1) { 
        len_out = read(client_fd, buf, sizeof(buf)); 
        write(client_fd, buf, len_out); 
        // close(client_fd); 
    } 

    close(server_fd); 

    return 0;
}