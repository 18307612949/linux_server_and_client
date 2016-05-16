#include "utility.h"
#include<map>

map<int ,CLIENT> clients_map;

int main(int argc, char *argv[])
{
    //������IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    struct rlimit rt;//��Դ���Ʒ�
    //����ÿ����������򿪵�����ļ���
    rt.rlim_max=rt.rlim_cur=EPOLL_SIZE;
    if(setrlimit(RLIMIT_NOFILE,&rt)==-1)
    {
        perror("setrlimt error.\n");
        return -1;
    }
    //��������socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("listener");
        return -1;
    }
    printf("listen socket created \n");
    //�󶨵�ַ
    if( bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    //����
    int ret = listen(listener, 5);
    if(ret < 0)
    {
        perror("listen error");
        return -1;
    }
    printf("Start to listen: %s\n", SERVER_IP);
    //���ں��д����¼���
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        perror("epfd error");
        return -1;
    }
    printf("epoll created, epollfd = %d\n", epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //���ں��¼���������¼�
    addfd(epfd, listener, true);
    //��ѭ��
    while(1)
    {
        //epoll_events_count��ʾ�����¼�����Ŀ
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0)
        {
            perror("epoll failure");
            break;
        }
        //������epoll_events_count�������¼�
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //���û�����
            if(sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);

                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );
                printf("client connection from: %s : % d(IP : port), socketfd = %d \n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), clientfd);

                addfd(epfd, clientfd, true);

                // �������list�����û�����
                CLIENT client;
                client.socketfd=clientfd;
                clients_map[clientfd]=client;
                printf("Now there are %d clients int the chat room\n\n", (int)clients_map.size());//zsd
            }
            //�����û���������Ϣ
            else
            {
                // buf[BUF_SIZE] receive new chat message
                char buf[BUF_SIZE];
                bzero(buf, BUF_SIZE);
                // receive message
                int len = recv(sockfd, buf, BUF_SIZE, 0);
                buf[len+1]='\0';//zsd
                if(len == 0) // len = 0 means the client closed connection//ò�Ʋ�����
                {
                    close(sockfd);
                    delfd(epfd, sockfd, true);/////////////////////
                    //clients_list.remove(sockfd); //server remove the client
                    map<int,CLIENT>::iterator map_it;
                    map_it=clients_map.find(sockfd);
                    printf("ClientID = %d closed.\n now there are %d client in the char room\n", clients_map[sockfd].ID, (int)clients_map.size()-1);//zsd
                    clients_map.erase(map_it);
                }
                else if(len < 0)
                {
                    perror("error");
                    return -1;
                }
                else
                {
                    char order[ORDER_LEN+1],message[BUF_SIZE];
                    bzero(order, ORDER_LEN+1);
                    bzero(message, BUF_SIZE);
                    strncat(order,buf,ORDER_LEN);
                    strcat(message,&buf[ORDER_LEN]);
                    printf("order= %s\n",order);
                    if(strcmp(order,"00")==0)//���սṹ��
                    {
                        CLIENT client;
                        memcpy(&client,message,sizeof(message));
                        clients_map[sockfd]=client;
                        printf("ClientID = %d comes.\n", clients_map[sockfd].ID);
                        //send back message
                        char message_send[BUF_SIZE];
                        bzero(message_send, BUF_SIZE);
                        // format message
                        sprintf(message_send, "Server received ClientID=%d 's message.\n",clients_map[sockfd].ID);
                        if( send(sockfd, message_send, BUF_SIZE, 0) < 0 )
                        {
                            perror("error");
                            return -1;
                        }
                    }
                    else if(strcmp(order,"-1")==0)//��socket�˳�
                    {
                        close(sockfd);
                        delfd(epfd, sockfd, true);/////////////////////
                        map<int,CLIENT>::iterator map_it;
                        map_it=clients_map.find(sockfd);
                        printf("ClientID = %d closed.\n now there are %d client in the char room\n", clients_map[sockfd].ID, (int)clients_map.size()-1);//zsd
                        clients_map.erase(map_it);
                    }
                    else
                    {
                        if(clients_map[sockfd].ID==-1)
                        {
                            char message_send[BUF_SIZE];
                            bzero(message_send, BUF_SIZE);
                            sprintf(message_send, "Server says: Please send the client's info first.\n");
                            send(sockfd, message_send, BUF_SIZE, 0);
                        }
                        else
                        {
                            printf("ClientID = %d says: %s\n", clients_map[sockfd].ID,message);
                            char message_send[BUF_SIZE];
                            bzero(message_send, BUF_SIZE);
                            sprintf(message_send, "Server received ClientID=%d 's message.\n",clients_map[sockfd].ID);
                            send(sockfd, message_send, BUF_SIZE, 0);
                        }
                    }
                }
            }
        }
    }
    close(listener); //�ر�socket
    close(epfd); //�ر��ں�
    return 0;
}
