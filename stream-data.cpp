#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <thread>
#include <poll.h>
#include <grpcpp/grpcpp.h>
#include "stream.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using stream::StreamController;
using stream::SubscriberRequest;
using stream::StatusResponse;
using namespace std;

#define PORT 5005         // The port server listens on
#define BUFFER_SIZE 65535 // Max size of a UDP packet mtlab 65535 bytes k packets can be sent

//Quality levels for video streaming
enum Quality { HIGH = 0, MID = 1, LOW = 2 };

// Jo stream krega
struct Subscriber {
    struct sockaddr_in addr;
    string ip;
    Quality current_quality;
};


vector<Subscriber> subscribers;
mutex sub_mutex;

// gRPC Service Implementation 
class StreamServiceImpl final : public StreamController::Service {
    grpc::Status AddSubscriber(grpc::ServerContext* context, const stream::SubscriberRequest* request, stream::StatusResponse* reply) override {
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa)); 
        
        sa.sin_family = AF_INET;
        sa.sin_port = htons(request->port());
        
        if (inet_pton(AF_INET, request->ip_address().c_str(), &sa.sin_addr) <= 0) {
            reply->set_success(false);
            reply->set_message("Invalid IP");
            return grpc::Status::OK;
        }

        lock_guard<mutex> lock(sub_mutex);
        Subscriber new_sub;
        new_sub.addr = sa;
        new_sub.ip = request->ip_address();
        new_sub.current_quality = HIGH; // Default quality level
        subscribers.push_back(new_sub);
        
        cout << ">>> [Control Plane] ADDED: " << request->ip_address() << ":" << request->port() << endl;
        reply->set_message("Subscriber added successfully");
        reply->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status RemoveSubscriber(grpc::ServerContext* context, const stream::SubscriberRequest* request, stream::StatusResponse* reply) override {
        lock_guard<mutex> lock(sub_mutex);
        
        string target_ip = request->ip_address();
        int target_port = request->port();

        auto it = std::remove_if(subscribers.begin(), subscribers.end(), [&](const Subscriber& s) {
            return s.ip == target_ip && ntohs(s.addr.sin_port) == target_port;
        });

        if (it != subscribers.end()) {
            subscribers.erase(it, subscribers.end());
            cout << ">>> [Control Plane] KICKED: " << target_ip << endl;
            reply->set_success(true);
            reply->set_message("Subscriber removed");
        } else {
            reply->set_success(false);
            reply->set_message("IP not found in list");
        }
        
        return grpc::Status::OK;
    }

    grpc::Status SetQuality(ServerContext* context, const stream::QualityRequest* request, stream::StatusResponse* reply) override {
        lock_guard<mutex> lock(sub_mutex);
        bool found = false;
        
        for (auto& sub : subscribers) {
            if (sub.ip == request->ip_address()) {
                sub.current_quality = (Quality)request->quality_level();
                found = true;
                break;
            }
        }

        if (found) {
            reply->set_success(true);
            reply->set_message("Switched to quality level " + to_string(request->quality_level()));
            cout << ">>> [Control Plane] Switched " << request->ip_address() << " to Level " << request->quality_level() << endl;
        } else {
            reply->set_success(false);
            reply->set_message("Subscriber not found");
        }
        return grpc::Status::OK;
    }
};

