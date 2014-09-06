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

#define ipnodeLen 50

typedef struct fileNode{
	int	st_size;
	char	filename[50];
	struct fileNode *next;
}Lnode, *LinkList;

struct fileNode *L;
pthread_mutex_t ListLock = PTHREAD_MUTEX_INITIALIZER;

struct ip_socket{
        int     shou_socket;
        char    ip[20];
        int     flag;
}ipnode[ipnodeLen];

pthread_mutex_t ipLock = PTHREAD_MUTEX_INITIALIZER;


void  init_ipnode();

void  lock_to_store(char *filename, int filelen);
int   get_ip_port(char *pathname, char *ip, int *port);
int   init_socket_client(char *confname);
int   recv_file(int fd);
int   Bwriten(int fd, char *ptr, int nbytes);
int   Breadn(int fd, char *ptr, int nbytes);
void  *start_server1();
void  *start_server2();
void  *parse_file_send();
int   find_socket(char *filename);
int   send_file(char *filename, int fd);
void  *start_server_hj2_1();
void  *start_server_hj2_2();


int main(int argc, char **argv)
{

        int     err;
        pthread_t       tid1,tid2,tid3,tid4;
	pthread_t	tid_hj2_1,tid_hj2_2;
	
	L = (Lnode *)malloc(sizeof(Lnode));
	if(!L)
	{
		printf("create first node error\n");
	}

        err = pthread_create(&tid1, NULL, start_server1, NULL);
        if(err != 0)
        {
                printf("can't create thread start_server1\n");
        }

        err = pthread_create(&tid2, NULL, start_server2, NULL);
        if(err != 0)
        {
                printf("can't create thread start_server2\n");
        }

        err = pthread_create(&tid3, NULL, parse_file_send, NULL);


        err = pthread_create(&tid_hj2_1, NULL, start_server_hj2_1, NULL);
        if(err != 0)
        {
                printf("can't create thread start_server1\n");
        }

        err = pthread_create(&tid_hj2_2, NULL, start_server_hj2_2, NULL);
        if(err != 0)
        {
                printf("can't create thread start_server2\n");
        }


        err = pthread_join(tid1, NULL);
        err = pthread_join(tid2, NULL);
        err = pthread_join(tid_hj2_1, NULL);
        err = pthread_join(tid_hj2_2, NULL);
        err = pthread_join(tid3, NULL);

	exit(0);
}	

//此线程专门用于解析文件并将文件发送到相应的汇聚节点
void  *parse_file_send()
{
	char	filename[50];
	int	fd;
	int	filelen;

	while(1)
	{
		pthread_mutex_lock(&ListLock);
		while(L->next != NULL)
		{
			Lnode   *s;
			s = L->next;

			memset(filename, 0, sizeof(filename));
			sprintf(filename, s->filename);
			filelen = s->st_size;
printf("parse_file_send filename is %s\n", filename);

			if((fd = find_socket(filename)) > 0)
			{
				send_file(filename, fd);
			}
			else if(fd <= 0)
			{
				printf("find_socket error\n");
			}


			L->next = s->next;
			free(s);
		}

		pthread_mutex_unlock(&ListLock);

		sleep(3);
	}
}


int  find_socket(char *filename)
{
	char	ipsrc[20];
	char	ipdest[20];
	int	i = 0;
	FILE	*fp;
	char	buf[100];
	int	n = 0;

	if((fp = fopen(filename, "r")) == NULL)
	{
		printf("open filename error [%d]\n", __LINE__);
	}

	if(fgets(ipsrc, 20, fp) == NULL)
        {
                printf("fgets ipsrc error\n");
        }

        if(fgets(ipdest, 20, fp) == NULL)
        {
                printf("fgets ipdest error\n");
        }

	ipsrc[strlen(ipsrc)-1] = '\0';
	ipdest[strlen(ipdest)-1] = '\0';
printf("ipsrc is %s, ipdest is %s\n", ipsrc, ipdest);
	fclose(fp);

//write to xin.log
	sprintf(buf, "ipsrc: %s, ipdest: %s\n", ipsrc, ipdest);
printf("buf is %s", buf);
	if((fp = fopen("xin.log", "a+")) == NULL)
	{
		printf("open xin.log error [%d]\n", __LINE__);
	}
	n = strlen(buf);	
	if(fwrite(buf, sizeof(char), n, fp) != n) 
	{
		printf("fwrite to xin.log error\n");
	}
	fclose(fp);

	for(i=0; i<ipnodeLen; i++)
	{
		if(ipnode[i].flag == 0)
		{
			continue;
		}
		else if(ipnode[i].flag == 1)
		{
			if(strncmp(ipnode[i].ip, ipdest, strlen(ipdest)) == 0)
			{
printf("found socket!!!\n");
				return ipnode[i].shou_socket;
			}
		}
	}
	return -1;
}

