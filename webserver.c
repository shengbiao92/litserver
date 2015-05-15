#include "common.h"

static int configuration(int* port, char* path){
	int i;
	FILE* fp;
	char* p;
	char buf[50];
	fp=fopen("config.ini","r");
	
	if(fp == NULL){
		perror("fail to open config.ini\n");
		return -1;
	}
	while((fgets(buf, 50, fp)) != NULL){
		if(buf[strlen(buf)-1] != '\n'){
			perror("error in config.ini\n");
			return -1;
		}else
			buf[strlen(buf)-1] = '\0';
		if(strstr(buf,"port") == buf){
			if((p=strchr(buf, ':')) == NULL){
				printf("config.ini expect ':'\n");
				return -1;
			}
			*port=atoi(p + 2);
			if( *port <= 0){
				printf("error port\n");
				return -1;
			}
		}else if(strstr(buf,"root-path") == buf){
			if((p=strchr(buf,':')) == NULL){
				printf("config.ini expect ':'\n");
				return -1;
			}
			p++;
			p++;
			strcpy(path, p);
		}else{
			printf("error in config.ini\n");
			return -1;
		}
	}
	return 0;
} 

int init(struct sockaddr_in* sin, int* lfd, int *port, char* path){
	int tfd;
	configuration(port, path);
	bzero(sin, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons(*port);
	if((tfd=socket(AF_INET, SOCK_STREAM,0))== -1){
		perror("fail to creat socket\n");
		return -1;
	}
	if((bind(tfd, (struct sockaddr *)sin, sizeof(struct sockaddr_in))) == -1){
		perror("fail to bind\n");
		return -1;
	}
	if((listen(tfd,20)) == -1){
		perror("fail to listen\n");
		return -1;
	}
	*lfd=tfd;
	return 0;
}

ssize_t my_read(int fd, void* buffer, size_t length){
	ssize_t done = length;
	while(done > 0){
		done=read(fd, buffer, length);
		if(done == -1){
			if(errno = EINTR)
				done=length;
			else{
				perror("fail to read\n");
				return -1;	
			}
		
	}	else
			break;
	}
	return done;
}

ssize_t my_write(int fd, void* buffer, size_t length){
	ssize_t  done=length;
	while(done > 0){
		done=write(fd,buffer,length);
		if(done != length)
			if(errno = EINTR)
				done = length;
			else{
				perror("fail to write\n");
				return -1;
			}
		else
			break;
	}
	return done;

}

int get_path(int cfd, char* path){
	char buf[MAX_LINE];
	if(my_read(cfd, buf, MAX_LINE) == -1)
		return -1;	
	if(strstr(buf,"GET") != buf){
		DEBUG_PRINT("wrong request\n");
		return -1;
	}
	if((buf[4] == '/') && (buf[5] == ' ')){
		strcat(path, "/index.html");
	}	
	else if((buf[4] == '/') && (buf[5] != ' ')){
		char *tmp;               // error from here
		tmp=strtok(buf+4, " ");
		strcat(path, tmp);
	}
	else{
		return -1;
	}
	return 0;
}

int error_page(int sock_fd){
	char err_str[1024];
	sprintf(err_str, "HTTP/1.1 404 NOT Found\r\n");
	if(my_write(sock_fd, err_str, strlen(err_str)) == -1)
		return -1;
	if(my_write(sock_fd,"Content-Type: text/html\r\n\r\n",strlen("Content-Type: text/html\r\n\r\n")) == -1)
		return -1;
	if(my_write(sock_fd,"<html><body>the file does not exsit!</body></html>",strlen("<html><body>the file does not exsit!</body></html>")) == -1)
		return -1;
	return 0;
}

int write_page(int cfd, int fd, char* path){
	int n;
	char buf[MAX_LINE];
	if(my_write(cfd, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n")) == -1 )
		return -1;
	if(my_write(cfd,"Content-Type: text/html", strlen("Content-Type: text/html")) == -1)
		return -1;
	if(my_write(cfd, "\r\n\r\n",strlen("\r\n\r\n")) == -1)
		return -1;             // write http headers
	while((n=read(fd, buf, MAX_LINE)) > 0)  // write html file content
		if(my_write(cfd, buf, n) == -1)
			return -1;
	return 0;
}
