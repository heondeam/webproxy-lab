/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void serve_dynamic(int fd, char *filename, char *cgiargs , char *method);
void get_filetype(char *filename, char *filetype);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/**
 * 요청에 대한 오류 응답을 전달하기 위한 함수
 * @param fd 현재 연결중인 클라이언트 소켓의 fd
 * @param cause 오류의 원인을 설명하는 문자열
 * @param errnum 오륲 상태 코드 문자열
 * @param shortmsg 간단한 오류 메시지 문자열
 * @param longmsg 상세한 오류 메시지 문자열
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  // 요청에 대한 오류 응답을 전달하기 위해 적절한 상태 코드와 상태 메세지를 응답에 담아서 보낸다.
  // 또한 response body에 브라우저 사용자에게 에러를 설명하는 HTML 파일도 함께 보낸다.
  // 이 때 HTMl 응답은 body에서 컨텐츠 크기와 타입을 나타내야 한다는 것을 기억하자.

  /* response body */
  sprintf(body, "<html><title>Tiny error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, buf, strlen(body));
};

/**
 * 요청 헤더 읽기
 * @param rp 요청을 읽기 위한 rio_t 구조체의 포인터 (Rio_readlineb 읽는데에 필요.)
*/
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  
  /* 
      int strcmp(const char* str1, const char* str2)
      str1과 str2 를 비교해서 완전히 같다면 0을 반환하고 다르다면 음수 또는 양수를 반환한다.
   */
  while(strcmp(buf, "\r\n")) {
    // 헤더 한 줄씩 읽고 출력함.
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }

  return;
};

/**
 * uri 파싱
 * @param uri 분석할 uri 문자열
 * @param filename 변환된 파일 경로를 저장할 문자열
 * @param cgiargs CGI 인자를 저장할 문자열
*/
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if(!strstr(uri, "cgi-bin")) {
    // 정적 컨텐츠일 경우 URI 분석
    
    // cgi 문자열을 빈 문자열로 초기화
    strcpy(cgiargs, "");    

    // filename에 ./를 추가해서 상대 경로로 변환한다.
    strcpy(filename, ".");   
    strcat(filename, uri);


    // 만약 uri가 "/" 로 끝난다면 filename에 index.html을 추가한다.
    if(uri[strlen(uri) - 1] == '/') {
      strcat(filename, "index.html");
    }

    // 만약 uri가 "/adder" 라면 filename에 adder.html을 추가한다. 
    if(strstr(uri, "/adder")) {
      strcat(filename, ".html");
    }

    return 1;
  }else {
    // 동적 컨텐츠일 경우 URI 분석

    // uri에서 '?'문자를 찾는다.
    ptr = index(uri, '?');
    
    // '?' 문자 기준으로 CGI 인자들을 추출한다.
    if(ptr) {
      // 인자들이 존재한다? -> '?' 이후의 문자열을 잘라내서 cgiargs에 복사함.
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }else {
      // 인자들이 존재하지 않는다? -> cgiargs를 빈문자열로 초기화한다.
      strcpy(cgiargs, "");
    }

    // filename에 ./를 추가해서 상대 경로로 변환한다.
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }
};

/**
 * 파일 타입 반환
 * @param filename 파일 이름을 나타내는 문자열
 * @param filetype 결정된 파일 타입을 저장할 문자열
*/
void get_filetype(char *filename, char *filetype) {
  // strstr 함수를 사용해서 주어진 파일 이름에 특정 확장자가 있는지 확인하고, MIME 타입을 설정한다.
  if(strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }else if(strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }else if(strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }else if(strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }else if(strstr(filename, ".mpg")) {
    strcpy(filetype, "video/mpg");
  }else {
    // 지원하는 확장자가 없는 경우에 기본적으로 text/plain으로 filetype을 설정한다.
    strcpy(filetype, "text/plain");
  }
};