int send_file(char *filename, int fd)
{
        FILE    *fp;
        char    buf[1024+1];
        int     len, n;


        struct stat stbuf;
        char    request[7];
        char    stsize[20];
        char    end[3];
        char    file[20];

        if(lstat(filename, &stbuf) < 0)
        {
                printf("stat error");
        }

//    printf("filename size is %d\n", stbuf.st_size);

        sprintf(request, "request");
        write(fd, request, 7);

        sprintf(file, "%s", filename);
//        printf("buf filename is %s\n", file);
        write(fd, file, 50);

        sprintf(stsize, "%d", stbuf.st_size);
//        printf("buf st_size is %s\n", stsize);
        write(fd, stsize, 20);



        if(!(fp = fopen(filename, "r")))
        {
                printf("open file error\n");
                return -1;
        }

        while( (len = fread(buf, sizeof(char), 1024, fp)) == 1024)
        {
                n = Bwriten(fd, buf, 1024);
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
                n = Bwriten(fd, buf, len);
                if(n != len)
                {
                        printf("send file content error2\n");
                        fclose(fp);
                        return -1;
                }
        }
        fclose(fp);

        sprintf(end, "end");
        write(fd, end, 3);

        printf("function send_file %s over\n\n", filename);
        return 0;
}




//此线程用于接收来自汇聚节点的ip
void *start_server2()
{
        int     sockfd;
	char	ip[20];
	int	i = 0;

	init_ipnode();


        sockfd = init_socket_client("/root/onesafe/code/jun/conf8");
        if(sockfd < 0)
        {
                printf("init_socket_client error\n");
        }

        while(1)
        {
                memset(ip, 0, sizeof(ip));
                read(sockfd, ip, 20);
		ip[strlen(ip)-1] = '\0';

//printf("start receive huiju1 ip .......%s.....\n", ip);
                pthread_mutex_lock(&ipLock);

                for(i=0; i<ipnodeLen; i++)
                {
			if(!strcmp(ip, ipnode[i].ip))
			{
				if((ipnode[i].flag == 0))
				{
                			printf("recv ip is %s\n", ip);
					sprintf(ipnode[i].ip, "%s", ip);
//				printf("ipnode[i].ip is %s\n", ipnode[i].ip);
					ipnode[i].shou_socket = sockfd;
					ipnode[i].flag = 1;
				}
			}
                }

                pthread_mutex_unlock(&ipLock);
		
		sleep(3);
	}

}


void *start_server_hj2_2()
{
        int     sockfd;
        char    ip[20];
        int     i = 0;

        init_ipnode();


        sockfd = init_socket_client("/root/onesafe/code/jun/conf_hj2_2");
        if(sockfd < 0)
        {
                printf("init_socket_client error\n");
        }
//printf("i am in xin , shou qu ip haha\n");
        while(1)
        {
                memset(ip, 0, sizeof(ip));
                read(sockfd, ip, 20);
		ip[strlen(ip)] = '\0';
                pthread_mutex_lock(&ipLock);
//printf("received ip is %s\n", ip);
                for(i=0; i<ipnodeLen; i++)
                {

			if(strncmp(ip, ipnode[i].ip, strlen(ip)) == 0)
			{
//				printf("ipnode already haved the same ip\n");
				break;
			}
                        else if(strncmp(ip, ipnode[i].ip, strlen(ip)) != 0) 
                        {
                                if((ipnode[i].flag == 0))
                                {
                                        strncpy(ipnode[i].ip, ip, strlen(ip));
//printf("ipnode[%d]  is %s \n", i, ipnode[i].ip);
                                        ipnode[i].shou_socket = sockfd;
                                        ipnode[i].flag = 1;
					break;
                                }
                        }

                }

                pthread_mutex_unlock(&ipLock);

                sleep(10);
        }

}


