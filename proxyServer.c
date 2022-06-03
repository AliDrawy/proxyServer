#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "threadpool.h"
char** arr_filter;
int filt_count;
//change the capital letter to small letter
char small_letter(char n)
{
    if(n >= 65 && n <=90){
        n+=32;
    }


    return n;
}
//change the decimal number to binary number
void  decToBinary(char* n0,char binary[8] )
{
// array to store binary number
int n= atoi(n0);
//char binary[8];
int binaryNum[8];
    for (int i = 0; i < 8; ++i) {
        binaryNum[i]=0;
    }

// counter for binary array
int i = 7;
while (n > 0) {
// storing remainder in binary array
binaryNum[i] = n % 2;
n = n / 2;
i--;
}
    for (int j = 0; j < 8; ++j) {
        if(binaryNum[j]==1){
            binary[j]='1';
        }else{
            binary[j]='0';
        }
    }

}
//convert file extension to mime type
char *get_mime_type(char *name)
{
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
}
//make a copy of string from other string
void copy_str( char * str1, char * str2){
    int i = 0;
    for (; i < strlen(str2); ++i) {
        str1[i]=str2[i];
    }
    str1[i]='\0';
}
//send a request that contain error 500 to the client
void error_500(int fd_client){
    char  error_res[]="HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: 144\r\nConnection: close\r\n\r\n"
                      "<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n"
                      "<BODY><H4>500 Internal Server Error</H4>\n"
                      "Some server side error.\n"
                      "</BODY></HTML>\n";
    if(write(fd_client,error_res,244)<0){
        perror("write is failed\n");
        return;
    }



}
//send the file that i opened to the client and  saving it  in the local file system
int read_from_file(int fd,int fd_client,unsigned char  buff[]){
    int size=0,r=0;

    for (int i = 0; i < 10000; ++i) {
        buff[i]='\0';
    }
    r= read(fd,buff,10000);
    if(r<0){
        perror("read is failed\n");
        error_500((fd));
        return 1;
    }
    size+=r;
    while (r>0){
        if(write(fd_client,buff,r)==-1){
            error_500((fd));
            perror("write is failed\n");
            return 1;
        }


        for (int i = 0; i < 10000; ++i) {
            buff[i]='\0';
        }
        r= read(fd,buff,10000);
        if(r<0){
            perror("read is failed\n");
            error_500((fd));
            return 1;
        }
        size+=r;
    }

    return size;
}