/**
 * 정적 컨텐츠 처리 함수
 * @param fd 클라이언트와 연결된 소켓의 파일 디스크립터
 * @param filename 제공할 정적 파일의 경로, 이름
 * @param filesize 제공할 파일의 크기를 나타내는 정수값
*/
void serve_static(int fd, char *filename, int filesize, char *method) {
  /* html, 무형식 텍스트, GIF, PNG, JPEG, MPG의 정적 컨텐츠 지원 */
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // send response headers to client

  // filename에 기반한 filetype 설정
  get_filetype(filename, filetype);

  // 응답 헤더 생성
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  
  // 응답 헤더 클라이언트에 전송
  Rio_writen(fd, buf, strlen(buf));

  // 서버 측에도 응답 헤더 출력.
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD 메서드는 응답 헤더까지만 처리한다.

  if(!strcasecmp(method, "GET")) {
    /* send response body to client */

    // 응답 파일을 열고 해당 파일 디스크립터를 변수에 저장함.
    srcfd = Open(filename, O_RDONLY, 0);

    /* Mmap & Munmap 사용 */
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // Munmap(srcp, filesize);

    // Malloc을 사용해서 현재 filesize만큼의 메모리를 할당받아 srcp에 시작 포인터를 저장한다.
    srcp = (char *)Malloc(filesize);

    // 파일을 읽는다.
    Rio_readn(srcfd, srcp, filesize);
    // 파일을 보낸다.
    Rio_writen(fd, srcp, filesize);
    // 연결을 종료한다.
    Close(srcfd);

    // 할당된 메모리를 해제한다.
    Free(srcp);
  }
};

/**
 * 동적 컨텐츠의 처리
 * @param fd
 * @param filename
 * @param cgiargs
 * @param method
*/
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  // 응답 헤더 클라이언트에게 전송
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));


  // 자식 프로세스를 생성한다.
  if (Fork() == 0) {
    /* Child */
    /* Real server would set all CGI vars here */
    
    // 자식 프로세스에서 CGI(Common Gateway Interface) 프로그램을 실행하기 위해 필요한 환경 변수를 설정한다.
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);

    // 표준 출력을 클라이언트 소켓의 파일 디스크립터로 복제하여 CGI 프로그램의 출력이 클라이언트로 전송되도록 한다.
    Dup2(fd, STDOUT_FILENO); // redirect stdout to client

    // CGI 프로그램을 실행한다. 
    // filename : 실행할 프로그램의 경로
    // emptylist : 인수로 전달할 명령 인수 배열
    // environ : 환경 변수의 배열
    Execve(filename, emptylist, environ); // Run CGI prigram
  }

  // 부모 프로세스는 자식 프로세스가 종료될 때까지 기다린다.
  Wait(NULL); 
};

/**
 * 한 개의 트랜잭션(HTTP 요청 -> HTTP 응답)을 처리하는 doit함수 
 * @param fd 클라이언트가 연결된 소켓의 파일 디스크립터
*/
void doit(int fd) {
  // 정적 컨텐츠 여부를 나타내는 플래그
  int is_static;
  // stat 함수로부터 얻은 파일 정보를 저장하는 구조체 변수
  struct stat sbuf;
  // 문자열을 저장할 버퍼들
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];

  // rio_readlineb를 위해 rio_t타입(구조체)의 읽기 버퍼를 선언
  rio_t rio;

  /* Read request line & headers */

  // rio 구조체 초기화
  Rio_readinitb(&rio, fd);

  // rio_readlineb()로 클라이언트의 요청 라인을 읽고 해당 라인을 출력ㅎㄴ다.
  Rio_readlineb(&rio, buf, MAXLINE); 
  printf("Request headers:\n");
  printf("%s", buf);

  // 요청 라인에서 method, uri, http version 정보를 각각 버퍼 변수에 저장한다.
  sscanf(buf, "%s %s %s", method, uri, version);

  if(strcasecmp(method, "GET") != 0 && strcasecmp(method, "HEAD") != 0) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");

    return;
  }

  // 요청 라인 읽 고 출력
  read_requesthdrs(&rio);

  /* Parse URL from GET request */

  // parse_uri를 통해서 정적 컨텐츠 여부 확인.
  is_static = parse_uri(uri, filename, cgiargs);

  // stat 함수를 사용해서 filename에 해당하는 파일의 정보를 가져온다.
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");

    return;
  }

  if(is_static) {
    /* 정적 컨텐츠의 처리 */

    // 파일의 존재여부 및 읽기 권한 확인.
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");

      return;
    }

    // 문제 없다면 정적 컨텐츠 처리
    serve_static(fd, filename, sbuf.st_size, method);
  }else {
    /* 동적 컨텐츠의 처리 */

    // 파일의 존재여부 및 실행 권한 확인.
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");

      return;
    }

    // 문제 없다면 동적 컨텐츠 처리
    serve_dynamic(fd, filename, cgiargs, method);
  }
};

