# cs425_mp2

## Operating process
### Preparing the code and data

First, you should save the 'introducer.cpp' onto one of your virtual machines to make it as a introducer. In this time, we use vm1 as introducer. And you should save 'udp_server_client.cpp' that in the 'mp2 folder' onto other nine virtual machines.

You can git clone or scp all the code onto your servers.
Before git clone or scp the code,you should take care of the variable `ip_list` in the 'introducer.cpp' and 'udp_server_client.cpp'
```
vector<string> ip_list={"172.22.156.2","172.22.158.2","172.22.94.2","172.22.156.3","172.22.158.3",
                        "172.22.94.3","172.22.156.4","172.22.158.4","172.22.94.4","172.22.156.5"};
```
You can modify it into your own ip of 10 virtual machines.


Then, You can refer to`./scripts/scp_vm_udp_multi_thread.sh`.
Input the following commands to scp 'udp_server_client.cpp' and 'introducer.cpp' onto all 10 virtual machines at one time:
```
./scripts/scp_vm_udp_multi_thread.sh
```


### Compile the introducer and  udp_server_client program
Once connected to all virtual machines, user can chose one machine as introducer, and run the 'g++ -pthread -g introducer.cpp -o ./introducer_try' to compile the introducer program. And you can run 'g++ -pthread -g udp_server_client.cpp -o  ./udp_server_client' to compile the udp server client program.


### Run the client program and send grep command
After activating introducers and servers, you can run './introducer_try' on the introducer virtual machine which would waiting new virtual machines joining. And you can run './udp_server_client' on others nine virtual machines on by one, you will see the introducer and all other joining virtual machines showing the newst member list status. 

```
1 172.22.94.198  active   1664218584419230964 
2 172.22.156.199 active   1664218578342640987 
3 172.22.158.199 unactive 1664218584419202165 
4 172.22.94.199  unactive 1664218574415306622 
5 172.22.156.200 active   1664218587915163773 
6 172.22.158.200 unactive 1664218574415306622 
7 172.22.94.200  unactive 1664218574415306622 
8 172.22.156.201 unactive 1664218574415306622 
9 172.22.158.201 unactive 1664218574415306622 
10 172.22.94.201 unactive 1664218574415306622 
```

Once all the other virtual machines joining the group, all the virtual machines status is 'active', and you can start to fail or leave any virtual machines, by click 'ctrl+c', the virtual machine would be killed, and all the remaining virtual machines would update their member list status.


```
1 172.22.94.198  active 1664218584419230964 
2 172.22.156.199 active 1664218578342640987 
3 172.22.158.199 active 1664218584419202165 
4 172.22.94.199  active 1664218574415306622 
5 172.22.156.200 fail   1664218587915163773 
6 172.22.158.200 active 1664218574415306622 
7 172.22.94.200  active 1664218574415306622 
8 172.22.156.201 active 1664218574415306622 
9 172.22.158.201 active 1664218574415306622 
10 172.22.94.201 active 1664218574415306622 
```
