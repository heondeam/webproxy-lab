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
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

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

  // Open_listenfd 함수를 호출해서 listen 소켓을 오픈하고 fd를 리턴받는다. 
  // 인자로 포트번호 넘겨줌
  listenfd = Open_listenfd(argv[1]);
  
  // 무한 서버 루프 실행
  while (1) {

    // accept 함수의 인자에 담을 주소의 길이를 계산한다.
    clientlen = sizeof(clientaddr);

    // 반복적으로 연결 요청을 받는다.
    // Accept 함수는 1. 현재 listen 상태에 있는 소켓 fd 2. 소켓 구조체 주소 3. 주소(소켓구조체)의 길이를 인자롤 받음.
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    // Getaddrinfo는 호스트 이름: 호스트 주소, 서비스 이름: 포트 번호의 스트링 표시를 소켓 주소 구조체로 변환
    // Getnameinfo는 위를 반대로 소켓 주소 구조체에서 스트링 표시로 변환.
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    // HTTP 트랜잭션 수행
    doit(connfd);   // line:netp:tiny:doit

    // HTTP 트랜잭션이 수행된 후 자신 쪽의 end-point 를 닫는다.
    Close(connfd);  // line:netp:tiny:close
    printf("Connect closed!!!!\n\n");
  }
}

/**
 * 요청에 대한 오류 응답을 전달하기 위한 함수
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

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
};

/**
 * 
 * 
*/
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if(!strstr(uri, "cgi-bin")) {
    // 정적 컨텐츠일 경우 URI 분석
    
    // cgi 인자 스트링 지움.
    strcpy(cgiargs, "");    

    // uri를 ./index.html 같은 상대 경로 이름으로 변환한다.
    strcpy(filename, ".");   
    strcat(filename, uri);

    // 만약 uri가 "/" 로 끝난다면 기본 파일 이름을 추가한다.
    if(uri[strlen(uri) - 1] == '/') {
      strcat(filename, "home.html");
    }

    return 1;
  }else {
    // 동적 컨텐츠일 경우 URI 분석

    // 모든 cgi 인자들을 추출한다.
    ptr = index(uri, '?');
    
    if(ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }else {
      strcpy(cgiargs, "");
    }

    // 나머지 URI 부분을 상대 리눅스 파일 이름으로 변환한다.
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }
};

/**
 * 파일이름을 인자로 받아서 파일 타입을 리턴한다.
*/
void get_filetype(char *filename, char *filetype) {
  if(strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }else if(strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }else if(strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }else if(strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }else {
    strcpy(filetype, "text/plain");
  }
};


/**
 * 정적 컨텐츠 처리 함수
*/
void serve_static(int fd, char *filename, int filesize) {
  /* html, 무형식 텍스트, GIF, PNG, JPEG의 정적 컨텐츠 지원 */
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // send response headers to client
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
};


/**
 * 동적 컨텐츠의 처리
*/
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) {
    /* Child */
    /* Real server would set all CGI vars here */
    setenv("QEURY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // redirect stdout to client
    Execve(filename, emptylist, environ); // Run CGI prigram
  }

  Wait(NULL); 
};

/**
 * 한개의 트랜잭션(HTTP 요청 -> HTTP 응답)을 처리하는 doit 함수 
 * @param fd 소켓 file descriptor
*/
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];

  // rio_readlineb를 위해 rio_t 타입(구조체)의 읽기 버퍼를 선언
  rio_t rio;

  /* Read request line & headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); // rio_readlineb() 함수로 요청을 읽어들임.
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  /* Tiny 서버는 GET 요청만 처리할 수 있음  */
  if(strcasecmp(method, "GET")) {
    // 요청한 HTTP 메서드가 GET이 아닐 경우에 501 에러 메세지를 보내고
    clienterror(fd, method, "501", "NOT implemented", "Tiny does not implement this method");

    // main 함수로 돌아온다.
    return;
  }

  // GET 요청이 맞다면 요청을 읽어온다.
  read_requesthdrs(&rio);

  /* GET 요청으로부터 받은 URI를 분석한다. */
  is_static = parse_uri(uri, filename, cgiargs);

  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");

    return;
  }

  if(is_static) {
    /* 정적 컨텐츠를 요청 받았을 때 */
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      // 권한이 없다면 403 에러를 리턴한다.
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");

      return;
    }

    // 권한이 있을 경우 처리한다.
    serve_static(fd, filename, sbuf.st_size);
  }else {
    /* 동적 컨텐츠를 요청 받았을 때 */
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      // 권한이 없다면 403 에러를 리턴한다.
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");

      return;
    }

    // 권한이 있을 경우 처리한다.
    serve_dynamic(fd, filename, cgiargs);
  }
};