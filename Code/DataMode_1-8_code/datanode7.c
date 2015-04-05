#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<string.h>

int main()
{
	struct sockaddr_in serv; //for connection with server
	
	int sfd,i,j;
	bzero(&serv,sizeof(serv));
	sfd=socket(AF_INET,SOCK_STREAM,0);
	serv.sin_addr.s_addr=htonl(INADDR_ANY);
	serv.sin_family=AF_INET;
	serv.sin_port=htons(3134);

	connect(sfd,(struct sockaddr*)&serv,sizeof(serv));
	perror("connect");
	send(sfd,"me",2,0);

	sleep(100);

return 0;
}

