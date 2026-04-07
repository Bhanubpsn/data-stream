


# 🛰️ Hybrid ABR Video Streamer
> **A Software-Defined Media Engine with Dynamic Quality Switching**

This project implements a high-performance video streaming system featuring a **C++ Data Plane** for asynchronous UDP reflection and a **Go Control Plane** for real-time subscriber management via **gRPC**.

---

## 🏗️ System Architecture
* **Video Source:** FFmpeg (Windows) generating a 3-tier bitrate ladder (High, Mid, Low).
* **Data Plane:** C++ Engine (WSL2) using `poll.h` to multiplex three input streams to multiple subscribers.
* **Control Plane:** Go CLI (Windows/WSL2) communicating with the engine via gRPC to add/kick users or switch quality levels.
* **Protocols:** UDP for video data, gRPC (Protobuf) for control signals.

---

## 🛠️ Prerequisites

### Windows Host
* **FFmpeg:** Installed and added to System PATH.
* **VLC Media Player:** For stream verification and playback.

### WSL2 (Ubuntu) Environment
```bash
sudo apt update
sudo apt install -y build-essential cmake libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev golang-go


🚀 Setup Instructions
1. Define the Protocol
Create stream.proto:

Protocol Buffers

syntax = "proto3";
option go_package = "./pb";
package stream;

service StreamController {
  rpc AddSubscriber (SubscriberRequest) returns (StatusResponse) {}
  rpc RemoveSubscriber (SubscriberRequest) returns (StatusResponse) {}
  rpc SetQuality (QualityRequest) returns (StatusResponse) {}
}

message SubscriberRequest {
  string ip_address = 1;
  int32 port = 2;
}

message QualityRequest {
  string ip_address = 1;
  int32 quality_level = 2; // 0=High, 1=Mid, 2=Low
}

message StatusResponse {
  bool success = 1;
  string message = 2;
}


2. Generate Protobuf Files
Run these inside your WSL2 or Mac project directory:

# Generate Go code
protoc --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative stream.proto

# Generate C++ code
protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` stream.proto


3. Compile the C++ Engine (Data Plane)

g++ -O3 reflector.cpp stream.pb.cc stream.grpc.pb.cc -lgrpc++ -lprotobuf -lpthread -o reflector


🎮 How to Run
Step 1: Start the Reflector
Launch the engine in your terminal:

./reflector


Step 2: Start the Video Source
Note: Replace 172.x.x.x with your actual IP (found via wsl hostname -I).


ffmpeg -re -stream_loop -1 -i test.mp4 -map 0:v -c:v libx264 -b:v:0 3000k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5005?pkt_size=1316" -map 0:v -c:v libx264 -b:v:1 1000k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5006?pkt_size=1316" -map 0:v -c:v libx264 -b:v:2 200k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5007?pkt_size=1316"


Step 3: Start the Control Plane (Go)

go run main.go


📝 CLI Command Reference
Command Usage Result
Add     add [IP]    Starts streaming High-Quality video to target IP.

Kick    kick [IP]    Instantly removes the IP from the active stream.

Quality    quality [IP] [0/1/2]    Switches bitrate: 0=High, 1=Mid, 2=Low.

Exit    exit    Shuts down the Go Controller.

🔍 Troubleshooting
Grey Screen in VLC: This is normal during a quality switch. VLC is waiting for the next I-Frame. Our FFmpeg command sends one every 1 second (keyint=30).
Networking: Ensure the Windows Firewall allows UDP traffic on ports 5005, 5006, and 5007.
VLC Settings: If transitions are laggy, go to VLC Preferences > Input / Codecs > Disable "Hardware-accelerated decoding".