//此线程用于接收来自汇聚节点的文件
void *start_server1()
{
        int     sockfd;


        sockfd = init_socket_client("/root/onesafe/code/jun/conf9");
        if(sockfd < 0)
        {
                printf("init_socket_client error\n");
        }

        while(1)
        {
                if(recv_file(sockfd) < 0)
                {
                        printf("send_file error\n");
                }

                sleep(3);
        }

}

void *start_server_hj2_1()
{
        int     sockfd;


        sockfd = init_socket_client("/root/onesafe/code/jun/conf_hj2_1");
        if(sockfd < 0)
        {
                printf("init_socket_client error\n");
        }

        while(1)
        {
                if(recv_file(sockfd) < 0)
                {
                        printf("send_file error\n");
                }

                sleep(3);
        }

}




void  init_ipnode()
{
        int     i;
        for(i=0; i<ipnodeLen; i++)
        {
                ipnode[i].flag = 0;
                ipnode[i].shou_socket = -1;
		strcpy(ipnode[i].ip, "0");
        }
}


/**************************************
 ** 功能：从套接字fd读取nbytes字节数
 **
 ** ************************************/
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



/**************************************
 *  *功能：接收文件并命名为filename
 *   *
 *    * ***********************************/
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
//        printf("request is %s, len = %d\n", request,len);

        len = read(fd, filename, sizeof(filename));
        filename[strlen(filename)] = '\0';
//	printf("strlen(filename) is %d", strlen(filename));
        sprintf(createname, "%s", filename);
//        printf("createname is %s, len = %d\n", createname,len);
	createname[strlen(createname)] = '\0';

        read(fd, stsize, sizeof(stsize));
        stsize[strlen(stsize)] = '\0';
//        printf("stsize is %s\n", stsize);

        lenleft = filelen = atoi(stsize);
//        printf("filelen is %d\n", filelen);


        if((filename != NULL) && (strlen(filename) != 0))
        {
		fp = fopen(createname, "w");
        }

        if(lenleft > 1024)
        {
                while((len = Breadn(fd, buf, 1024)) == 1024)
                {

                        if(fwrite(buf, sizeof(char), len, fp) != 1024)
                        {
                                printf("fwrite error\n");
                                fclose(fp);
                                return -1;
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
                        return -1;
                }

        }
        else
        {
                len = Breadn(fd, buf, lenleft);
                if(fwrite(buf, sizeof(char), len, fp) < 0)
                {
                        printf("fwrite error[%s %d]\n", __FILE__, __LINE__);
	                fclose(fp);
        	        return -1;
                }
        }


        fclose(fp);

//printf("before read end createname is %s \n", createname);
        read(fd, end, 3);
//        printf("end is %s\n", end);
//printf("createname is %s \n", createname);
        printf("recv_file %s  %d successful\n\n", createname, filelen );

        lock_to_store(filename, filelen);

        return 0;
}


void lock_to_store(char *filename, int filelen)
{
        Lnode   *s;
        pthread_mutex_lock(&ListLock);

        s = (Lnode *)malloc(sizeof(Lnode));
        if(!s)
        {
                printf("create node s error\n");
        }
        s->st_size = filelen;
        sprintf(s->filename, filename);

        s->next = L->next;
        L->next = s;

        pthread_mutex_unlock(&ListLock);
}



/************************************************
 * 功能：初始化socket_client
 * 返回值：返回socket描述符
 *
 * 
 * *********************************************/
int init_socket_client(char *confname)
{
	int     sockfd;
        char    ip[20];
        int     port;
        struct sockaddr_in servaddr;


        if(get_ip_port(confname, ip, &port) < 0)
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
//	printf("ip is %s ", ip);
//	printf("port is %d\n", port);

}




