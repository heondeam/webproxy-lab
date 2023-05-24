/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
#include "csapp.h"

int main(void) {
  // 문자열 처리를 위한 포인터 변수
  char *buf, *p, *n1_p, *n2_p;
  // 문자열을 저장하기 위한 배열
  char arg1[MAXLINE], arg2[MAXLINE], num1[MAXLINE], num2[MAXLINE], content[MAXLINE];
  // 파일 디스크립터를 저장하는 변수
  int fd;
  // 덧셈 연산을 위한 변수
  int n1 = 0, n2 = 0;
  // HTTP 요청 메서드 저장하는 문자열 포인터 변수
  char *method;

  /* Extract the two arguments */

  // 환경변수에서 QUERY_STRING를 가지고 온다.
  if((buf = getenv("QUERY_STRING")) != NULL) {

    // '&' 문자를 찾아서 해당 위치의 포인터 반환
    p = strchr(buf, '&');
    *p = '\0';

    // arg1 과 arg2 문자열 배열에 첫번째 인자와 두번재 인자를 각각 복사한다.    
    strcpy(arg1, buf);
    strcpy(arg2, p+1);

    // num1=value에서 value만 추출하기 위한 작업
    n1_p = strchr(arg1, '=');
    *n1_p = '\0';
    strcpy(num1, n1_p+1);

    // num1=value에서 value만 추출하기 위한 작업
    n2_p = strchr(arg2, '=');
    *n2_p = '\0';
    strcpy(num2, n2_p+1);

    // atoi함수를 사용해서 문자를 정수로 전환한 후에 n1, n2에 저장함. 
    n1 = atoi(num1);
    n2 = atoi(num2);
  }

  // 환경변수에서 REQUEST_METHOD를 가지고 온다.
  method = getenv("REQUEST_METHOD");

  /* Make the response body */

  // tiny.c에서 표준 출력을 연결된 클라이언트 소켓의 파일 디스크립터로 변경했음.
  // 출력이 사용자에게 전달됨.
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */

  // 서버측에 응답 정보 출력
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  // HTTP 메서드가 GET일 경우 content 배열에 저장된 응답 내용을 출력한다.
  // 이는 웹 브라우저로 전송될 HTML 형식의 응답을 나타냄.
  if(strcasecmp(method, "GET") == 0) {
    printf("%s", content);
  }

  // 모든 출력 내용을 웹 서버로 전송하고 버퍼를 비워서 브라우저에 즉시 표시되도록 한다.
  fflush(stdout);
  
  exit(0);
}