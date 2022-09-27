//
// Created by px6 and ycc5 on 2022/9/18.
//
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <assert.h>
#include <mutex>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <chrono>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>
#include <mutex>
#include <sstream>
#include <cmath>
std::mutex mtx;

#define PORT 5000
#define INTRODUCER_PORT 5001
using namespace std::chrono;
using namespace std;
//TODO: To check when DATA_BUFFER_SIZE is exceeded, whether bad thing will happen
#define DATA_BUFFER_SIZE 1024
#define BUFFER_SIZE 128
#define MAX_CONNECTIONS 10
#define VM_COUNT 2
#define FAIL_PERIOD 5
using namespace std::chrono_literals;
struct timeval timeout={FAIL_PERIOD-1,100};
bool toLeave=false;
string ltos(long l){
    ostringstream os;
    os<<l;
    string result;
    istringstream is(os.str());
    is>>result;
    return result;

}
string return_time_string(){
    auto time_now = high_resolution_clock::now();
    auto nanosec = time_now.time_since_epoch();
    //cout<<"start time is" <<nanosec.count() <<" nanoseconds since epoch"<<endl;
    string time_string=ltos(nanosec.count());
    cout<<"Now time is" <<time_string<<" nanoseconds since epoch"<<endl;
    return time_string;
}


int get_local_ip(char * ifname, char * ip){
    char *tmp = NULL;
    int inet_sock;
    struct ifreq ifr;
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    memcpy(ifr.ifr_name, ifname, strlen(ifname));

    if(0 != ioctl(inet_sock, SIOCGIFADDR, &ifr)){
        perror("ioctl error");
        return -1;
    }

    tmp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    memcpy(ip, tmp, strlen(tmp));
    close(inet_sock);
    return 0;
}

vector<string> split_command(string& temp){
    vector <string> dane;
    //string temp;// = string(s);
    // std::cout<<"temp="<< temp <<endl;
    string space_delimiter = " ";
    vector<string> words_with_prefix{};
    //vector<string> words{};
    std::string buf;                 // Have a buffer string
    std::stringstream ss(temp);       // Insert the string into a stream
    std::vector<std::string> tokens; // Create vector to hold our words
    //int i = 0;
    while (ss >> buf){
        words_with_prefix.push_back(buf);
    }
    // cout<<"in split_command(),words push_back"<<endl;
    for( auto const& s : words_with_prefix ){
        // std::cout << s+" ";
    }cout<<endl;

    return words_with_prefix;
}

vector<string> update_maintain_list_state(string& temp, vector<vector<string>>& maintain_list,int VM_number,bool toIntroduce){
    //if toIntroduce we must change content, else, we only change different and newest membership list change
    cout<<"in update_maintain_list_state()"<<endl;

    vector<string> words_with_prefix=split_command(temp);
    vector<string> words(words_with_prefix.begin() + 2,words_with_prefix.end());


    for( auto const& s : words ){
        // std::cout << s+" ";
    }cout<<endl;


    for(int i = 0;i<words.size()-3;i=i+maintain_list[0].size()){
        int idx = stoi(words[i]);
        // std::cout<<"idx="<<idx<<endl;
        // cout<<"words["<<i+2<<"]="<<words[i+2]<<endl;
        if(idx==VM_number){
            maintain_list[idx-1][2] = "active";

        }else{
            if(toIntroduce || (!toIntroduce && maintain_list[idx-1][2]!= words[i+2] && maintain_list[idx-1][3]<words[i+3])){
                maintain_list[idx-1][2] = words[i+2];//whether active
                maintain_list[idx-1][3] = words[i+3];//time
            }
        }
    }
    //print current maintain list
    for( auto const& string_vec : maintain_list ){
        for( auto const& s : string_vec ){
            std::cout << s+" ";
        }
        std::cout<<std::endl;
    }
    return words_with_prefix;
}

