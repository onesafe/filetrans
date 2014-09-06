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


int init_socket_client();
int get_ip_port(char *pathname, char *ip, int *port);
int recv_file(int fd);
int Breadn(int fd, char *ptr, int nbytes);


int main(int argc, char **argv)
{
	int		sockfd;

	sockfd = init_socket_client();
	while(1)
	{
		printf("shou file \n");
		recv_file(sockfd);
	}	

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
int recv_file(int fd)
{
 	FILE    *fp;
        char    buf[1024+1];
        int     len;
        char    filename[50];
        int     filelen;
        int     lenleft;
        char    request[10];
        char    stsize[20];
        char    end[10];
	char	createname[50];

	memset(filename, 0x0, sizeof(filename));
        memset(createname, 0x0, sizeof(createname));
        memset(request, 0x0, sizeof(request));
        memset(stsize, 0x0, sizeof(stsize));
        memset(end, 0x0, sizeof(end));

        len = read(fd, request, 7);

	len = read(fd, filename, 50);
        filename[strlen(filename)] = '\0';
        sprintf(createname, "%s-shou", filename);
	createname[strlen(createname)] = '\0';

        read(fd, stsize, 20);
        stsize[strlen(stsize)] = '\0';

        lenleft = filelen = atoi(stsize);

        if((filename != NULL) && (strlen(filename) != 0))
        {
                fp = fopen(createname, "w");
        }


        if(lenleft > 1024)
        {
                while((len = Breadn(fd, buf, 1024)) == 1024)
                {
                        {
                                printf("fwrite error\n");
                                fclose(fp);
                        }
                        lenleft -= 1024;

                        if(lenleft < 1024)
                        {
                                break;
                        }
                        memset(buf, 0x0, sizeof(buf));
                }
                len = Breadn(fd, buf, lenleft);
                if(fwrite(buf, sizeof(char), len, fp) < 0)
                {
                        printf("fwrite error[%s %d]\n", __FILE__, __LINE__);
                        fclose(fp);
                }

        }
        else
        {
                len = Breadn(fd, buf, lenleft);
                if(fwrite(buf, sizeof(char), len, fp) < 0)
                {
                        printf("fwrite error[%s %d]\n", __FILE__, __LINE__);
                        fclose(fp);
                }
        }

        fclose(fp);

        read(fd, end, 3);
        printf("recv_file %s  %d successful\n", filename, filelen );


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
        char    ip[15];
        int     port;
        struct sockaddr_in servaddr;


        if(get_ip_port("/root/onesafe/code/jun/conf5", ip, &port) < 0)
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
}




