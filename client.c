#include "csapp.c"
#include "csapp.h"

int main(int argc, char **argv) {
    /* argc : 메인 함수에 전달되는 정보의 갯수 , argv : 메인 함수에 전달되는 실질적인 정보로 문자열의 배열을 의미 */
    
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    
    host = argv[1];
    port = argv[2];
    
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }

    /* close를 반복적으로 수행하는 이유는 열었던 모든 식별자들을 명시적으로 닫아주는 것이 올바른 프로그래밍이기 때문임 */
    Close(clientfd);
    exit(0);
}