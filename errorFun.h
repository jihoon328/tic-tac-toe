 typedef int SOCKET;
 #define SOCKET_ERROR -1
 #define INVALID_SOCKET -1

 void err_quit(const char *msg);
 void err_display(const char *msg);
 void err_display(int errcode);

