#include "Common.h"


//전역변수영역
int clientCount = 0; //접속한 클라이언트를 카운딩 변수
pthread_mutex_t mutex; //뮤텍스 전역 변수 선언 (어디에서나 사용 가능)

// 서버에 접속할 클라이언트를 관리하기위한 배열 배열의 크기가 관리할 수 있는 클라이언트의 수다.
// 현제는 배열의크기가 10이므로 10명까지 다중접속 관리가 가능하고 배열의 크기를 더크게 하면
// 더 많은 수의 클라이언트에 대한 다중접속관리가 가능하다.
// 다중접속이 가능한 최대 접속자 수의 제한은 없다.
SOCKET client_list[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
char client_name_list[10][10];



int row;
int col;
int play_count = 0;

int sock_list[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int client_index = 0;


#define SERVERPORT 4026
#define BUFSIZE    512


void* ProcessClient(void* arg)

{
    int flag = 0;
    int i = 0;
    
    int client_index;
    int client_opponent;
  


    int retval;
    SOCKET client_sock = (SOCKET)(long long)arg;
    // 클라이언트 목록 추가.

    for (i = 0; i < 10; i++) {
        if (client_list[i] == 0) {
            client_list[i] = client_sock;
            client_index = i;
            break;
        }
    }
    
    printf("client_list : ");
    for (i = 0; i < 10; i++) {
        printf("%d ", client_list[i]);
    }
    printf("\n");


    struct sockaddr_in clientaddr;
    char addr[INET_ADDRSTRLEN];
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    //      sock_list[clientCount-1] = client_sock;

            // 클라이언트 정보 얻기
    printf("클라이언트 정보얻기...");
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

    if (client_index % 2 == 0) send(client_sock, "a", 2, 0);
    else if (client_index % 2 == 1) send(client_sock, "b", 2, 0);

    recv(client_sock, buf, 25, 0);
    strcpy(client_name_list[client_index], buf);

    // 매칭이 성사되면 서버에서  클라이언트 상대의 닉네님을 각각 보내준다
    if (client_index % 2 == 0) {
        client_opponent = client_index + 1;
        if (client_list[client_index] != 0 && client_list[client_index + 1] != 0) {
            send(client_list[client_index], client_name_list[client_opponent], 25, 0);
            send(client_list[client_opponent], client_name_list[client_index], 25, 0);
        }
    }
    else if (client_index % 2 == 1) {
        client_opponent = client_index - 1;
        if (client_list[client_index] != 0 && client_list[client_index - 1] != 0) {
            send(client_list[client_index], client_name_list[client_opponent], 25, 0);
            send(client_list[client_opponent], client_name_list[client_index], 25, 0);

        }
    }


    while (1) {
        // 데이터 받기

        printf("%d", client_sock);
        printf("데이터 받기");
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;



        // 받은 데이터 출력
        buf[retval] = '\0';
        printf("[TCP/%s:%d,total client : %d] %s\n", addr, ntohs(clientaddr.sin_port), clientCount, buf);


        // 데이터 보내기
        retval = send(client_sock, buf, retval, 0);

        if (strcmp(buf, "retract") == 0)
            printf("무르기 신청 받음");
        else if (buf[0] == 'y') printf("무르기 수락\n");
        else if (buf[0] == 'n') printf("무르기 거절\n");

        // 서버에 보낸 데이터를 다시 상대 클라이언트에게 보내주는 부분
        send(client_list[client_opponent], buf, 10, 0);


        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        if (flag == 0) flag = 1;
        else flag = 0;
    }

    // 소켓 닫기
    printf("소켓 닫기");
    // 한명이 접속을 종료하면 나머지 한명에게 out 이라는 데이터를 전송해서 알려준다.
    send(client_list[client_opponent], "out", 10, 0);
    send(client_list[client_index], "out", 10, 0);



    close(client_sock);
    pthread_mutex_lock(&mutex);
    clientCount--;
    pthread_mutex_unlock(&mutex);
    printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d , 남은클라이언트 = %d\n",
        addr, ntohs(clientaddr.sin_port), clientCount); //클라이언트 출력







    for (i = 0; i < 10; i++) {
        if (client_list[i] == client_sock) client_list[i] = 0;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int retval;
    //뮤텍스 초기화
    pthread_mutex_init(&mutex, NULL);
    printf("뮤텍스 초기화");


    // 소켓 생성
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // 데이터 통신에 사용할 변수
    SOCKET client_sock;
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    pthread_t tid;

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        //클라이언트 카운트 증가
        printf("클라이언트 카운트 증가");
        pthread_mutex_lock(&mutex); //뮤텍스 ON
        clientCount++; //전역변수 변경 (클라이언트 카운트 증가)
        pthread_mutex_unlock(&mutex); //뮤텍스 OFF



        // 접속한 클라이언트 정보 출력
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));




        printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d , 접속중인 클라이언트 총수 : %d\n",
            addr, ntohs(clientaddr.sin_port), clientCount); //접속한 클라이언트 출력



    // 스레드 생성
        retval = pthread_create(&tid, NULL, ProcessClient,
            (void*)(long long)client_sock);
        if (retval != 0) { close(client_sock); }
    }

    // 소켓 닫기
    printf("스켓 닫기");
    close(listen_sock);

    // 뮤텍스 삭제
    printf("뮤텍스 삭제");
    pthread_mutex_destroy(&mutex);
    return 0;
}
