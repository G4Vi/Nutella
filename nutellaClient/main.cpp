#include "main.h"
using namespace std;

struct globalArgs_t
{
    int usage;          //-h
    char* rate;         //-r
} args;


class connection
{
    string m_ipAddress;
    string m_port;
    int m_UniSocket;
    struct addrinfo *m_results;
    int m_sendSocket;
    int m_recvSocket;
    char m_message[MESSAGE_LEN];
    char m_frame[MAX_MESSAGE_SIZE];

    void setupSender()
    {
        /* set up socket */
        if ((m_sendSocket=msockcreate(SEND, EXAMPLE_ADDR, EXAMPLE_PORT)) < 0) {
            perror("msockcreate");
            exit(1);
        }
    }

    void setupReceiver()
    {
        /* set up socket */
        if ((m_recvSocket=msockcreate(RECV, EXAMPLE_ADDR, RECV_PORT)) < 0) {
            perror("msockcreate");
            exit(1);
        }
    }


    int getSocket(string host,  string port_number, struct addrinfo **results)
    {
        int tcp_socket;
        struct addrinfo socket_info; connection* m_conn;
        memset(&socket_info, 0, sizeof socket_info); //clear struct to be safe
        socket_info.ai_family = AF_UNSPEC; //compatible with IPV4 and 6
        socket_info.ai_socktype = SOCK_STREAM; //TCP
        socket_info.ai_flags  = AI_PASSIVE;

        //setup the socket
        int status = getaddrinfo(host.c_str(),port_number.c_str(),&socket_info, results);

        if(status != 0) //Check getaddrinfo worked before proceeding with bad information
        {
            printf("getaddrinfo failed!\n error: %s\n", gai_strerror(status));
            exit(1);
        }

        if ((tcp_socket = socket((*results)->ai_family, (*results)->ai_socktype, (*results)->ai_protocol)) == -1)
        {
            printf("Socket opening error\n");
            exit(1);
        }

        return tcp_socket;
    }

    int processLookUp(char* message)
    {
        //verify the integrity of
        m_ipAddress = message;
        m_port = message;
        int colon = m_ipAddress.find_first_of(':');
        if(colon == -1)
        {
            cerr << "Invalid connection info recieved, exiting" << endl;
            return -1;
        }
        m_ipAddress = m_ipAddress.substr(0, colon);
        m_port = m_port.substr(colon+1);

        cout << "Received IP and port from server" << endl;
        return 0;
    }

    int recvLookupResponse()
    {
        int maxfd, cnt;			/* max file descriptors */
        fd_set rset;			/* set of file desc to check */
        struct timeval alarm;		/* for timer */
        time_t start = time(0);
        cnt = 0;

        /* initialize set for select */
        FD_ZERO(&rset);

        while(difftime(time(0), start) < 3) //see if 3 seconds have gone by
        {
            /* we want either the socket or timer */
            FD_SET(m_recvSocket, &rset);

            /* need the largest */
            maxfd = m_recvSocket + 1;

            /* set timer to go off at a certain time */
            alarm.tv_sec = 0;
            alarm.tv_usec = 333333;

            if (select(maxfd, &rset, NULL, NULL, &alarm) == -1) {
                perror("select failed");
            }

            /* something went off, let's see if socket */
            if (FD_ISSET(m_recvSocket, &rset)) { /* socket is readable */
                cnt = mrecv(m_recvSocket, m_message, MESSAGE_LEN);

                if(cnt < 0)
                {
                    perror("mrecv");
                    exit(1);
                }
                return 0;
            }

        }
        return -1;
    }

    void clearAndPrint()
    {
        //clear screen
        printf("\033[2J");
        printf("\033[0;0f");
        cout.flush();

        //print
        cout << m_frame;
    }

public:

    connection()
    {
        //sets up multicast
        setupSender();
        setupReceiver();

    }

    int request(string searchTerm)
    {
        //send unicast request
        if (connect(m_UniSocket, m_results->ai_addr, m_results->ai_addrlen) != -1) //Check if connect succeeds
        {
            if (send(m_UniSocket, searchTerm.c_str(), searchTerm.length()+1, 0)!= -1) //Send searchTerm
            {
                return 0;
            }
            return -1;
        }
        else
        {
            cerr << "Connect to '" << m_ipAddress << "'failed!"<<endl;
            perror("connect");
            return -1;
        }
    }

