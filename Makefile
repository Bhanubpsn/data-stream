LDFLAGS = -L/usr/local/lib `pkg-config --libs protobuf grpc++` -lpthread
CXXFLAGS = -std=c++17 `pkg-config --cflags protobuf grpc++`

all: reflector

stream.pb.cc stream.grpc.pb.cc: ./proto/stream.proto
	protoc -I ./proto --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./proto/stream.proto

reflector: stream.pb.cc stream.grpc.pb.cc stream-data.cpp
	g++ $(CXXFLAGS) -o reflector stream-data.cpp stream.pb.cc stream.grpc.pb.cc $(LDFLAGS)
	