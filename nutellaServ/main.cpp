  #include "main.h"
  using namespace std;

  struct globalArgs_t
  {
      int usage;          //-h
      char* port;         //-p
      char* dir;          //-d
      char* unip;         //-a
      char* rate;         //-r

  } args;

  class connection
  {
      int m_sendSocket;
      int m_recvSocket;
      int m_uniSocket;
      int m_clientfd;
      char m_message[MESSAGE_LEN];
      string m_UniIP;
      string m_UniPort;

      int getSocket(string port_number)
      {
	  int tcp_socket;
	  struct addrinfo socket_info, *results;
	  memset(&socket_info, 0, sizeof socket_info); //clear struct to be safe
	  socket_info.ai_family = AF_UNSPEC; //compatible with IPV4 and 6
	  socket_info.ai_socktype = SOCK_STREAM; //TCP
	  socket_info.ai_flags  = AI_PASSIVE;

	  //setup the socket
	  int status = getaddrinfo(NULL,port_number.c_str(),&socket_info, &results);

	  if(status != 0) //Check getaddrinfo worked before proceeding with bad information
	  {
	      printf("getaddrinfo failed!\n error: %s\n", gai_strerror(status));
	      exit(1);
	  }

	  if ((tcp_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == -1)
	  {
		  printf("Socket opening error\n");
	      perror("socket");
	      exit(1);
	  }

	  if(bind(tcp_socket, results->ai_addr, results->ai_addrlen) < 0)
	  {
	      printf("Socket binding error\n");
	      perror("bind");
	      exit(1);
	  }

	  if (listen(tcp_socket, 20) != 0)
	  {
	      printf("Socket listen error\n%s\n", strerror(errno));
	      exit(1);
	  }

	  freeaddrinfo(results); //Free linked list as its no longer used;
	  return tcp_socket;
      }

  public:
      connection(string ipAddress, string port)
      {
	  m_UniIP = ipAddress;
	  m_UniPort = port;
      }
      void setupReceiver()
      {
	  /* set up socket */
	  if ((m_recvSocket=msockcreate(RECV, EXAMPLE_ADDR, EXAMPLE_PORT)) < 0) {
	      perror("msockcreate");
	      exit(1);
	  }
      }

      void receiveQuery()
      {
	  cout << "Listening..." << endl;
	  int cnt = 0;
	  /* receiver plays out messages */
	  while (cnt == 0) {
	      cnt = mrecv(m_recvSocket, m_message, MESSAGE_LEN);
	      if (cnt < 0) {
		  perror("mrecv");
		  exit(1);
	      }
	  }
	  cout << "Received search request for '" << m_message << "'\n";
      }

      string Message()
      {
	  return m_message;
      }

      void setupSender()
      {
	  /* set up socket */
	  if ((m_sendSocket=msockcreate(SEND, EXAMPLE_ADDR, SEND_PORT)) < 0) {
	      perror("msockcreate");
	      exit(1);
	  }
      }

      void sendResponse() //send out unicast details
      {
	  int cnt;
	  string message = m_UniIP + ":" + m_UniPort;
	  strcpy(m_message, message.c_str());
	  cout << "Sending IP " << m_UniIP << ", port " << m_UniPort << " to client" << endl;
	  cnt = msend(m_sendSocket, m_message, message.length()+1);
	  if (cnt < 0) {
	      perror("msend");
	      exit(1);
	  }
      }

      void setupUnicast()
      {
	  m_uniSocket = getSocket(m_UniPort);
      }

      void closeCFD()
      {
	  close(m_clientfd);
      }

      int recvPlay()
      {
	  int cnt;
	  struct sockaddr_in client_addr;
	  socklen_t addrlen =sizeof(client_addr);
	  memset(m_message, 0, MESSAGE_LEN);
	  cout << "Listening ..." << endl;
	  m_clientfd = accept(m_uniSocket, (struct sockaddr*)&client_addr, &addrlen); //accept connection
	  if(m_clientfd != -1)
	  {
	      if((cnt = read(m_clientfd, m_message, MESSAGE_LEN))> 0) //print output
	      {
		  cout << "Request recieved." << endl;
		  return 0;
	      }
	      cout << "Message read failed" << endl;
	      return -1;
	  }
	  perror("accept");
	  cout << "Connection accept failed" << endl;
	  return -1;
      }

      int sendFrame(string frame)
      {
	  if (send(m_clientfd, frame.c_str(), frame.length()+1, MSG_NOSIGNAL)!= -1) //Send frame
	  {
	      return 0;
	  }
	  return -1;
      }



  };

  class media
  {
      vector<string> m_movieList;
      string libraryDir;
      long m_framesInNano;

      long framerate(int fps)          //turns fps into time
      {
	  if(fps > 120)
	  {
	      cerr << "FPS way too high, exiting" << endl;
	      exit(EXIT_FAILURE);
	  }
	  static long sec_to_nsec = 1000000000;
	  double seconds = (double)1/fps;
	  return (seconds * sec_to_nsec);
      }

  public:
      media(const char* libraryDir, const char* fps)
      {
	  this->libraryDir = libraryDir; //determine movie in library
	  DIR *dir;
	  struct dirent *ent;
	  if ((dir = opendir (libraryDir)) != NULL) {
	      while ((ent = readdir (dir)) != NULL) {
		  m_movieList.push_back(ent->d_name);
	      }
	      closedir (dir);
	  } else {
	      perror ("");
	      exit(EXIT_FAILURE);
	  }
	  m_framesInNano = framerate(atoi(fps));
      }

      int queryLibrary(string message)
      {
	  int i;
	  for(i = 0; i < m_movieList.size(); i++)
	  {
	      string movieName = m_movieList[i];
	      movieName = movieName.substr(0, movieName.find_last_of(".txt")-3);
	      if(message == movieName)
	      {
		  cout << "Movie here" << endl;
		  return 0;
	      }
	  }
	  cout << "Movie not here, so no response" << endl;
	  return -1;
      }

      void playMovie(connection* conn)
      {
	  int start, framesize;
	  string path;
	  string frame, file;
	  struct timespec tim, tim2;
	  tim.tv_sec = 0;
	  tim.tv_nsec = m_framesInNano;

	  cout << "Streaming movie "<< conn->Message() << " to client ..." << endl;

	  //get the file path and open
	  path = libraryDir + "/" + conn->Message() + ".txt";
	  ifstream ifs (path.c_str());

	  //read into a string
	  if (ifs)
	  {
	      std::ostringstream contents;
	      contents << ifs.rdbuf();
	      ifs.close();
	      file = contents.str();
	  }
	  else
	      cerr << "IO error" << endl;
	  ifs.close();


	  //parse string for frames and send
	  start = 0;
	  for(;;)
	  {
	      try
	      {
		  framesize = file.find("\nend\n", start);
		  if(framesize == -1)
		  {
		      cout << "Movie done or invalid movie format"<< endl;
		      break;
		  }
		  framesize -= start;

		  frame = file.substr(start, framesize+1);

		  if(conn->sendFrame(frame) == -1) //if send fails stop sending the movie
		  {
		      cout << "Send failed,  client probably closed window with movie playing." << endl;
		      break;
		  }

		  nanosleep(&tim, &tim2);
		  start+=framesize;
		  start += 5; //length of \nend\n
	      }
	      catch(const std::out_of_range& oor)
	      {
		  cout << "Movie done"<< endl;
		  break;
	      }

	  }
	  conn->closeCFD();    //don't need that socket anymore
      }
  };

  void usage()
  {
      cout << "Usage: nutellaServer [flags], where flags are:" << endl;
      cout << "  -p #      port to unicast (default is 6666)" << endl;
      cout << "  -d dir    directory to serve movie out of" << endl;
      cout << "  -a ipadddress, ipaddress to send to client for unicasting, (default is 127.0.0.1)"<< endl;
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
      args.port = NULL;
      args.dir = NULL;
      args.unip = NULL;
      args.rate = NULL;

      while((opt = getopt(argc, argv, "hd:p:r:a:")) != -1)
      {
	  switch(opt){
	  case 'h':
	      args.usage = 1;
	      usage();
	      break;
	  case 'd':
	      args.dir = optarg;
	      break;
	  case 'p':
	      args.port = optarg;
	      break;
	  case 'r':
	      args.rate = optarg;
	      break;
	  case 'a':
	      args.unip = optarg;
	      break;

	  default:
	      //won't go here
	      break;
	  }
      }
  }

  int main(int argc, char** argv)
  {
      //defaults
      string ipaddress = "127.0.0.1";
      string port = "6666";     
      string rate = "3";
      string dir = "";

      //from commandline
      setupArgs(argc, argv);
      if(args.port != NULL)
	  port = args.port;
      
      if(args.dir != NULL)
	dir = args.dir;

      if(args.rate != NULL)
	  rate = args.rate;

      if(args.unip != NULL)
	  ipaddress = args.unip;

      connection conn(ipaddress, port);      
      
	if(chdir(dir.c_str())== -1)
	{
	      perror("chdir");
	      cerr << "No valid movies directory specified" << endl;
	      exit(EXIT_FAILURE);
	}
	  
      media med(dir.c_str(), rate.c_str());

      cout << "Nutella Server started." << endl;

      conn.setupReceiver();
      conn.setupSender();
      conn.setupUnicast();
      for(;;)
      {
	  conn.receiveQuery();                          //listen for movie request
	  if(med.queryLibrary(conn.Message()) == 0)     //check library
	  {
	      conn.sendResponse();                      //inform client if in library
	      if(conn.recvPlay() == 0)                  //recieved unicast connection with request of movie
	      {
		  med.playMovie(&conn);                 //unicast movie to client
	      }
	  }
      }
      return 0;
  }

