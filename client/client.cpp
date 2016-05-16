#include "utility.h"
#include<map>

const int CLIENTNUM=3001;//��1��ʼ
map<int,int> map_ID_sockets;//��1��ʼ
map<int,CLIENT> map_socket_clients;

int main(int argc, char *argv[])
{
    //�û����ӵķ����� IP + port
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
    // ����epoll
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        perror("epfd error");
        return -1;
    }
    static struct epoll_event events[EPOLL_SIZE];

    for(int c_i=1; c_i<CLIENTNUM+1; c_i++)//��1��ʼ
    {
        // ����socket
        map_ID_sockets[c_i]= socket(PF_INET, SOCK_STREAM, 0);
        if(map_ID_sockets[c_i]< 0)
        {
            perror("sock error");
            return -1;
        }
        // ���ӷ����
        if(connect(map_ID_sockets[c_i], (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("connect error");
            return -1;
        }

        //��sock��ӵ��ں��¼�����
        addfd(epfd, map_ID_sockets[c_i], true);
    }

    // �����ܵ�������fd[0]���ڸ����̶���fd[1]�����ӽ���д
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0)
    {
        perror("pipe error");
        return -1;
    }

    //���ܵ�������������ӵ��ں��¼�����
    addfd(epfd, pipe_fd[0], true);

    // Fork
    int pid = fork();
    if(pid < 0)
    {
        perror("fork error");
        return -1;
    }
    else if(pid == 0)   // �ӽ���
    {
        //�ӽ��̸���д��ܵ�����˹رն���
        close(pipe_fd[0]);

        bool isClientwork=true;
        while(isClientwork)
        {
            sleep(1);
            //cout<<"end sleep"<<endl;
            printf("Please input the sockets' ID: ");

            int ID;
            cin>>ID;
            if(cin.fail())
            {
                cout<<"input is wrong, please try again."<<endl;
                cin.clear();
                cin.ignore(4096,'\n');
                continue;
            }

            //�б�ID�Ƿ���ڵĹ�������������

            printf("Please input the order: ");
            // ������Ϣ������
            char order[BUF_SIZE];
            bzero(order, BUF_SIZE);
            cin>>order;
            char message[BUF_SIZE];
            bzero(message, BUF_SIZE);
            if(strcmp(order,"00")==0)
            {
                sprintf(message,"%s %d %s",order,ID," ");
            }
            else if(strcmp(order,"-1")==0)//�رյ�ǰsocket
            {
                sprintf(message,"%s %d %s",order,ID," ");
            }
            else
            {
                printf("Please input the message: ");
                // ������Ϣ������
                char msg[BUF_SIZE];
                bzero(msg, BUF_SIZE);
                cin>>msg;
                sprintf(message,"%s %d %s",order,ID,msg);
            }
            // �ӽ��̽���Ϣд��ܵ�
            if( write(pipe_fd[1], message, strlen(message)  ) < 0 )//zsd
            {
                perror("fork error");
                return -1;
            }
        }
    }
    else   //pid > 0 ������
    {
        //�����̸�����ܵ����ݣ�����ȹر�д��
        close(pipe_fd[1]);
        bool isClientwork=true;
        while(isClientwork)
        {
            int epoll_events_count = epoll_wait( epfd, events, EPOLL_SIZE, -1 );
            //��������¼�
            for(int i = 0; i < epoll_events_count ; ++i)
            {
                // ������Ϣ������
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                if(events[i].data.fd == pipe_fd[0])
                {
                    char order[BUF_SIZE];
                    bzero(order, BUF_SIZE);
                    int ID;
                    char msg[BUF_SIZE];
                    bzero(msg, BUF_SIZE);
                    //�ӹܵ����ӽ��̼��������ָ��
                    int ret = read(events[i].data.fd, message, BUF_SIZE);
                    if(ret == 0)
                    {
                        //isClientwork = 0;
                    }
                    else
                    {
                        sscanf(message,"%s %d %s",order,&ID,msg);
                        //printf("msg = %s\n",msg);
                        map<int,int>::iterator map_int_int_it;
                        map_int_int_it=map_ID_sockets.find(ID);
                        if(map_int_int_it==map_ID_sockets.end() ) //û�ҵ�
                        {
                            printf("This ID's socket can't find, you must enroll(00) it. ID = %d\n",ID);
                            // ����socket
                            map_ID_sockets[ID]= socket(PF_INET, SOCK_STREAM, 0);
                            // ���ӷ����
                            if(connect(map_ID_sockets[ID], (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
                            {
                                perror("connect error");
                                return -1;
                            }
                            //��sock��ӵ��ں��¼�����
                            addfd(epfd, map_ID_sockets[ID], true);
                        }
                        else
                        {
                            if(strcmp(order,"00")==0)
                            {
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                CLIENT client;
                                client.ID=ID;
                                client.live_sec=10;
                                client.socketfd=map_ID_sockets[ID];
                                map_socket_clients[map_ID_sockets[ID]]=client;
                                // ����Ϣ���͸������
                                char client_info[BUF_SIZE];
                                bzero(client_info, BUF_SIZE);
                                memcpy(client_info,&client,sizeof(CLIENT));
                                strcat(send_message,order);
                                strcat(&send_message[ORDER_LEN],client_info);
                                send(map_ID_sockets[ID],send_message, BUF_SIZE, 0);
                                printf("ID = %d, socket = %d, send message: %s\n",ID,map_ID_sockets[ID],send_message);
                            }
                            else if(strcmp(order,"-1")==0)//�رյ�ǰsocket
                            {
                                map<int,CLIENT>::iterator map_int_client_it;
                                map_int_client_it=map_socket_clients.find(map_ID_sockets[ID]);
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                strcat(send_message,order);
                                strcat(&send_message[ORDER_LEN],msg);
                                send(map_ID_sockets[ID],send_message, BUF_SIZE, 0);
                                //printf("send message: %s\n",send_message);
                                close(map_ID_sockets[ID]);
                                delfd(epfd, map_ID_sockets[ID], true);/////////////////////
                                printf("ClientID = %d closed.\n", ID);//zsd
                                map_ID_sockets.erase(map_int_int_it);
                                map_socket_clients.erase(map_int_client_it);
                            }
                            else
                            {
                                char send_message[BUF_SIZE];
                                bzero(send_message, BUF_SIZE);
                                strcat(send_message,order);
                                strcat(&send_message[ORDER_LEN],msg);
                                send(map_ID_sockets[ID],send_message, BUF_SIZE, 0);
                                printf("send message: %s\n",send_message);
                            }
                        }
                    }
                }
                else//�ӷ��������յ�
                {
                    //����˷�����Ϣ
                    int sock=events[i].data.fd;
                    //���ܷ������Ϣ
                    int ret = recv(sock, message, BUF_SIZE, 0);

                    // ret= 0 ����˽���socket�ر�
                    if(ret == 0)
                    {
                        map<int,int>::iterator map_int_int_it;
                        for(map_int_int_it=map_ID_sockets.begin(); map_int_int_it!=map_ID_sockets.end(); ++map_int_int_it)
                        {
                            if(map_int_int_it->second==sock) break;
                        }
                        if(map_int_int_it==map_ID_sockets.end()) printf("Can't find the socket to close.\n");
                        else
                        {
                            map_ID_sockets.erase(map_int_int_it);
                        }
                        map<int,CLIENT>::iterator map_int_client_it;
                        map_int_client_it=map_socket_clients.find(sock);
                        map_socket_clients.erase(map_int_client_it);
                        close(sock);//��֪Ҫ��Ҫ�����ظ��ر�
                        delfd(epfd, sock, true);/////////////////////
                        printf("Server closed connection: %d\n", sock);
                    }
                    else printf("%s\n", message);
                }
            }//for
        }//while
    }

    if(pid)
    {
        //�رո����̺�sock
        printf("close the father thread.\n");
        close(pipe_fd[0]);
        //close(sock);
    }
    else
    {
        //�ر��ӽ���
        printf("close the son thread.\n");
        close(pipe_fd[1]);
    }

    return 0;
}
