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

typedef struct fileNode_f_xin{
	int	st_size;
	char	filename[50];
	struct fileNode_f_xin *next;
}Lnode_xin;

struct fileNode	*L;
struct fileNode_f_xin *L_xin;

int	fileNodeCount = 0;
pthread_mutex_t	ListLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ListLock_xin = PTHREAD_MUTEX_INITIALIZER;


struct ip_socket{
	int	shou_socket;
	char	ip[20];
	int	flag;
}ipnode[ipnodeLen];

pthread_mutex_t ipLock = PTHREAD_MUTEX_INITIALIZER;



int   init_socket_server(char *confname);
int   get_ip_port(char *pathname, char *ip, int *port);
void  *recv_file(void *arg);
void  *write_ip_toxin(void *arg);
void  *recv_parse_file(void *arg);
int   find_socket(char *filename);
void  *parse_file_send();
int   send_file(char *filename, int fd);
int   Breadn(int fd, char *ptr, int nbytes);
int   Bwriten(int fd, char *ptr, int nbytes);
void  *store_ip(void *arg);
void  lock_to_store(char *filename, int filelen);
void  lock_to_store_f_xin(char *filename, int filelen);
void  *start_server1();
void  *start_server2();
void  *start_server3();
void  *start_server4();
void  init_ipnode();



int main(int argc, char **argv)
{

	int	err;
	pthread_t	tid1,tid2,tid3,tid4;
	pthread_t	tid_parsefile;

	L = (Lnode *)malloc(sizeof(Lnode));
        if(!L)
        {
                printf("create first node error\n");
        }
        L->next = NULL;

        L_xin = (Lnode_xin *)malloc(sizeof(Lnode_xin));
        if(!L_xin)
        {
                printf("create first node error\n");
        }
        L_xin->next = NULL;
		
	init_ipnode();

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

	err = pthread_create(&tid3, NULL, start_server3, NULL);
	if(err != 0)
        {
                printf("can't create thread start_server2\n");
	}

	err = pthread_create(&tid4, NULL, start_server4, NULL);
	if(err != 0)
        {
                printf("can't create thread start_server2\n");
	}

        err = pthread_create(&tid_parsefile, NULL, parse_file_send, NULL);
        if(err != 0)
        {
                printf("can't create thread start_server2\n");
        }

	err = pthread_join(tid1, NULL);
	err = pthread_join(tid2, NULL);
	err = pthread_join(tid3, NULL);
	err = pthread_join(tid4, NULL);
	err = pthread_join(tid_parsefile, NULL);

}


//此线程专门用于解析文件并将文件发送到相应的主机
void  *parse_file_send()
{
        char    filename[50];
        int     fd;
        int     filelen;

        while(1)
        {
                pthread_mutex_lock(&ListLock_xin);
                while(L_xin->next != NULL)
                {
                        Lnode_xin   *s;
                        s = L_xin->next;

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


                        L_xin->next = s->next;
                        free(s);
                }

                pthread_mutex_unlock(&ListLock_xin);

                sleep(3);
        }
}


int  find_socket(char *filename)
{
        char    ipsrc[20];
        char    ipdest[20];
        int     i = 0;
        FILE    *fp;

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


void *start_server1()
{
	int             listenfd;
        int             connfd;
        socklen_t       clilen;
        struct sockaddr_in cliaddr;
        int             sockfd;
	int		child_pid;
	pthread_t	tid5;


        if((listenfd = init_socket_server("/root/onesafe/code/jun/conf7")) < 0)
        {
                printf("init_socket_server error [%d]\n", __LINE__);
        }

//printf("listenfd is %d\n", listenfd);
	while(1)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));	
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		
		pthread_create(&tid5, NULL, recv_file, &connfd);
		pthread_join(tid5, NULL);
	}
}

