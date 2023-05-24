#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#define BUFSIZE 65536
#define SEND_MESSAGE_BUFSIZE 1024
char *CURR_MY_PATH_ROOT;

void error_handler(char *message);
void request_handler(void *);
void get_handler(char *, char *, char *, int);
void post_handler(char *, char *, char *, int, char*);

int main(int argc, char **argv) {   
    /* 
        argc : 입력받은 문자열의 갯수, 무조건 1부터 시작 1: 현재 디렉터리 
        argv : 입력받은 문자열
    */
	CURR_MY_PATH_ROOT = getenv("PWD");

    int server_sock;
    int client_sock;
    char message[BUFSIZE];
    int str_len;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int client_addr_size;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 
        socket() 시스템콜 함수. 인자로 domain, type, protocol을 받아서 새로 만들어낸 소켓의 fd를 반환한다. (에러 발생시 -1을 리턴)

        1. domain : PF_INET(인터넷 프로토콜), PF_UNIX(UNIX 프로토콜), PF_NS(XEROX 프로토콜)
        2. type : SOCK_STREAM(TCP 방식의 소켓 생성), SOCK_DGRAM(UDP 방식의 소켓 생성)
        3. protocol : 0 입력 시에 시스템이 자동으로 설정해줌.
     */
    if((server_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        error_handler("socket() error occured!!\n");
    }

    /* 
        memset(void *ptr, int value, size_t num) 배열이나 구조체와 같은 메모리 블록을 초기화 하는데 사용된다. 
        <string.h> 에 선언되어 있다.
         
        1. ptr: 값을 설정할 메모리 블록의 시작 주소를 가리키는 포인터
        2. value: 설정할 값. int 형식으로 전달되어 unsigned char로 암시적 형변환 발생
        3. num: 설정할 바이트 수를 나타내는 size_t 형식의 정수

        즉, ptr부터 시작하는 num 바이트의 메모리 블록을 value로 설정한다. 그리고 ptr을 반환한다.
     */
    memset(&server_addr, 0, sizeof(server_addr));

    /* 
        소켓 구조체: 주소 체계(sin_family), IP 주소(sin_addr -> s.addr), port(sin_port)
     */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 주소로부터의 bind를 허용하기 위해 INADDR_ANY를 사용해서 0.0.0.0을 할당함
    server_addr.sin_port = htons(atoi(argv[1]));

    /* 
        int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) 시스템콜 함수. 소켓을 로컬 주소에 연결하는데 사용하는 함수. 서버쪽 프로그래밍에서 특정 IP 주소와 포트에 바인딩해서 
        클라이언트의 연결을 수신할 수 있도록 설정하는데 사용된다. 
        bind 성공 시에 0을 리턴하고 실패 시 -1을 리턴한다.

        1. sockfd : 바인딩 할 소켓의 파일 디스크립터
        2. addr : 로컬 소켓 주소 정보를 가리키는 struct sockaddr 구조체의 포인터
        3. addrlen : addr 구조체의 크기를 나타내는 socklen_t 형식의 값
     */
    if(bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        error_handler("bind() error occured!!\n");
    }

    /* 
        int listen(int sockfd, int backlog) 시스템콜 함수. 서버의 소켓을 수신 대기 상태로 설정하는 함수. bind로 바인딩 된 소켓을 사용해서
        클라이언트의 연결을 받아들일 수 있도록 사전 설정을 수행하는 데 사용된다.
        listen 성공 시 0을 리턴하고 실패 시 -1을 리턴한다.

        1. sockfd : 수신 대기 상태로 설정할 소켓 파일 디스크립터
        2. backlog : 대기열에 저장할 연결 요청의 최대 개수. 대기열은 아직 accept로 처리되지 않은 연결 요청을 보유하는 버퍼.
     */
    if(listen(server_sock, 5) == -1) {
        error_handler("listen() error occured!!\n");
    }

    /* 
        무한 서버 루프 실행
     */
    while (1) {
        printf("Server is listening....\n\n");

        client_addr_size = sizeof(client_addr);

        /* 
            int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) 시스템콜 함수.
            서버 소켓이 클라이언트의 연결을 수락하고 실제 통신에 사용할 소켓을 반환한다.
            성공 시 새로운 소켓 파일 디스크립터를 반환한다. 이 새로운 소켓은 클라이언트와의 통신에 사용됨.
            실패 시 -1을 반환함.

            1. sockfd : 연결 요청을 수락할 서버 소켓의 파일 디스크립터.
            2. addr : 클라이언트의 주소 정보를 저장할 struct sockaddr 구조체의 포인터. 여기에 클라이언트의 ip 주소와 포트 번호가 저장된다.
            3. addrlen : addr 구조체의 크기를 가리키는 socklen_t 타입의 포인터.
               이 때 addrlen은 호출 전에 구조체의 크기를 나타내고 호출 후에는 실제로 저장된 주소 구조체의 크기로 업데이트 된다.
         */
        client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_addr_size);

        if(client_sock == -1) {
            error_handler("accept() error occured!!\n");
            close(client_sock);
        }

        printf("Connection Request : %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        request_handler(&client_sock);

        close(client_sock);
        printf("Connect in down\n");
    }

    return 0;
}


void get_handler(char *V, char *message, char *U, int client) {
    int fd, str_len;
	char SEND_DATA[SEND_MESSAGE_BUFSIZE];
	char FIANL_PATH[BUFSIZE];
	char VERSION[10]="";
	char URL[SEND_MESSAGE_BUFSIZE]="";

    strcpy(VERSION, V);
	strcpy(URL, U);

    if(strncmp(VERSION, "HTTP/1.0", 8) != 0 && strncmp(VERSION, "HTTP/1.1", 8) != 0){
        /*  
            ssize_t write(int fd, const void *buf, size_t count) 시스템콜 함수
            주어진 파일 디스크럽터에 데이터를 입력한다. 
            쓰기 작업이 성공적으로 수행되면 쓴 바이트 수를 반환한다.
            쓰기 작업 중 오류가 발생한 경우 -1을 반환하고, 오류 정보는 errno 변수에 저장된다.

            1. fd : 파일 디스크립터 (정수 값), 쓰기 작업을 수행할 대상 파일이나 소켓을 가리킨다.
            2. buf : 입력할 데이터가 있는 버퍼의 포인터. 보통 문자열이나 데이터 버퍼를 전달한다.
            3. count : 입력할 데이터의 크기를 나타내는 바이트 수(size_t). 버퍼에 있는 데이터 중에서 몇 바이트를 쓸 것인지 지정.
        */
		write(client, "HTTP/1.1 400 Bad Request\n",25);
	}

    if(strlen(URL) == 1 && !strncmp(URL, "/",1)) {
        strcpy(URL, "/index.html");
    }

	strcpy(FIANL_PATH, CURR_MY_PATH_ROOT);
	strcat(FIANL_PATH, URL);

	if((fd = open(FIANL_PATH, O_RDONLY)) != -1){
		send(client, "HTTP/1.0 200 OK\n\n", 17, 0);
		while(1){

            /* 
                ssize_t read(int fd, void *buf, size_t count)
                파일 디스크립터로부터 데이터를 읽어오는 데 사용되는 함수.

                1. fd : 데이터를 읽을 파일 디스크립터.
                2. buf : 읽어온 데이터를 저장할 버퍼의 포인터.
                3. count : 읽어올 데이터의 최대 길이.
             */
			str_len = read(fd, SEND_DATA, SEND_MESSAGE_BUFSIZE);
			
            if(str_len <= 0) {
                break;
            }

			write(client, SEND_DATA, str_len);
		}
	}else {
		write(client, "HTTP/1.1 404 Not Found\n", 23);
	}    
};

void post_handler(char *V, char *message, char *U, int client, char *PI) {
	int fd, str_len;
	char FIANL_PATH[BUFSIZE];
	char VERSION[10]="";
	char URL[SEND_MESSAGE_BUFSIZE]="";
	char HTML_DATA[BUFSIZE];
	
	strcpy(VERSION, V);
	strcpy(URL, U);
	
    /*
        int sprintf(char *str, const char *format, ...) 형식화된 문자열을 str 버퍼에 저장한다.
     */
	sprintf(HTML_DATA, "<!DOCTYPE html><html><body><h2>%s</h2></body></html>", PI);
	
	if(strncmp(VERSION, "HTTP/1.0",8) != 0 && strncmp(VERSION, "HTTP/1.1",8) != 0){
		write(client, "HTTP/1.1 400 Bad Request\n",25);
	}
	else{

        /* 
            ssize_t send(int sockfd, const void *buf, size_t len, int flags)
            소켓을 통해 데이터를 전송하는 데 사용되는 함수.

            1. sockfd : 데이터가 보낼 소켓의 파일 디스크립터.
            2. buf : 전송할 데이터가 저장된 버퍼의 포인터.
            3. len : 전송할 데이터의 길이
            4. flags : 전송 동작에 영향을 주는 특정 플래그 주로 0 OR 특정 플래그 상수 사용.
         */
		send(client, "HTTP/1.1 200 OK\n\n", 17, 0);
		write(client, HTML_DATA, strlen(HTML_DATA));
	}
};


void request_handler(void *arg) {
    char msg[BUFSIZE];
    char *firstLine[3];
    int sd = * (int *) arg;
    int received_msg_len;

    /* 
        ssize_t recv(int sockfd, void *buf, size_t len, int flags) 시스템콜 함수.
        소켓으로부터 데이터를 읽어온다. 일반적으로 TCP 소켓에서 사용된다.

        성공적으로 데이터를 읽어왔을 경우에 읽은 데이터를 buf에 저장하고, 읽은 데이터의 길이를 반환한다.
        만약 연결이 종료되었을 경우 0을 반환한다. (클라이언트가 소켓을 닫았거나 에러 발생 시)
        오류가 발생한 경우 -1을 반환한다.

        1. sockfd : 읽어올 데이터를 갖고 있는 소켓의 파일 디스크립터.
        2. buf : 읽어온 데이터를 저장할 버퍼의 포인터.
        3. len : 읽어올 데이터의 최대 길이
        4. flags : 읽기 동작에 영향을 주는 특정 플래그. 주로 0 또는 특정 플래그 상수를 사용한다.
    
     */
    if((received_msg_len = recv(sd, msg, BUFSIZE-1, 0)) <= 0) {
        error_handler("recv() error occured!!\n");
    }

	printf("-----------Request message from Client-----------\n");
	printf("%s",msg);
	printf("-------------------------------------------------\n");

	char post_information[SEND_MESSAGE_BUFSIZE];
	char *curr_msg = NULL;
	char METHOD[4] = "";
	char VERSION[10] = "";
	char URL[SEND_MESSAGE_BUFSIZE] = "";

    /* 
        char *strtok(char *str, const char *delim) 함수는 문자열을 특정 구분자를 기준으로 토큰으로 분리한다.
        그리고 분리된 토큰을 가리키는 포인터를 반환한다. 
        분리된 토큰이 없는 경우 NULL을 반환한다.

        1. str : 분리할 대상 문자열. 첫 번째 호출 시에만 사용하고 이후 호출 시에는 NULL을 전달해야함.
        2. delim : 토큰으로 분리할 구분자이다 여러 개의 구분자를 지정할 수 있으며 문자열 형태로 전달된다.
     */
    curr_msg = strtok(msg, "\n");

    int line_count = 1;

    while (curr_msg) {
        if(line_count >= 15) {
            strcpy(post_information, curr_msg);
            curr_msg = strtok(NULL, "\n");
            line_count++;
        }
    }

    firstLine[0] = strtok(msg, " \t\n");
	firstLine[1] = strtok(NULL, " \t");
	firstLine[2] = strtok(NULL, " \t\n");
	
    /* 
        char *strcpy(char *dest, const char *src) 함수는 문자열을 복사하는 함수이다.

        1. dest : 복사된 문자열이 저장될 대상 버퍼의 포인터.
        2. src : 복사할 원본 문자열의 포인터
     */
	strcpy(METHOD, firstLine[0]);
	strcpy(URL, firstLine[1]);
	strcpy(VERSION, firstLine[2]);


    /*
        int strncmp(const char *str1, const char *str2, size_t n) 함수는 두 개의 문자열을 비교하는 함수이다.
        주어진 두 문자열의 각 문자를 순서대로 비교하면서 동일한지 확인한다. 

        str1과 str2의 첫 번째 문자를 비교한다.
        비교 결과가 같으면 다음 문자를 비교한다.
        비교 결과가 다른 경우 해당 문자의 아스키 값 차이를 반환한다.
        비교 결과가 모두 같고 n개의 문자를 비교한 경우 or 두 문자열이 동일한 경우 0을 반환한다.

        리턴값이 양수 - 첫 번쨰 문자열이 더 큼
        리턴값이 음수 - 두 번째 문자열이 더 큼 

        1. str1 : 첫 번째 문자열의 포인터
        2. str2 : 두 번쨰 문자열의 포인터
        3. n : 비교할 문자의 최대 개수 또는 비교할 길이
     */
    if(!(strncmp(METHOD, "GET", 3))) {
        get_handler(VERSION, msg, URL, sd);
    }else if(!(strncmp(METHOD, "POST", 4))) {
        post_handler(VERSION, msg, URL, sd, post_information);
    }

    /* 
        int shutdown(int sockfd, int how) 시스템콜 함수는 네트워크 소켓을 닫는 데 사용되는 함수이다.
        이 함수는 특정 소켓 연결을 종료하거나 소켈을 읽기 또는 쓰기 가능한 상태로 유지할 지 설정할 수 있다.
        close() 함수와 같이 사용해서 소켓을 완전히 닫아야한다.

        1. sockfd : 연결을 종료할 소켓의 파일 디스크립터
        2. how : 연결 종료 방식을 지정하는 인자. "SHUT_RD" , "SHUT_WR" , "SHUT_RDWR"
     */
    shutdown(sd, SHUT_RDWR);
    close(sd);
}

void error_handler(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}