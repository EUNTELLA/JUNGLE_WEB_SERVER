#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 16
#define NTHREADS 4

/* You won't lose style points for including this long line in your code */
typedef struct
{
  int *buf;    /* 버퍼 배열 */
  int n;       /* 최대 슬롯 수 */
  int front;   /* buf[(front+1)%n]이 첫 번째 항목 */
  int rear;    /* buf[rear%n]이 마지막 항목 */
  sem_t mutex; /* 버퍼 접근 보호 */
  sem_t slots; /* 사용 가능한 슬롯 수 */
  sem_t items; /* 사용 가능한 항목 수 */
} sbuf_t;

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void build_header(char *header, char *hostname, char *path, rio_t *client_rio);
void parse_uri(char *uri, char *hostname, char *filepath, int *port);
int connect_endserver(char *hostname, int port, char *http_header);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

sbuf_t sbuf;

int main(int argc, char *argv[])
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  printf("%s", user_agent_hdr);

  listenfd = Open_listenfd(argv[1]);

  sbuf_init(&sbuf, SBUFSIZE);
  for (int i = 0; i < NTHREADS; i++)
  {
    Pthread_create(&tid, NULL, thread, NULL);
  }

  while (1)
  {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    sbuf_insert(&sbuf, connfd);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s,%s)\n", hostname, port);
  }
}

void doit(int fd)
{
  int end_serverfd, port = 80;

  // 버퍼
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  // 파일 경로와  CGI 프로그램 인자를 저장할 버퍼
  char filepath[MAXLINE], hostname[MAXLINE];
  char endserver_header[MAXLINE];

  rio_t client_rio, server_rio;

  rio_readinitb(&client_rio, fd);
  rio_readlineb(&client_rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Proxy dose not implement this method");
    return;
  }

  parse_uri(uri, hostname, filepath, &port);

  build_header(endserver_header, hostname, filepath, &client_rio);

  end_serverfd = connect_endserver(hostname, port, endserver_header);

  if (end_serverfd < 0)
  {
    printf("Connection failed\n");
    return;
  }

  Rio_readinitb(&server_rio, end_serverfd);

  Rio_writen(end_serverfd, endserver_header, strlen(endserver_header));

  size_t n;
  // while((n=rio_readlineb(&server_rio,buf,MAXLINE))!=0){
  //   printf("Proxy received %ld bytes, then send to cliend %d\n",n,fd);
  //   Rio_writen(fd,buf,n);
  // }
  // Close(end_serverfd);
  while ((n = Rio_readnb(&server_rio, buf, MAXLINE)) > 0)
  {
    printf("Proxy received %ld bytes, then send to client %d\n", n, fd);
    Rio_writen(fd, buf, n);
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

void parse_uri(char *uri, char *hostname, char *filepath, int *port)
{
  char *ptr = strstr(uri, "//");

  strcpy(filepath, "/");
  ptr = ptr != NULL ? ptr + 2 : uri;

  char *temp = strstr(ptr, ":");
  if (temp != NULL)
  {
    *temp = '\0';
    sscanf(ptr, "%s", hostname);
    sscanf(temp + 1, "%d%s", port, filepath);
  }
  else
  {
    temp = strstr(ptr, "/");
    if (temp != NULL)
    {
      *temp = '\0';
      sscanf(ptr, "%s", hostname);
      *temp = '/';
      sscanf(temp, "%s", filepath);
    }
    else
    {
      sscanf(ptr, "%s", hostname);
    }
  }
  return;
}

void build_header(char *header, char *hostname, char *path, rio_t *client_rio)
{
  char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];

  other_hdr[0] = '\0';
  host_hdr[0] = '\0';

  static const char *connection_key = "Connection";
  static const char *user_agent_key = "User-Agent";
  static const char *proxy_connection_key = "Proxy-Connection";

  sprintf(request_hdr, "GET %s HTTP/1.0\r\n", path);

  while (Rio_readlineb(client_rio, buf, MAXLINE) > 0)
  {
    if (strcmp(buf, "\r\n") == 0)
    {
      break;
    }
    if (!strncasecmp(buf, "Host", strlen("Host")))
    {
      strcpy(host_hdr, buf);
      continue;
    }

    if (strncasecmp(buf, connection_key, strlen(connection_key)) && strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key)) && strncasecmp(buf, user_agent_key, strlen(user_agent_key)))
    {
      strcat(other_hdr, buf);
    }
  }
  if (strlen(host_hdr) == 0)
  {
    sprintf(host_hdr, "Host: %s\r\n", hostname);
  }
  sprintf(header, "%s%s%s%s%s%s%s",
          request_hdr,
          host_hdr,
          "Connection: close\r\n",
          "Proxy-Connection: close\r\n",
          user_agent_hdr,
          other_hdr,
          "\r\n");
}

int connect_endserver(char *hostname, int port, char *http_header)
{
  char portstr[100];
  sprintf(portstr, "%d", port);
  return Open_clientfd(hostname, portstr);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "<hr><em>The Tiny Web Server</em>\r\n");

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void *thread(void *vargp)
{
  Pthread_detach(pthread_self());
  while (1)
  {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}
void sbuf_init(sbuf_t *sp, int n)
{
  sp->buf = Calloc(n, sizeof(int));
  sp->n = n;
  sp->front = sp->rear = 0;
  Sem_init(&sp->mutex, 0, 1);
  Sem_init(&sp->slots, 0, n);
  Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp)
{
  Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
  P(&sp->slots);
  P(&sp->mutex);
  sp->buf[(++sp->rear) % (sp->n)] = item;
  V(&sp->mutex);
  V(&sp->items);
}

int sbuf_remove(sbuf_t *sp)
{
  int item;
  P(&sp->items);
  P(&sp->mutex);
  item = sp->buf[(++sp->front) % (sp->n)];
  V(&sp->mutex);
  V(&sp->slots);
  return item;
}