string create_udp_client_socket(const char *hostname = "172.22.94.198",int VM_number=1,
                            string COMMAND_CONTENT="1 172.22.94.198 unactive", bool ping = false,int sendto_VM_number=2,vector<vector<string>>* maintain_list={}){
//hostname: ip which we send ping/message
//VM_number: VM_number which we send ping/message
    int sockfd, addrlen;
    char buffer[DATA_BUFFER_SIZE];
    const char* message = COMMAND_CONTENT.c_str();
    struct sockaddr_in servaddr, addr;

    int n, len;
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket creation failed");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(addr));
  
    // Filling server information
    servaddr.sin_family = AF_INET;
    if(!ping){
        servaddr.sin_port = htons(INTRODUCER_PORT);
    }else{
        servaddr.sin_port = htons(PORT);
    }
    servaddr.sin_addr.s_addr = inet_addr(hostname);

    // connect to server
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("\n Error : Connect Failed \n");
        return "1";
    }

    // send update maintain list message to server
    std::cout<<sizeof(message)<<endl;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));//set time out
    auto send_ping_time_point = high_resolution_clock::now();

    // test bandwidth
    auto start = high_resolution_clock::now();
    char data[256];
    // std::cout<<"calculate how many byte "<<message<<endl;

    if(sendto(sockfd, message, strlen(message),
        0, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr))<0){
                std::cout<<"unable send message"<<endl;
                return "1";
            };

    // receive server's response and update self maintainlist

    addrlen = sizeof(addr);

    int bytes_sent = sizeof(message); 
    n = recvfrom(sockfd, (char*)buffer, DATA_BUFFER_SIZE, 0,
                (struct sockaddr *)&addr,  (socklen_t *)&addr);

    auto stop = high_resolution_clock::now();

    auto final_duration = duration_cast<microseconds>(stop  - start);
    double duration_t = final_duration.count() *0.000001;
    double bandwidth = ((double)bytes_sent/duration_t);
    // std::cout<<"size of message"<<sizeof(message)<<endl;
    // std::cout<<"the bandwidth is"<< bandwidth<<endl;

    auto recv_ack_time_point = high_resolution_clock::now();
    puts(buffer);
    auto ping_ack_duration = duration_cast<microseconds>(recv_ack_time_point  - send_ping_time_point);
    // cout<<"recv byte="<<n<<endl;
    // cout<<"ping_ack_duration="<<ping_ack_duration.count()*0.000001<<endl;
    // cout<<"self VM number is "<<VM_number<<endl;
    // cout<<"sendto VM number is "<<sendto_VM_number<<endl;
    // cout<<"sendto ip address is "<<hostname<<endl;
    if (n<0 || ping_ack_duration.count()*0.000001>=FAIL_PERIOD){
        if(ping){
            //update membeship set as "failed";

            // std::cout<<"fail                       fail"<<endl;
            // std::cout<<"error while receiving server's msg, ack time out "<<endl;

            std::cout<<"\n"<<endl;

            (*maintain_list)[sendto_VM_number-1][2] = "fail";//whether active
            (*maintain_list)[sendto_VM_number-1][3] = return_time_string();//time
            // cout<<"maintain_list changed"<<endl;
            close(sockfd);
            return "1";
        }
    }

    // std::cout<<"recv message"<<endl;
    string finalResult="";
    finalResult += buffer; 

    close(sockfd);
    // cout<<"finalResult is"+finalResult<<endl;
    return finalResult;

}


string print_maintain_list(vector<vector<string>>* maintain_list){
    string message = "";
    for (vector<string> & str :(* maintain_list)){
        for(string & ch: str){
            message +=ch+" ";
        }
        message += " ";
    }
    return message;
}
void create_udp_client_socket_multi_thread(string* ip_address,int self_VM_number, vector<vector<string>>* maintain_list, vector<string>* ip_list,int sendto_VM_number){



    string sendto_ip_address=(* ip_list)[sendto_VM_number-1];
    /*
    for(int i=0;i<VM_COUNT;i++){
        int vm_number = (self_VM_number+i)%10;
        string temp=(* ip_list)[vm_number];
        string update_other_vm_status = create_udp_client_socket(temp.data(),self_VM_number,message,true);
    }*/

    // false positive rate

    while(1){
        auto start_time_point = high_resolution_clock::now();
        float false_positive_rate;
        (*maintain_list)[self_VM_number-1][2] = "active";//time
        (*maintain_list)[self_VM_number-1][3] = return_time_string();//time
        string command="ping";
        string message = to_string(self_VM_number)+" "+command+" ";
        message+=print_maintain_list(maintain_list);

        // test false positive rate
        // random produce a number between 0 and 1
        false_positive_rate =  (float) rand()/RAND_MAX;

        if( false_positive_rate>0.03){
            string update_other_vm_status = create_udp_client_socket(sendto_ip_address.data(),self_VM_number,message,true,sendto_VM_number,maintain_list);
            auto end_time_point = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(start_time_point  - end_time_point);
            auto remain_time=FAIL_PERIOD*1000000-duration.count();
            if(remain_time>0){
                usleep(remain_time);
            }
            //usleep(max(FAIL_PERIOD*10e6-round(duration.count()),0.0));//*0.000001
            if(toLeave){break;}
        }   
        std::cout<<"no send"<<endl;     
    }

    return;
}


