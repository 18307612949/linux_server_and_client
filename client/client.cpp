#include "utility.h"
#include<vector>

const int CLIENTNUM=1001;
vector<int> sockets(CLIENTNUM);

int main(int argc, char *argv[])
{
    //�û����ӵķ����� IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // ����epoll
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        perror("epfd error");
        return -1;
    }
    static struct epoll_event events[EPOLL_SIZE];

    for(int c_i=1; c_i<CLIENTNUM+1; c_i++)//���4���ᴴ�����ˣ������������롣ò�ƽ����
    {
        // ����socket
        sockets[c_i]= socket(PF_INET, SOCK_STREAM, 0);
        if(sockets[c_i] < 0)
        {
            perror("sock error");
            return -1;
        }
        // ���ӷ����
        if(connect(sockets[c_i], (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("connect error");
            return -1;
        }

        //��sock��ӵ��ں��¼�����
        addfd(epfd, sockets[c_i], true);
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
            cout<<"ID = "<<ID<<endl;

            if(ID<1 || ID>CLIENTNUM)
            {
                cout<<"ID is error"<<endl;
                continue;
            }

            printf("Please input the order: ");
            // ������Ϣ������
            char order[BUF_SIZE];
            bzero(order, BUF_SIZE);
            cin>>order;
            //cout<<"code = "<<message<<endl;
            char message[BUF_SIZE];
            bzero(message, BUF_SIZE);
            if(strcmp(order,"00")==0)
            {
                sprintf(message,"%s %d %s",order,ID," ");
                /*CLIENT client;
                client.ID=ID;
                client.socketfd=sockets[ID];
                //strcpy(client.code,message);
                // ����Ϣ���͸������
                char client_info[BUF_SIZE];
                bzero(client_info, BUF_SIZE);
                memcpy(client_info,&client,sizeof(CLIENT));
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                strcat(message,order);
                strcat(&message[ORDER_LEN],client_info);*/
                //send(sockets[ID],message, BUF_SIZE, 0);
            }
            else if(strcmp(order,"-1")==0)//�رյ�ǰsocket
            {
                sprintf(message,"%s %d %s",order,ID,"END");
            }
            else
            {
                printf("Please input the message: ");
                // ������Ϣ������
                char msg[BUF_SIZE];
                bzero(msg, BUF_SIZE);
                //char message[BUF_SIZE];
                //bzero(message, BUF_SIZE);
                cin>>msg;
                //strcat(message,order);
                //strcat(&message[ORDER_LEN],msg);
                //send(sockets[ID],message, BUF_SIZE, 0);
                //printf("send message: %s\n",message);
                sprintf(message,"%s %d %s",order,ID,msg);
            }
            // �ӽ��̽���Ϣд��ܵ�
            if( write(pipe_fd[1], message, strlen(message)  ) < 0 )//zsd
            {
                perror("fork error");
                return -1;
            }
            //printf("send message: %s\n",message);
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
                    char IDchars[BUF_SIZE];
                    bzero(IDchars, BUF_SIZE);
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
                        printf("msg = %s\n",msg);
                        //ID=atoi(IDchars);
                        if(strcmp(order,"00")==0)
                        {
                            char send_message[BUF_SIZE];
                            bzero(send_message, BUF_SIZE);
                            CLIENT client;
                            client.ID=ID;
                            client.socketfd=sockets[ID];
                            // ����Ϣ���͸������
                            char client_info[BUF_SIZE];
                            bzero(client_info, BUF_SIZE);
                            memcpy(client_info,&client,sizeof(CLIENT));
                            strcat(send_message,order);
                            strcat(&send_message[ORDER_LEN],client_info);
                            send(sockets[ID],send_message, BUF_SIZE, 0);
                            printf("send message: %s\n",send_message);
                        }
                        else if(strcmp(order,"-1")==0)//�رյ�ǰsocket
                        {
                            //
                        }
                        else
                        {
                            char send_message[BUF_SIZE];
                            bzero(send_message, BUF_SIZE);
                            strcat(send_message,order);
                            strcat(&send_message[ORDER_LEN],msg);
                            send(sockets[ID],send_message, BUF_SIZE, 0);
                            printf("send message: %s\n",send_message);
                        }
                    }
                }
                else//�ӷ��������յ�
                {
                    // ������Ϣ������
                    //char message[BUF_SIZE];
                    //bzero(&message, BUF_SIZE);//?

                    //����˷�����Ϣ
                    int sock=events[i].data.fd;
                    //���ܷ������Ϣ
                    int ret = recv(sock, message, BUF_SIZE, 0);

                    // ret= 0 ����˹ر�
                    if(ret == 0)
                    {
                        printf("Server closed connection: %d\n", sock);
                        //�����˳����Զ��ر�����socket
                        isClientwork = false;
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
        //close(pipe_fd[0]);
        //close(sock);
    }
    else
    {
        //�ر��ӽ���
        printf("close the son thread.\n");
        //close(pipe_fd[1]);
    }

    return 0;
}