//this function that we put it in the dispatch to take it by the thread and do the work
int function(void * new_cli){
     char  request[10000];
    for (int i = 0; i < 10000; ++i) {
        request[i]='\0';
    }
    char * temp_req;


    int status1=0,status2=0,status3=0;
     int cli_fd=*((int *)new_cli);
     int size=0,r;
     char method[]="GET";//1
     char host[]="host:";//2
     char protocol0[]="HTTP/1.0";//3
    char protocol1[]="HTTP/1.1";//3
    char * path;
    char* token;
     int space_num=2;

int sum=0;
    while (1){
        r= (int )read(cli_fd,request+sum,10000);
        if(r<0){
            perror("read is failed\n");
            exit(1);
        }
        sum+=r;
        request[sum]='\0';
        if(r==0||strstr(request,"\r\n\r\n")!=NULL){
            break;
        }
    }

    temp_req=(char *) malloc(sizeof(char )*(sum+1));
    for (int i = 0; i < sum + 1; ++i) {
        if(request[i]==' '&&request[i+1]!=' '){
            space_num++;
        }

    }
    copy_str(temp_req,request);
    temp_req[sum]='\0';
    char** arr=(char **) malloc(sizeof(char*)*space_num);
    token= strtok(temp_req," \r\n");
    int x=0;
    while (token!=NULL){
        arr[x]=(char *) malloc(strlen(token)+1);
        copy_str(arr[x],token);
        token= strtok(NULL," \r\n");
        x++;

    }
    free(temp_req);
    if(x!=5){
        char  error_res[]="HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 113\r\nConnection: close\r\n\r\n"
                          "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
                          "<BODY><H4>400 Bad request</H4>\n"
                          "Bad Request.\n"
                          "</BODY></HTML>";


        if(write(cli_fd,error_res,202)<0){
            perror("write is failed\n");
            return 1;
        }
        for (int i = 0; i < space_num; ++i) {
            free(arr[i]);
        }
        free(arr);

        return 1;



    }

    for (int i = 0; i < x; ++i) {
        if(strcmp(arr[i],method)==0){
        status1=1;
        }
        else if(strcmp(arr[i],protocol0)==0||strcmp(arr[i],protocol1)==0){
            status3=1;
        }else{
            for (int j = 0; j < strlen(arr[i]); ++j) {
                arr[i][j]= small_letter(arr[i][j]);
            }


         }
        if(strcmp(arr[i],host)==0){
            status2=1;
        }

    }

    int w,res_size;
    char response[10000];
    for (int i = 0; i < 10000; ++i) {
        response[i]=0;
    }
    if(strcmp(arr[2],protocol0)!=0&&strcmp(arr[2],protocol1)!=0){
        status3=0;
    }
     if(status2==0||status3==0 ) {
        char error_res[] = "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 113\r\nConnection: close\r\n\r\n"
                           "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
                           "<BODY><H4>400 Bad request</H4>\n"
                           "Bad Request.\n"
                           "</BODY></HTML>";


        if (write(cli_fd, error_res, 202) < 0) {
            perror("write is failed\n");
            return 1;

        }
         for (int i = 0; i < space_num; ++i) {
             free(arr[i]);
         }
         free(arr);

        return 1;
    }
      if(status1==0|| strcmp(arr[0],method)!=0){
         char error_res[]="HTTP/1.0 501 Not supported\r\n"
                          "Content-Type: text/html\r\nContent-Length: 129\r\nConnection: close\r\n\r\n"
                          "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\n"
                          "<BODY><H4>501 Not supported</H4>\n"
                          "Method is not supported.\n"
                          "</BODY></HTML>";
         if(write(cli_fd,error_res,220)<0){
             perror("write is failed\n");
             return 1;

         }
          for (int i = 0; i < space_num; ++i) {
              free(arr[i]);
          }
          free(arr);
         return 1;


    }else{


          struct  sockaddr_in struct_socket;
          struct hostent* hostent;
          int port=80;
          int z;
          unsigned char  buff[10000];
          for (int i = 0; i < 10000; ++i) {
              buff[i]='\0';
          }
          char * HOST;
          HOST=(char *) malloc(strlen(arr[4]) + 2);
          if(HOST == NULL){
              perror("malloc is failed\n");
              error_500((cli_fd));
              return 1;
          }
          int count =0;
          for (int i = 0; i < strlen(arr[1]); ++i) {
              if(arr[1][i]=='/'){
                  count++;
              }

          }
          if(arr[1][strlen(arr[1])-1]=='/'){
              arr[1]= malloc(strlen(arr[1])+strlen("index.html")+1);
              strcat(arr[1],"index.html");
          }
          char** array=(char**) malloc(sizeof(char*)*count);
          char* token0 ;
          int x2=0;
          int path_size= (int )strlen(arr[1])+ (int )strlen(arr[4])+2;
          token0= strtok(arr[1], "/");
          while (token0 != NULL){
              array[x2]=(char *) malloc((int)strlen(token0) + 1);

              strcpy(array[x2], token0);
              token0= strtok(NULL, "/");
              x2++;
          }

          copy_str(HOST, arr[4]);
          struct stat st ={0};
          char*path3=(char*) malloc(path_size);
          copy_str(path3, arr[4]);
          int found=0;

          for (z = 0; z < count ; ++z){
              if(stat(path3, &st) == -1){
                  found=1;
                  break;
              }
              strcat(path3, "/");
              strcat(path3, array[z]);
          }
          if(stat(path3, &st) == -1){
              found=1;
          }
          if(found==0){
              printf("HTTP request =\n%s\nLEN = %d\n", request,(int) strlen(request));
              int fd,r1;
              int size1,read_size=0;
              fd= open(path3, O_RDWR, 0644);
              if(fd == -1) {
                  perror("open is failed\n");
                  error_500((cli_fd));
                  return 1;
              }
              size1= (int )lseek(fd, 0, SEEK_END);
              lseek(fd,0,SEEK_SET);
              int temp=size1,counter=0;
              while (temp!=0){
                  counter++;
                  temp=temp/10;

              }
              for (int i = 0; i < 10000; ++i) {
                  buff[i]='\0';
              }
              lseek(fd,0,SEEK_SET);
              char *headers1=(char*)malloc(strlen("HTTP/1.0 200 OK\r\nContent-length: \r\n")+counter+2);

              sprintf(headers1, "HTTP/1.0 200 OK\r\nContent-length: %d\r\n", size1);
              int h1size= strlen("HTTP/1.0 200 OK\r\nContent-length: \r\n");
              while (size1>0){
                  h1size++;
                  size1=size1/10;
              }
              w=write(cli_fd, headers1, h1size);
              res_size=w;
              if(w < 0){
                  perror("write is failed\n");
                  error_500(cli_fd);
                  return 1;
              }
              free(headers1);
              char*type= get_mime_type(path3);
              if(type!=NULL){
                  char *headers2=(char*)malloc(strlen("Content-type: ")+ strlen(type)+1);
                  sprintf(headers1,"Content-type: %s\r\n",type);
                  w=write(cli_fd, headers2, strlen(headers2));
                  res_size+=w;
                  if(w < 0){
                      perror("write is failed\n");
                      error_500(cli_fd);
                      return 1;
                  }
                  free(type);
                  free(headers2);
              }
              char headers3[]="connection: Close\r\n\r\n";
              w=write(cli_fd, headers3, 22 );
              res_size+=w;
              if(w < 0){
                  perror("write is failed\n");
                  error_500(cli_fd);
                  return 1;
              }
             res_size+= read_from_file(fd, cli_fd, buff);
              printf("File is given from local filesystem\n");
              printf("\n Total response bytes: %d\n",res_size);
              close(fd);

          }
          else if(found==1){
              int fd;
              hostent=gethostbyname(arr[4]);
              if(hostent==NULL) {
                  char error_res[]="HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 112\r\n"
                                   "Connection: close\r\n\r\n"
                                   "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
                                   "<BODY><H4>404 Not Found</H4>\n"
                                   "File not found.\n"
                                   "</BODY></HTML>";
                  if(write(cli_fd, error_res, 199) < 0){
                      perror("write is failed\n");
                      return 1;
                  }

                  for (int i = 0; i < space_num; ++i) {
                      free(arr[i]);
                  }
                  free(arr);
                  for (int i = 0; i < count; ++i) {
                      if(array[i]!=NULL){
                          free(array[i]);
                      }
                  }
                  free(array);
                  free(path3);
                  free(HOST);
                  return 1;
              }

              fd= socket(PF_INET,SOCK_STREAM,0);
              if(fd<0){
                  perror("socket is failed\n");
                  error_500(cli_fd);

                  for (int i = 0; i < space_num; ++i) {
                      free(arr[i]);
                  }
                  free(arr);
                  for (int i = 0; i < count; ++i) {
                      if(array[i]!=NULL){
                          free(array[i]);
                      }
                  }
                  free(array);
                  free(path3);
                  free(HOST);
                  return 1;
              }
              struct_socket.sin_family=AF_INET;
              struct_socket.sin_port= htons(80);
              struct_socket.sin_addr.s_addr=((struct in_addr*)hostent->h_addr)->s_addr;
              char* ip_addr;
             ip_addr= inet_ntoa(*(struct in_addr *)(hostent->h_addr));
               char binary_ip[32];
                  char part_binary[8];
                  char *part;
                  part = strtok(ip_addr, ".");
                  int j = 0;
                  while (part != NULL) {
                      decToBinary(part, part_binary);
                      int k=0;
                      for (; j < 32; ++j) {
                          if(k==8){
                              break;
                          }
                          binary_ip[j]=part_binary[k];
                          k++;

                      }
                      part= strtok(NULL, ".");
                  }

              for (int i = 0; i < filt_count; ++i) {
                  if(arr_filter[i][0]>=48&&arr_filter[i][0]<=57){

                  }else{
                      char* tmp;
                      struct  hostent* hp;
                      hp = gethostbyname(arr_filter[i]);
                      if(hp==NULL){
                          error_500(cli_fd);
                        //  herror("get host py name failed\n");
                          for (int i6 = 0; i6 < space_num; ++i6) {
                              free(arr[i6]);
                          }
                          free(arr);
                          for (int i6 = 0; i6 < count; ++i6) {
                              if(array[i6] != NULL){
                                  free(array[i6]);
                              }
                          }
                          free(array);
                          free(path3);
                          free(HOST);
                          return 1;
                      }
                     tmp= inet_ntoa(*(struct in_addr *)(hp->h_addr));
                     arr_filter[i]=(char*) realloc(arr_filter[i],strlen(tmp)+1);
                      copy_str(arr_filter[i],tmp);
                  }
              }
              for (int i = 0; i < filt_count; ++i) {
                  int founded = 0, sub = 32;
                  for (int l = 0; l < strlen(arr_filter[i]); ++l) {
                      if (arr_filter[i][l] == '/') {
                          founded = 1;
                          break;
                      }
                  }
                  char *ip;
                  if (founded == 1) {
                      char *to;
                      to = strtok(arr_filter[i], "/");
                      ip = (char *) malloc(strlen(to) + 1);
                      strcpy(ip, to);
                      int t = 0;
                      while (t == 0) {
                          to = strtok(NULL, "/");
                          t = 1;
                      }
                      sub = atoi(to);
                  } else {
                      ip = (char *) malloc(strlen(arr_filter[i]) + 1);
                      strcpy(ip, arr_filter[i]);
                  }
                  char binary_filter_ip[32];
                  char part_binary_filter[8];
                  char *part_filter;
                  part_filter = strtok(ip, ".");
                  int j0 = 0;
                  while (part_filter != NULL) {
                      decToBinary(part_filter, part_binary_filter);
                      int k=0;
                      for (; j0 < 32; ++j0) {
                          if(k==8){
                              break;
                          }
                          binary_filter_ip[j0]=part_binary_filter[k];
                          k++;

                      }
                      part_filter= strtok(NULL, ".");
                  }

                  if(strncmp(binary_ip,binary_filter_ip,sub)==0){
                      char error_res[]="HTTP/1.0 403 Forbidden\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: 111\r\n"
                                       "Connection: close\r\n\r\n"
                                       "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n"
                                       "<BODY><H4>403 Forbidden</H4>\n"
                                       "Access denied.\n"
                                       "</BODY></HTML>";
                      if(write(cli_fd, error_res, 198) < 0){
                          perror("write is failed\n");
                          return 1;
                      }
                      free(ip);
                      for (int i6 = 0; i6 < space_num; ++i6) {
                          free(arr[i6]);
                      }
                      free(arr);
                      for (int i6 = 0; i6 < count; ++i6) {
                          if(array[i6] != NULL){
                              free(array[i6]);
                          }
                      }
                      free(array);
                      free(path3);
                      free(HOST);
                      return 1;
                  }
                  free(ip);

              }

              printf("HTTP request =\n%s\nLEN = %d\n", request, (int )strlen(request));
              if(connect(fd,(struct sockaddr*)&struct_socket, sizeof(struct_socket))<0) {
                  perror("error: connect\n");
                  error_500(cli_fd);

                  return 1;
              }
              if(( write(fd,request, strlen(request)))==-1) {
                  perror("write is failed\n");
                  error_500(cli_fd);
                  return 1;

              }
              char ok[]="200 OK\r\n";
              char headers_end[]="\r\n\r\n";
              int read_size=0,good_req=0;
              for (int i = 0; i < 10000; ++i) {
                  buff[i]='\0';
              }
              int r2;
              lseek(fd,0,SEEK_SET);
              r2= read(fd, buff, 10000);
              if(r2 == -1){
                  perror("read is failed\n");
                  error_500(cli_fd);

                  return 1;
              }
              read_size=r2;
              while (r2 > 0){
                  if(write(cli_fd, buff, r2) == -1){
                      perror("write is failed\n");
                      error_500(cli_fd);
                      return 1;
                  }
                  if(strstr(buff,ok)!=NULL){
                      good_req=1;
                  }
                  if(strstr(buff,headers_end)!=NULL){
                      break;
                  }

                  for (int i = 0; i < 10000; ++i) {
                      buff[i]='\0';
                  }
                  r2= (int )read(fd, buff, 10000);
                  if(r2 == -1){
                      perror("write is failed\n");
                      error_500(cli_fd);

                      for (int i = 0; i < space_num; ++i) {
                          free(arr[i]);
                      }
                      free(arr);
                      for (int i = 0; i < count; ++i) {
                          if(array[i]!=NULL){
                              free(array[i]);
                          }
                      }
                      free(array);
                      free(path3);
                      free(HOST);
                      return 1;
                  }

                  read_size+=r2;

              }
              if(good_req==0){
                  lseek(fd,0,SEEK_SET);

                  printf("\n Total response bytes: %d\n",read_size);
                  close(fd);
                  for (int i = 0; i < space_num; ++i) {
                      free(arr[i]);
                  }
                  free(arr);
                  for (int i = 0; i < count; ++i) {
                      if(array[i]!=NULL){
                          free(array[i]);
                      }
                  }
                  free(array);
                  free(path3);
                  free(HOST);
                  return 0;
              }else{
                  for (; z <count ; ++z) {
                      if(stat(path3, &st) == -1){
                          mkdir(path3, 0700);
                      }
                      strcat(path3, "/");
                      strcat(path3, array[z]);
                  }
              }
              int file= open(path3, O_CREAT | O_RDWR, 0644);
              if(file==-1){
                  perror("open is failed\n");
                  error_500(cli_fd);

                  for (int i = 0; i < space_num; ++i) {
                      free(arr[i]);
                  }
                  free(arr);
                  for (int i = 0; i < count; ++i) {
                      if(array[i]!=NULL){
                          free(array[i]);
                      }
                  }
                  free(array);
                  free(path3);
                  free(HOST);
                  return 1;
              }
              int x0=0;
              for (; x0 < read_size - 4; ++x0) {
                  if(buff[x0]=='\r'&&buff[x0+1]=='\n'&&buff[x0+2]=='\r'&&buff[x0+3]=='\n'){
                      break;
                  }
              }
              int x1=x0+4;

              unsigned char  last[10000];
              for (int i = 0; i < 10000; ++i) {
                  last[i]='\0';
              }
              for (int i = 0; x1 <read_size ; ++i) {
                  last[i]=buff[x1];
                  x1++;
              }
              lseek(file,0,SEEK_SET);
              int w2=(int )write(file, last, read_size - x0 - 4);
              int w2_size=w2;
              if(w2 == -1){
                  perror("write is failed\n");
                  error_500(cli_fd);

                  for (int i = 0; i < space_num; ++i) {
                      free(arr[i]);
                  }
                  free(arr);
                  for (int i = 0; i < count; ++i) {
                      if(array[i]!=NULL){
                          free(array[i]);
                      }
                  }
                  free(array);
                  free(path3);
                  free(HOST);
                  return 1;
              }
              for (int i = 0; i < 10000; ++i) {
                  buff[i]='\0';
              }
              r2= (int )read(fd, buff, 10000);
              read_size+=r2;

              while (r2 > 0){
                  if(write(file, buff, r2) == -1){
                      perror("write is failed\n");
                      error_500(cli_fd);

                      close(fd);
                      for (int i = 0; i < space_num; ++i) {
                          free(arr[i]);
                      }
                      free(arr);
                      for (int i = 0; i < count; ++i) {
                          if(array[i]!=NULL){
                              free(array[i]);
                          }
                      }
                      free(array);
                      free(path3);
                      free(HOST);
                      return 1;
                  }
                  w2=(int )write(cli_fd, buff, r2);
                  if(w2 == -1){
                      perror("write is failed\n");
                      error_500(cli_fd);
                      for (int i = 0; i < space_num; ++i) {
                          free(arr[i]);
                      }
                      free(arr);
                      for (int i = 0; i < count; ++i) {
                          if(array[i]!=NULL){
                              free(array[i]);
                          }
                      }
                      free(array);
                      free(path3);
                      free(HOST);
                      return 1;
                  }
                  w2_size+=w2;
                  for (int i = 0; i < 10000; ++i) {
                      buff[i]='\0';
                  }
                  r2= (int )read(fd, buff, 10000);
                  read_size+=r2;
              }
              printf("File is given from origin server\n");
              printf("\n Total response bytes: %d\n",w2_size);
              close(fd);
              close(file);
          }


          for (int i = 0; i < count; ++i) {
              if(array[i]!=NULL){
                  free(array[i]);
              }
          }
          free(array);
          free(path3);
          free(HOST);


   }


    for (int i = 0; i < space_num; ++i) {
            free(arr[i]);
    }
    free(arr);
    close(cli_fd);

    return 0;
}