//接收来自客户端shou程序的连接
void *start_server3()
{
	int             listenfd;
        int             connfd;
        socklen_t       clilen;
        struct sockaddr_in cliaddr;
        int             sockfd;
	int		child_pid;
	pthread_t	tid3;
	
        if((listenfd = init_socket_server("/root/onesafe/code/jun/conf5")) < 0)
        {
                printf("init_socket_server error [%d]\n", __LINE__);
        }

	while(1)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));	
		clilen = sizeof(cliaddr);

		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

		pthread_create(&tid3, NULL, store_ip, &connfd);
		pthread_join(tid3, NULL);
	}
}


void  init_ipnode()
{
	int	i;
	for(i=0; i<ipnodeLen; i++)
	{
		ipnode[i].flag = 0;
		ipnode[i].shou_socket = -1;
	}
}


void *store_ip(void *arg)
{
	int	fd = *(int *)arg;
	char		ip[20];
	int		port;
	struct sockaddr_in rsa;
	int	i = 0;

	socklen_t rsa_len = sizeof(struct sockaddr_in);

	memset(&ip, 0, sizeof(ip));
	if(getpeername(fd, (struct sockaddr *)&rsa, &rsa_len) == 0)
	{
		strcpy(ip, inet_ntoa(rsa.sin_addr));
		port = ntohs(rsa.sin_port);
	}
	printf("client ip is %s, ", ip);
	printf("client port is %d\n", port);


	
	pthread_mutex_lock(&ipLock);

	for(i=0; i<ipnodeLen; i++)
	{
		if(ipnode[i].flag == 0)
		{
			strcpy(ipnode[i].ip, ip);
//			printf("ipnode[%d].ip is %s\n", i, ipnode[i].ip);
			ipnode[i].shou_socket = fd;
			ipnode[i].flag = 1;
			break;
		}
	}	
	
	pthread_mutex_unlock(&ipLock);

}

//往中心不断发送任务队列中的文件
void *start_server2()
{
	int             listenfd;
        int             connfd;
        socklen_t       clilen;
        struct sockaddr_in cliaddr;
        int             sockfd;
	char		filename[50];
	int		filelen;
	char		ip[20];
	int		i = 0;

        if((listenfd = init_socket_server("/root/onesafe/code/jun/conf9")) < 0)
        {
                printf("init_socket_server error [%d]\n", __LINE__);
        }

        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);


	while(1)
	{
		pthread_mutex_lock(&ListLock);
		while(L->next != NULL)
		{
			Lnode	*s;
			s = L->next;

			memset(filename, 0, sizeof(filename));
			sprintf(filename, s->filename);
			filelen = s->st_size;
					
	        	send_file(filename, connfd);
			
			L->next = s->next;
			free(s);
		}

		pthread_mutex_unlock(&ListLock);

		sleep(3);

		
	}

        close(connfd);
}

//往中心节点发送ip并且接收来自中心解析并发送过来的文件
void *start_server4()
{
	int             listenfd;
        int             connfd;
        socklen_t       clilen;
        struct sockaddr_in cliaddr;
        int             sockfd;
	int		child_pid;
	pthread_t	tid_recv_parse_file;
	pthread_t	tid_write_ip;
	int		err = 0;

        if((listenfd = init_socket_server("/root/onesafe/code/jun/conf8")) < 0)
        {
                printf("init_socket_server error [%d]\n", __LINE__);
        }


	memset(&cliaddr, 0, sizeof(cliaddr));	
	clilen = sizeof(cliaddr);

	connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

	err = pthread_create(&tid_recv_parse_file, NULL, recv_parse_file, &connfd);
	if(err != 0)
	{
		printf("can't create thread start_server2\n");
	}

	err = pthread_create(&tid_write_ip, NULL, write_ip_toxin, &connfd);
	if(err != 0)
	{
		printf("can't create thread start_server2\n");
	}

	err = pthread_join(tid_recv_parse_file, NULL);
	err = pthread_join(tid_write_ip, NULL);

	close(connfd);
}

void *write_ip_toxin(void *arg)
{
	int	fd = *(int *)arg;
	char	ip[20];	
	int	i = 0;

        while(1)
        {
                pthread_mutex_lock(&ipLock);

                for(i=0; i<ipnodeLen; i++)
                {
                        if(ipnode[i].flag == 1)
                        {
                                strcpy(ip, ipnode[i].ip);
                                write(fd, ip, 20);
                        }
                }

                pthread_mutex_unlock(&ipLock);
//printf("i am in huiju two writed ip to xin\n");
                sleep(10);

        }
}

