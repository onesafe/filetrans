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

int   get_ip_port(char *pathname, char *ip, int *port);
int   init_socket_client();
int   send_file(char *filename);
int   Bwriten(int fd, char *ptr, int nbytes);



int main(int argc, char **argv)
{

	if(send_file("/root/onesafe/code/jun/song") < 0)
	{
		printf("send_file error\n");
	}
	exit(0);
}	


/*******************************************
 * 功能：循环发送直到发送完指定数量的字节数
 *
 *
 ******************************************/
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


/*************************************
 * 功能：发送文件
 *
 *
 * ***********************************/

int send_file(char *filename)
{
	int	sockfd;
	FILE	*fp;
	char	buf[1024+1];
	int	len, n;

	sockfd = init_socket_client();
	if(sockfd < 0)
	{
		printf("init_socket_client error\n");
	}


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
	printf("function send_file over\n");
	return 0;	
}



/************************************************
 * 功能：初始化socket_client
 * 返回值：返回socket描述符
 *
 * 
 * *********************************************/
int init_socket_client()
{
	int     sockfd;
        char    ip[15];
        int     port;
        struct sockaddr_in servaddr;


        if(get_ip_port("/root/onesafe/code/jun/conf", ip, &port) < 0)
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
		return -1;
	}

        printf("start client!\n");
        
	return sockfd;
}



/*****************************************
 * 功能：  获取配置文件中的ip和port
 * 参数：  文件名路径，ip
 * 返回值：端口
 *
 * 
 * **************************************/
int get_ip_port(char *pathname, char *ip, int *port)
{
	FILE	*fp;
	char	strport[10];
	int	iport;

	if((fp = fopen(pathname, "r")) == NULL)
	{
		printf("open pathname error\n");
		return -1;
	}

	if(fgets(ip, 15, fp) == NULL)
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
//	printf("ip is %s ", ip);
//	printf("port is %d\n", port);

}




