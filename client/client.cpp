#include "utility.h"
#include<vector>

const int CLIENTNUM=101;
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

    for(int c_i=1; c_i<CLIENTNUM+1; c_i++)//���4���ᴴ�����ˣ�������������
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

        //��sock�͹ܵ���������������ӵ��ں��¼�����
        addfd(epfd, sockets[c_i], true);
    }

    // Fork
    int pid = fork();
    if(pid < 0)
    {
        perror("fork error");
        return -1;
    }
    else if(pid == 0)   // �ӽ���
    {
        sleep(1);
        while(true)
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

            printf("Please input the code: ");
            // ������Ϣ������
            char message[BUF_SIZE];
            cin>>message;
            cout<<"code = "<<message<<endl;
            CLIENT client;
            client.ID=ID;
            client.socketfd=sockets[ID];
            strcpy(client.code,message);
            // ����Ϣ���͸������
            char client_msg[BUF_SIZE];
            memcpy(client_msg,&client,sizeof(CLIENT));
            send(sockets[ID],client_msg, BUF_SIZE, 0);
        }
    }
    else   //pid > 0 ������
    {
        bool isClientwork=true;
        while(isClientwork)
        {
            int epoll_events_count = epoll_wait( epfd, events, EPOLL_SIZE, -1 );
            //��������¼�
            for(int i = 0; i < epoll_events_count ; ++i)
            {
                // ������Ϣ������
                char message[BUF_SIZE];
                bzero(&message, BUF_SIZE);

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

            }//for
        }//while
    }


    return 0;
}