void *recv_parse_file(void *arg)
{
	int	fd = *(int *)arg;
        FILE    *fp;
        char    buf[1024+1];
        int     len;
        char    filename[50];
        int     filelen;
        int     lenleft;
        char    request[10];
        char    stsize[20];
        char    end[10];
        char    createname[50];

printf("recv_parse_file!!!\n");
        memset(filename, 0x0, sizeof(filename));
        memset(createname, 0x0, sizeof(createname));
        memset(request, 0x0, sizeof(request));
        memset(stsize, 0x0, sizeof(stsize));
        memset(end, 0x0, sizeof(end));
while(1)
{
        if((len = read(fd, request, 7)) < 0)
	{
		if(errno == EINTR)
		{
			continue;
		}
		else
			return ;
	}
	else if( len == 0)
	{
		break;
	}

//        printf("request is %s, len = %d\n", request,len);

        len = read(fd, filename, sizeof(filename));
        filename[strlen(filename)] = '\0';
//        printf("strlen(filename) is %d", strlen(filename));
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

//printf("before read end createname is %s \n", createname);
        read(fd, end, 3);
//        printf("end is %s\n", end);
//printf("createname is %s \n", createname);
        printf("recv_file %s  %d successful\n\n", createname, filelen );

	lock_to_store_f_xin(filename, filelen);
}
}


void lock_to_store_f_xin(char *filename, int filelen)
{
        Lnode_xin   *s;
        pthread_mutex_lock(&ListLock_xin);

        s = (Lnode_xin *)malloc(sizeof(Lnode_xin));
        if(!s)
        {
                printf("create node s error\n");
        }
        s->st_size = filelen;
        sprintf(s->filename, filename);

        s->next = L_xin->next;
        L_xin->next = s;

        pthread_mutex_unlock(&ListLock_xin);
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
 *功能：接收文件并命名为filenaee
 *
 * ***********************************/
void  *recv_file(void *arg)
{
	FILE	*fp;
	char	buf[1024+1];
	int	len;
	char	filename[50];
	int	filelen;
	int	lenleft;
	char	request[10];
	char	stsize[20];
	char	end[10];

	int fd = *(int *)arg;

//	printf("fd is %d \n", fd);
	len = read(fd, request, 7);
//	printf("request is %s, len = %d\n", request,len);

	len = read(fd, filename, 50);
	filename[strlen(filename)] = '\0';
	sprintf(filename, "%s", filename);
//	printf("filename is %s, len = %d\n", filename,len);
	
	read(fd, stsize, 20);
	stsize[strlen(stsize)] = '\0';
//	printf("stsize is %s\n", stsize);

	lenleft = filelen = atoi(stsize);
//	printf("filelen is %d\n", filelen);

	if(!(fp = fopen(filename, "w")))	
	{
		printf("open filename error %d, %s\n", __LINE__, __FILE__);
	}

	if(lenleft > 1024)
	{
		while((len = Breadn(fd, buf, 1024)) == 1024)
		{
			if(fwrite(buf, sizeof(char), len, fp) < 0)
			{
				printf("fwrite error\n", __FILE__, __LINE__);
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
//	printf("end is %s\n", end);
	
	printf("recv_file %s  %d successful\n", filename, filelen );

	lock_to_store(filename, filelen);	

}



void lock_to_store(char *filename, int filelen)
{
	Lnode	*s;
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
 ** 功能：初始化socket_server
 ** 返回值：返回socket描述符
 **
 **
 ** *********************************************/
int init_socket_server(char *confname)
{
        int     listenfd;
        int     opt = 1;
        char    ip[20];
        int     port;
        pid_t   childpid;
        struct sockaddr_in  servaddr;

        if(get_ip_port(confname, ip, &port) < 0)
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

        printf("send file %s over\n\n", filename);
        return 0;
}


