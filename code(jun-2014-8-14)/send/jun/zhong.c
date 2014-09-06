#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <limits.h>


int init_socket_server();
int init_socket_client();
int get_ip_port(char *pathname, char *ip, int *port);
int recv_file(char *filename, int fd);
int send_file(char *filename);
int Breadn(int fd, char *ptr, int nbytes);
int Bwriten(int fd, char *ptr, int nbytes);


int main(int argc, char **argv)
{
	int		listenfd;
	int		connfd;
        socklen_t       clilen;
        struct sockaddr_in cliaddr;
	int		sockfd;
	char		filename[100];
/*
	if((listenfd = init_socket_server()) < 0)
	{
		printf("init_socket_server error [%d]\n", __LINE__);
		return -1;
	}
	
	clilen = sizeof(cliaddr);
	connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		
	recv_file("/root/onesafe/code/jun/received.txt", connfd);

	close(connfd);
*/	
	sprintf(filename, argv[1]);
	printf("filename is %s\n", filename);
	
	send_file(filename);

}

/**************************************
 * 功能：从套接字fd读取nbytes字节数
 *
 * ************************************/
int Breadn(int fd, char *ptr, int nbytes)
{
	int nleft, nread;
	
	nleft = nbytes;
	while(nleft > 0)
	{
		errno = 0;
		nread = read(fd, ptr, nleft);
		if(nread < 0)
		{
			return nread;
		}
		else if(nread == 0)
		{
			break;
		}
		nleft -= nread;
		ptr += nread;
	}
	
	return (nbytes - nleft);
}


/**************************************
 *功能：接收文件并命名为filename
 *
 * ***********************************/
int recv_file(char *filename, int fd)
{
	FILE	*fp;
	char	buf[1024+1];
	int	len;
	
	if(!(fp = fopen(filename, "w")))	
	{
		printf("open filename error\n");
		return -1;
	}
	
	while((len = Breadn(fd, buf, 1024)) == 1024)
	{

		if(fwrite(buf, sizeof(char), len, fp) != 1024)
		{
			printf("fwrite error\n");
			fclose(fp);
			return -1;
		}
		memset(buf, 0x0, sizeof(buf));
	}
	
	if(fwrite(buf, sizeof(char), len, fp) < 0)
	{
		printf("fwrite error[%s %d]\n", __FILE__, __LINE__);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	printf("function recv_file over\n");
	return 0;
}

/************************************************
 ** 功能：初始化socket_server
 ** 返回值：返回socket描述符
 **
 **
 ** *********************************************/
int init_socket_server()
{
	int     listenfd;
        int     opt = 1;
	char    ip[15];
        int     port;
        pid_t   childpid;
        struct sockaddr_in  servaddr;

	if(get_ip_port("/root/onesafe/code/jun/conf", ip, &port) < 0)
        {
                printf("get_ip_port error\n");
		return -1;
        }

	printf("ip is %s ", ip);
        printf("port is %d\n", port);


        listenfd = socket(AF_INET, SOCK_STREAM,0);
        if(listenfd < 0)
        {
                printf("create socket failed \n");
		return -1;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        {
                printf("bind failed\n");
		return -1;
        }

        listen(listenfd, 20);

        printf("start server!\n");

	
	return listenfd;
}


/************************************************
 ** 功能：初始化socket_client
 ** 返回值：返回socket描述符
 **
 **
 ** *********************************************/
int init_socket_client()
{
        int     sockfd;
        char    ip[20];
        int     port;
        struct sockaddr_in servaddr;


        if(get_ip_port("/root/onesafe/code/jun/conf7", ip, &port) < 0)
        {
                printf("get_ip_port error\n");
                return -1;
        }

        printf("ip is %s ", ip);
        printf("port is %d\n", port);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
                printf("create sockfd error\n");
                return -1;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

        if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
                printf("connect server error\n");
		perror("connect");
		printf("%s\n", strerror(errno));
		printf("errno = %d\n", errno);
                return -1;
        }

        printf("start client!\n");

        return sockfd;
}



/*****************************************
 ** 功能：  获取配置文件中的ip和port
 ** 参数：  文件名路径，ip
 ** 返回值：端口
 **
 **
 ** **************************************/
int get_ip_port(char *pathname, char *ip, int *port)
{
        FILE    *fp;
        char    strport[10];
        int     iport;

        if((fp = fopen(pathname, "r")) == NULL)
        {
                printf("open pathname error\n");
                return -1;
        }

        if(fgets(ip, 20, fp) == NULL)
        {
                printf("fgets ip error\n");
        }

        if(fgets(strport, 10, fp) == NULL)
        {
                printf("fgets port error\n");
        }

        iport = atoi(strport);
        ip[strlen(ip)-1] = '\0';
        *port = iport;

        fclose(fp);
}



/*************************************
 ** 功能：发送文件
 **
 **
 ** ***********************************/

int send_file(char *filename)
{
        int     sockfd;
        FILE    *fp;
        char    buf[1024+1];
        int     len, n;
	struct stat stbuf;
	char	request[7];
	char	stsize[20];
	char	end[3];
	char	file[50];

        sockfd = init_socket_client();
        if(sockfd < 0)
        {
                printf("init_socket_client error\n");
        }

	if(lstat(filename, &stbuf) < 0)
	{
		printf("stat error");
	}

	printf("filename size is %d\n", stbuf.st_size);

	sprintf(request, "request");
	write(sockfd, request, 7);

	sprintf(file, "%s", filename);
	printf("buf filename is %s\n", file);
	write(sockfd, file, 50);
	
	sprintf(stsize, "%d", stbuf.st_size);
	printf("buf st_size is %s\n", stsize);
	write(sockfd, stsize, 20);


        if(!(fp = fopen(filename, "r")))
        {
                printf("open file error\n");
                return -1;
        }
	
        while( (len = fread(buf, sizeof(char), 1024, fp)) == 1024)
        {
                n = Bwriten(sockfd, buf, 1024);
                if(n != 1024)
                {
                        printf("send file content error\n");
                        fclose(fp);
                        return -1;
                }
                memset(buf, 0x0, sizeof(buf));
        }
        if(len < 0)
        {
                printf("read file error\n");
                fclose(fp);
                return -1;
        }
        else
        {
                n = Bwriten(sockfd, buf, len);
                if(n != len)
                {
                        printf("send file content error2\n");
                        fclose(fp);
                        return -1;
                }
        }

        fclose(fp);

        
	sprintf(end, "end");
	write(sockfd, end, 3);

        printf("function send_file over\n");
        return 0;
}


/*******************************************
 ** 功能：循环发送直到发送完指定数量的字节数
 **
 **
 *******************************************/
int Bwriten(int fd, char *ptr, int nbytes)
{
        int nleft, nwriten;

        nleft = nbytes;
        while(nleft > 0)
        {
                errno = 0;
                nwriten = write(fd, ptr, nleft);
                if(nwriten < 0)
                {
                        return nwriten;
                }
                nleft -= nwriten;
                ptr += nwriten;
        }

        return (nbytes - nleft);
}