int main(int argc, char * argv[]){
    if(argc!=5){
        printf("Usage: proxyServer <port> <pool size> <max number of request> <filter>\n");
        exit(1);
    }
    struct  sockaddr_in server;
    struct sockaddr_in client ;
    int client_len= sizeof(client);
    struct hostent* hostent;
    unsigned char  buff[10000];
    for (int i = 0; i < 10000; ++i) {
        buff[i]='\0';
    }
    ///////////////////////////////////////////////
    int filter_fd;
    filter_fd= open(argv[4],O_RDONLY,0644);
    if(filter_fd<0){
        perror("open filter is failed\n");
        return 1;
    }
    char* read_filter=(char *) malloc(10000);
    for (int i = 0; i < 10000; ++i) {
        read_filter[i]=0;
    }
    int r_filter=(int ) read(filter_fd,read_filter,10000);
    if(r_filter==-1){
        perror("read is failed\n");
        close(filter_fd);
        free(read_filter);
        for (int i = 0; i < filt_count; ++i) {
            free(arr_filter[i]);
        }

        free(arr_filter);
        return 1;
    }
    int begin=r_filter;
    int r_f_size=10000;
    if(r_filter==10000) {
        while (r_filter > 0) {
            r_f_size+=r_f_size;
            read_filter=  (char *)realloc(read_filter,r_f_size);
            r_filter= (int )read(filter_fd,(read_filter)+begin,10000);
            begin+=r_filter;

        }
    }
     filt_count=0;
    for (int i = 0; i < strlen(read_filter); ++i) {
        if(read_filter[i]=='\n'){
            filt_count++;
        }
    }
     arr_filter=(char **) malloc(sizeof(char*)*filt_count);
    int v=0;
    char* token2;
    token2= strtok(read_filter," \r\n");
    while (token2!=NULL&&v<filt_count){
        arr_filter[v]=(char *) malloc(strlen(token2)+1);
        strcpy(arr_filter[v],token2);
        token2= strtok(NULL," \r\n");
        v++;
    }
    close(filter_fd);
    ///////////////////////////////////////////////
    int fd ,*new_fd;

    int port,pool_size,max_number_of_req;
    port= atoi(argv[1]);
    pool_size=atoi(argv[2]);
    max_number_of_req= atoi(argv[3]);
    new_fd=(int *) malloc(sizeof(int)*max_number_of_req);


    fd= socket(PF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket is failed\n");
        exit(1);
    }
    server.sin_family=AF_INET;
    server.sin_port= htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(fd, (struct sockaddr*) &server, sizeof(server)) < 0) {
        perror("bind is failed\n");
        exit(1);
    }
    if(listen(fd, 10) < 0) {
        perror("listen is failed\n");
        exit(1);
    }

threadpool * thread_pool;
   thread_pool= create_threadpool(pool_size);

    for (int i = 0; i < max_number_of_req; ++i) {
        new_fd[i] = accept(fd, (struct sockaddr*) &client, (socklen_t *)&client_len);
        if(new_fd[i]< 0) {
            perror("accept is failed\n");
            exit(1);
        }
        dispatch(thread_pool,function,&(new_fd[i]));

    }
    destroy_threadpool(thread_pool);


    close(fd);
    free(read_filter);
    for (int i = 0; i < filt_count; ++i) {
        free(arr_filter[i]);
    }
    free(new_fd);
    free(arr_filter);
    return 0;


}