void run_udp_engine() {
    int ports[] = {5005, 5006, 5007};
    int sockets[3];
    struct pollfd fds[3];

    // Creating a UDP Socket
    // Which is alos called Socket Descriptor
    // It is an integer that uniquely identifies the socket in the operating system (kernal) very similar to file descriptors

    for (int i = 0; i < 3; i++) {
        sockets[i] = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports[i]);    // htons converts the port number from host byte order to network byte order (big-endian) which is required for network communication
                                            // big-endian mtalab MSB phele store hogi then LSB, left to right padhne k liye
        addr.sin_addr.s_addr = INADDR_ANY;  // Listen on ALL available network interfaces
       // Bind the socket to the port, saara traffic jo is port pe aayega wo is socket pe receive hoga
        if (::bind(sockets[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        int broadcastEnable = 1;
        if (setsockopt(sockets[i], SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        // AF_INET: This specifies that we are using the IPv4 address family the usual 198.168.x.x
        // SOCK_DGRAM: This indicates that we want to create a datagram socket which is used for UDP communication
        // For TCP its SOCK_STREAM
        // 0 : This is the protocol field. For UDP we can set it to 0 to let the system choose the appropriate protocol (which will be IPPROTO_UDP)
    

        fds[i].fd = sockets[i];
        fds[i].events = POLLIN; // Tell poll to watch for INCOMING data
    }

    char buffer[BUFFER_SIZE];

    // Define our "Subscribers" (for testing, send to ports 6001 and 6002)
    // Jaha pe video data receive karna hai, matlab subscriber ke address aur port number define karna hai
    // struct sockaddr_in sub1, sub2;
    // sub1.sin_family = sub2.sin_family = AF_INET;
    // sub1.sin_port = htons(6001);
    // sub2.sin_port = htons(6002);

    // IP Network to Presentation: This function converts an IP address from its standard text presentation form
    // taki network card smajh sake, aur usko binary format me store kar sake, jo ki network communication ke liye zaruri hai
    // inet_pton(AF_INET, "192.168.1.143", &sub1.sin_addr);
    // inet_pton(AF_INET, "127.0.0.1", &sub2.sin_addr);
    // inet_pton(AF_INET, "192.168.15.255", &sub1.sin_addr); // Agar broadcast karna hai toh using router etc.
    // localhost pe hi dono subscribers hain, agar alag machines pe hote to unka IP address yaha specify karna padta

    cout << ">>> [Data Plane] UDP Reflector active on port High Quality " << ports[0] << endl;
    cout << ">>> [Data Plane] UDP Reflector active on port Medium Quality " << ports[1] << endl;
    cout << ">>> [Data Plane] UDP Reflector active on port Low Quality " << ports[2] << endl;

    while (true) {   
        int ret = poll(fds, 3, -1); 
        if (ret <= 0) continue;

        // Receive packet
        for (int i = 0; i < 3; i++) {
            if (fds[i].revents & POLLIN) {
                struct sockaddr_in clientAddr;
                socklen_t clen = sizeof(clientAddr);
                int n = recvfrom(sockets[i], buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clen);
                
                if (n > 0) {
                    lock_guard<mutex> lock(sub_mutex);
                    //This is the dynamic way to add subscribers using gRPC, jaha pe subscribers ka list maintain karte hain aur usme naye subscribers add karte hain jab bhi gRPC call aati hai
                    for (const auto& sub : subscribers) {
                        // THE ABR MAGIC: Only send if the quality matches the port
                        // Port 5005 (i=0) goes to HIGH (0)
                        // Port 5006 (i=1) goes to MID (1)
                        // Port 5007 (i=2) goes to LOW (2)
                        if ((int)sub.current_quality == i) {
                            sendto(sockets[i], buffer, n, 0, (struct sockaddr *)&sub.addr, sizeof(sub.addr));
                        }
                    }
                }
            }
        }

        // Forward the packet to subscribers clone karke 
        
        //Below is the manual way to add the subscribers
        // sendto(sockfd, buffer, n, 0, (struct sockaddr *)&sub1, sizeof(sub1));
        // sendto(sockfd, buffer, n, 0, (struct sockaddr *)&sub2, sizeof(sub2));
    }

    for(int i=0; i<3; i++) {
        close(sockets[i]);
    }
}

int main() {
    //Start the UDP Engine in a background thread
    std::thread udp_thread(run_udp_engine);

    //Start the gRPC Server in the main thread
    string server_address("0.0.0.0:50051");
    StreamServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    
    cout << ">>> [Control Plane] gRPC Server on " << server_address << endl;
    server->Wait();
    udp_thread.join();
    return 0;
}
