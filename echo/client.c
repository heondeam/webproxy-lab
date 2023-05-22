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

    int s, n, len_in, len_out;

    struct sockaddr_in server_addr;
    char *haddr;
    char buf[BUF_LEN + 1];

    if (argc != 2) {
        printf("usage: %s ip_address\n", argv[0]); 
        return -1; 
    }

    haddr = argv[1];

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("can't create socket\n"); 
        return -1; 
    }

    /* echo 서버의 소켓주소 구조체 작성 */
    // 구조체 변수의 메모리 영역을 0으로 채워서 초기화 한다. 유사 함수로는 memset()이 있다.
    bzero((char *)&server_addr, sizeof(server_addr)); 
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(haddr);
    server_addr.sin_port = htons(atoi("3000"));

    /* 연결 요청 */

    // connect() 함수를 비롯한 각종 소켓 함수의 정의에서는 
    // 일반적인 소켓주소 구조체인 sockaddr를 사용하도록 정의되어 있기 때문에 구조체 타입을 바꾸는 casting이 필요한 것이다.
    if(connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("can't connect.\n"); 
        return -1; 
    }

    /* 키보드 입력을 받음 */	

    printf("Input any string : ");
    while (fgets(buf, BUF_LEN, stdin) != NULL) {
        buf[128] = '\0'; 
        len_out = strlen(buf); 

        /* echo 서버로 메시지 송신 */	
        if (write(s, buf, len_out) < 0) { 
            printf("write error\n"); 
            return -1; 
        }         

        /* 수신된 echo 메시지 화면출력 */	
        printf("Echoed string : "); 

        for(len_in=0,n = 0; len_in < len_out; len_in += n) { 
            if((n = read(s, &buf[len_in], len_out - len_in)) < 0) { 
                printf("read error\n"); 
                return -1; 
            } 
        }

        printf("%s", buf); 
        close(s); 
    }

    close(s); 

    return 0;
}