/**
 * main
 * @param argc
 * @param argv
*/
int main(int argc, char **argv) {
  /* argc = 2, argv[0] = tiny, argv[1] = 3000 */
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Open_listenfd() 에서 socket() , bind() 수행한다.
  listenfd = Open_listenfd(argv[1]);

  // 무한 서버 루프 실행
  while (1) {
    printf("\n\nServer is listening...\n");

    // accept 함수의 인자에 담을 주소의 길이를 계산한다.
    clientlen = sizeof(clientaddr);

    /* 
      int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) 시스템콜 함수.
      서버 소켓이 클라이언트의 연결을 수락하고 실제 통신에 사용할 소켓을 반환한다.
      성공 시 새로운 소켓 파일 디스크립터를 반환한다. 이 새로운 소켓은 클라이언트와의 통신에 사용됨.
      실패 시 -1을 반환함.

      1. sockfd : 연결 요청을 수락할 서버 소켓의 파일 디스크립터.
      2. addr : 클라이언트의 주소 정보를 저장할 struct sockaddr 구조체의 포인터. 여기에 클라이언트의 ip 주소와 포트 번호가 저장된다.
      3. addrlen : addr 구조체의 크기를 가리키는 socklen_t 타입의 포인터. 이 때 addrlen은 호출 전에 구조체의 크기를 나타내고 호출 후에는 실제로 저장된 주소 구조체의 크기로 업데이트 된다.
    */
    // 클라이언트의 요청을 받아들인다.
    connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);

    /*
      int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
      주어진 IP 주소와 포트 번호에 대한 호스트 이름과 서비스 이름을 얻기 위해 사용되는 시스템 콜 함수.
      
      struct sockaddr 구조체를 받아와 해당 주소의 호스트 이름과 서비스 이름을 가져옴. 
      호스트 이름은 host 버퍼에 저장되고, 서비스 이름은 serv 버퍼에 저장. 
      hostlen과 servlen 매개변수는 각각 host와 serv 버퍼의 크기를 나타냄. 
      함수 호출이 성공하면 0을 반환하며, 오류가 발생한 경우 해당 오류 코드를 반환.
    
      sa: 주소 정보가 담긴 struct sockaddr 구조체의 포인터.
      salen: sa 구조체의 크기를 나타내는 값.
      host: 호스트 이름을 저장할 버퍼의 포인터.
      hostlen: 호스트 이름 버퍼의 크기.
      serv: 서비스 이름을 저장할 버퍼의 포인터.
      servlen: 서비스 이름 버퍼의 크기.
      flags: 변환 동작에 영향을 주는 특정 플래그로, 주로 0 또는 특정 플래그 상수를 사용.
    */
    // Getnameinfo()를 통해서 클라이언트의 주소 구조체 구성.
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    // HTTP 트랜잭션 수행 (request -> response)
    doit(connfd); 

    // HTTP 트랜잭션이 수행된 후 서버 쪽의 end-point 를 닫는다. 현재 연결 종료.
    Close(connfd);  // line:netp:tiny:close

    printf("Connection closed!!!!\n");
  }
}