    int lookup(string searchTerm)
    {
        strcpy(m_message, searchTerm.c_str());
        int cnt;
        //send multicast message
        cout << "Sending search request" << endl;

        cnt = msend(m_sendSocket, m_message, MESSAGE_LEN);
        if (cnt < 0) {
            perror("msend");
            exit(1);
        }

        memset(m_message,0, MESSAGE_LEN);

        //recieve multicast
        cout << "Waiting for response ..." << endl;

        if(recvLookupResponse() == 0) //recieve the message
        {
            //if recieved
            if(processLookUp(m_message)== 0)
                return 0;
            else
                cout << "Invalid connection info recieved" << endl;
        }
        else
        {
            cout << "Timeout/movie not found" << endl;
        }
        return -1;

    }

    void unicast()
    {
        m_UniSocket = getSocket(m_ipAddress, m_port, &m_results);
    }

    void clean()
    {
        close(m_UniSocket);
        freeaddrinfo(m_results);
    }

    void recvUni(long micro)
    {

        int maxfd, cnt;			/* max file descriptors */
        fd_set rset;			/* set of file desc to check */
        struct timeval alarm;		/* for timer */

        /* initialize set for select */
        FD_ZERO(&rset);

        for(;;)
        {
            /* we want either the socket or timer */
            FD_SET(m_UniSocket, &rset);

            /* need the largest */
            maxfd = m_UniSocket + 1;

            /* set timer to go off at a certain time */
            alarm.tv_sec = 0;
            alarm.tv_usec = micro;

            if (select(maxfd, &rset, NULL, NULL, &alarm) == -1) {
                perror("select failed");
            }

            /* something went off, let's see if socket */
            if (FD_ISSET(m_UniSocket, &rset)) { /* socket is readable */
                cnt = read(m_UniSocket, m_frame, MAX_MESSAGE_SIZE);
                clearAndPrint();
                if(cnt > 0)
                {
                    memset(m_frame, 0, MAX_MESSAGE_SIZE);
                }
                else
                    break;      //shows over folks or server just decides to quit on you
            }

        }
    }

};

class video
{
    connection* m_conn;

    long framerate(int fps) //turns fps into a time value
    {
        if(fps > 120)
        {
            cerr << "FPS way too high, exiting" << endl;
            exit(EXIT_FAILURE);
        }
        static long sec_to_usec = 1000000;
        double seconds = (double)1/fps;
        return (seconds * sec_to_usec);
    }

public:
    void stream(connection* conn, const char* fps)
    {
        m_conn = conn;

        //clear screen
        printf("\033[2J");
        printf("\033[0;0f");
        cout.flush();

        //play movie
        conn->recvUni(framerate(atoi(fps)));
        cout << "Movie done" << endl;

    }
};

void usage()
{
    cout << "Usage: nutellaServer [flags], where flags are:" << endl;
    cout << "  -r #      frames per second to send at, (default is 3)" << endl;
    cout << "  -h        display this help and exit" << endl << endl;
    cout << endl<< "Program by Gavin Hayes <gahayes@wpi.edu> for CS4513 C16" << endl;

    exit( EXIT_FAILURE );
}

void setupArgs(int argc, char** argv)
{
    //getopt
    int opt = 0;
    args.usage = 0;
    args.rate = NULL;

    while((opt = getopt(argc, argv, "hr:")) != -1)
    {
        switch(opt){
        case 'h':
            args.usage = 1;
            usage();
            break;
        case 'r':
            args.rate = optarg;
            break;

        default:
            //won't go here
            break;
        }
    }
}

int main(int argc, char** argv)
{
    //initialization
    string searchTerm;
    string rate = "3";
    video vplayer;

    setupArgs(argc, argv);
    if(args.rate != NULL)
        rate = args.rate;


    cout << "Welcome to Nutella Client" << endl;
    connection conn;                                          //setup a multicast connection


    for(;;)
    {
        cout << "Enter Movie Name: ";
        getline(cin, searchTerm);                               //get query from user

        if(searchTerm == "/exit")
            break;

        if(conn.lookup(searchTerm) != -1)                       //see if movie availible
        {
            conn.unicast();                                     //setup for unicast

            if(conn.request(searchTerm) != -1)                  //request movie
            {

                vplayer.stream(&conn, rate.c_str());                     //play the video
            }
            conn.clean();
        }
    }
    return 0;
}