double send_status(string input_command,vector<vector<string>>&  maintain_list, vector<string>& ip_list) {
    
    char local_ip[32] = {0};
    char IF_NAME[5]= "eth0";
    get_local_ip(IF_NAME, local_ip);

    if(0 != strcmp(local_ip, "")){
        cout<<IF_NAME<<" ip is: "<<local_ip<< "\n";
    }
    int self_VM_number;
    for(int i=0;i<ip_list.size();i++){
        if(0 == strcmp(local_ip, ip_list[i].data())){
            //get self VM number
            self_VM_number=i+1;
        }
    }

    // time before sent status
    auto start = high_resolution_clock::now();

    // protocol buffers: vm number, ip , status ex: 2, 172.22.156.199, active
    string whole_command = "";
    if(input_command==""){input_command="join";}

    whole_command=to_string(self_VM_number)+" "+input_command+" "+
            to_string(self_VM_number)+" "+maintain_list[self_VM_number-1][1]+" active"+" "+maintain_list[self_VM_number-1][3];
    // cout<<"send status, whole command is"<<whole_command<<endl;
        
    // std::cout<<"sent status to introducer vm1"<<std::endl;
    string update_other_vm_status = create_udp_client_socket(ip_list[0].data(),self_VM_number,whole_command, false,1,&maintain_list);

    if (update_other_vm_status != "" and update_other_vm_status != "1"){
        //update all the maintain list
        // cout<<"before update_maintain_list_state" <<endl;
        //const char* a="1 172.22.156.2 active  2 172.22.158.2 unactive  3 172.22.94.2 unactive  4 172.22.156.3 unactive  5 172.22.158.3 unactive  6 172.22.94.3 unactive  7 172.22.156.4 unactive  8 172.22.158.4 unactive  9 172.22.94.4 unactive  10 172.22.156.5 unactive";
        update_maintain_list_state(update_other_vm_status, maintain_list,self_VM_number,true);//const_cast<char*>(update_other_vm_status.c_str())
        // cout<<"after update_maintain_list_state" <<endl;
    }

    else{
        std::cout<<"introducer is broken!"<<endl;
    }

    auto before_print_stop = high_resolution_clock::now();
    auto before_print_duration = duration_cast<microseconds>(before_print_stop  - start);

    // Get the difference in timepoints and cast it to required units //
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    auto final_stop = high_resolution_clock::now();
    auto final_duration = duration_cast<microseconds>(final_stop  - start);

    return duration.count() *0.000001;
}


void create_udp_server_to_recv_message(vector<vector<string>>* maintain_list,int self_VM_number,bool toIntroduce){

    int listenfd, connfd, udpfd, nready, maxfdp1;
    char buffer[DATA_BUFFER_SIZE];
    pid_t childpid;
    fd_set rset;
    ssize_t n;
    socklen_t len;
    const int on = 1;
    struct sockaddr_in cliaddr, servaddr;
    string message = "";
    void sig_chld(int);

    // create listening TCP socket //
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(toIntroduce){
        servaddr.sin_port = htons(INTRODUCER_PORT);
    }else{
        servaddr.sin_port = htons(PORT);
    }


    // binding server addr structure to listenfd
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    // create UDP socket //
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    // binding server addr structure to udp sockfd
    bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    // clear the descriptor set
    FD_ZERO(&rset);

    // get maxfd
    maxfdp1 = max(listenfd, udpfd) + 1;
    for (;;) {

        // set listenfd and udpfd in readset
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        // select the ready descriptor
        nready = select(maxfdp1, &rset, NULL, NULL, NULL);

        // if udp socket is readable receive the message.
        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            bzero(buffer, sizeof(buffer));
            // printf("\nMessage from UDP client: ");
            setsockopt(udpfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));//set time out
            n = recvfrom(udpfd, (char*)buffer, DATA_BUFFER_SIZE, 0,
                         (struct sockaddr *)&cliaddr, &len);


            // update_maintain_list_state(buffer, maintain_list);
            string update_other_vm_status = buffer;
            vector<string> words_with_prefix;

            words_with_prefix=update_maintain_list_state(update_other_vm_status, (*maintain_list),self_VM_number,toIntroduce);
            puts(buffer);

            //TODO:send acknowledge to the sender machine
            (*maintain_list)[self_VM_number-1][2]="active";
            (*maintain_list)[self_VM_number-1][3]=return_time_string();
            message+=words_with_prefix[0]+" "+words_with_prefix[1]+" ";
            for (vector<string> & str : * maintain_list){
                for(string & ch: str){
                    message +=ch+" ";
                }
                message += " ";
            }

            const char * sendmessage = message.c_str();
            if(!toIntroduce){//ping-ack,send ack
                sendto(udpfd, sendmessage, strlen(sendmessage), 0,
                       (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            }

        }
    }
    //return 0;
}

