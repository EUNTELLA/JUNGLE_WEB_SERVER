# Makefile for Echo Server and Client

# C 컴파일러
CC = gcc
# 디버깅 정보 포함 및 모든 경고 활성화
CFLAGS = -g -Wall
# 링크 플래그 (POSIX 스레드 라이브러리)
LDFLAGS = -lpthread

# 'all' 타겟은 클라이언트와 서버를 모두 빌드합니다.
all: client server

# client.o 파일을 컴파일합니다
# client.c와 ../csapp.h,가 변경되면 다시 컴파일됩니다.
client.o: client.c ../csapp.h
	$(CC) $(CFLAGS) -c client.c

# client 실행 파일을 빌드합니다.
# client.o와 ../csapp.o 파일이 필요합니다.
client: client.o ../csapp.o
	$(CC) $(CFLAGS) client.o ../csapp.o -o client $(LDFLAGS)

# server.o 파일을 컴파일합니다.
# server.c와 ../csapp.h,가 변경되면 다시 컴파일됩니다.
server.o: server.c ../csapp.h
	$(CC) $(CFLAGS) -c server.c

# server 실행 파일을 빌드합니다.
# server.o와 ../csapp.o 파일이 필요합니다.
server: server.o ../csapp.o
	$(CC) $(CFLAGS) server.o ../csapp.o -o server $(LDFLAGS)

# 빌드 시 생성된 파일들을 정리합니다.
clean:
	rm -f *~ *.o client server
	# .PHONY 타겟이 아니므로 make clean 실행 시 실제로 파일이 삭제됩니다.