int main () {
    auto start = high_resolution_clock::now();
    auto nanosec = start.time_since_epoch();
    cout<<"start time is" <<nanosec.count() <<" nanoseconds since epoch"<<endl;
    string vm_time=ltos(nanosec.count());
    cout<<"VM time is" <<vm_time<<" nanoseconds since epoch"<<endl;

    vector<string> ip_list= {"172.22.94.198","172.22.156.199","172.22.158.199","172.22.94.199","172.22.156.200",
    "172.22.158.200","172.22.94.200","172.22.156.201","172.22.158.201","172.22.94.201"};


// {"172.22.156.2","172.22.158.2","172.22.94.2","172.22.156.3","172.22.158.3",
//  "172.22.94.3","172.22.156.4","172.22.158.4","172.22.94.4","172.22.156.5"};
    vector<vector<string>> maintain_list;
    /*      ={{"1","172.22.156.2","unactive",vm_time}, {"2","172.22.158.2","unactive",vm_time}, {"3","172.22.94.2","unactive",vm_time},
             {"4","172.22.156.3","unactive",vm_time}, {"5","172.22.158.3","unactive",vm_time}, {"6","172.22.94.3","unactive",vm_time},
             {"7","172.22.156.4","unactive",vm_time}, {"8","172.22.158.4","unactive",vm_time}, {"9","172.22.94.4","unactive",vm_time},
             {"10","172.22.156.5","unactive",vm_time}};*/
    for(int i=0;i<ip_list.size();i++){
        vector<string> v={to_string(i+1),ip_list[i],"unactive",vm_time};
        maintain_list.push_back(v);
    }


    ios::sync_with_stdio(false);
    cin.tie(0);
    cout.tie(0);

    // create udp client socket to send message
    string input_command = "";

    double duration=send_status(input_command,maintain_list,ip_list);

    // get self vm number
    char local_ip[32] = {0};
    char IF_NAME[5]= "eth0";
    get_local_ip(IF_NAME, local_ip);
    int self_VM_number;
    for(int i=0;i<ip_list.size();i++){
        if(0 == strcmp(local_ip, ip_list[i].data())){
            //get self VM number
            self_VM_number=i+1;
        }
    }

    // create udp server socket to receive message
    thread introducer_thread;
    //receive the introducer's message
    introducer_thread = std::thread(create_udp_server_to_recv_message,&maintain_list,self_VM_number,true);
    //int return_result=create_udp_server_to_recv_message(maintain_list);


    thread ping_threads[VM_COUNT];
    for(int i =0; i<VM_COUNT;i++){
        //mtx.lock();
        int vm_number = (self_VM_number+i)%10;
        string ip_address=ip_list[vm_number];
        // cout<<"beginning of ping thread,ip_address="<<ip_address<<endl;

        ping_threads[i] = std::thread(create_udp_client_socket_multi_thread,&ip_address,self_VM_number,&maintain_list, &ip_list,vm_number+1);
        //mtx.lock();
    }


    thread ack_threads[VM_COUNT];
    for(int i =0; i<VM_COUNT;i++){
        int vm_number = (self_VM_number-i)%10;
        string ip_address=ip_list[vm_number];
        ack_threads[i] = std::thread(create_udp_server_to_recv_message,&maintain_list,self_VM_number,false);
    }


    for (auto &thread : ack_threads){
        thread.join();
    }

    for (auto &thread : ping_threads){
        thread.join();
    }


    introducer_thread.join();
}
