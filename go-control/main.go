package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	pb "github.com/bhanubpsn/data-stream/go-control/pb"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

func main() {
	//Connect to the C++ Engine
	conn, err := grpc.Dial("localhost:50051", grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("Failed to connect to C++ Engine: %v", err)
	}
	defer conn.Close()
	client := pb.NewStreamControllerClient(conn)

	fmt.Println("------------------------------------------")
	fmt.Println("   VIDEO STREAM CONTROL CENTER (Go)       ")
	fmt.Println("------------------------------------------")
	fmt.Println("Enter IP addresses to add them to the stream.")
	fmt.Println("Type 'exit' to quit.")
	fmt.Println("Example: 192.168.1.50")
	fmt.Println("------------------------------------------")

	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("Command (add/kick/exit/quality) [IP] >> ")
		if !scanner.Scan() {
			break
		}

		parts := strings.Fields(scanner.Text())
		if len(parts) == 0 {
			continue
		}

		command := strings.ToLower(parts[0])

		if command == "exit" {
			break
		}
		if len(parts) < 2 {
			fmt.Println(" [!] Usage: add 192.168.x.x or kick 192.168.x.x or quality 192.168.x.x [0/1/2]")
			continue
		}

		ip := parts[1]
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)

		var resp *pb.StatusResponse
		var err error

		if command == "add" {
			resp, err = client.AddSubscriber(ctx, &pb.SubscriberRequest{IpAddress: ip, Port: 6001})
		} else if command == "kick" {
			resp, err = client.RemoveSubscriber(ctx, &pb.SubscriberRequest{IpAddress: ip, Port: 6001})
		} else if command == "quality" {
			if len(parts) < 3 {
				fmt.Println("Usage: quality [IP] [0/1/2]")
				continue
			}
			level, _ := strconv.Atoi(parts[2])
			resp, err = client.SetQuality(ctx, &pb.QualityRequest{
				IpAddress:    ip,
				QualityLevel: int32(level),
			})
		}
		cancel()

		if err != nil {
			fmt.Printf(" [!] gRPC Error: %v\n", err)
		} else {
			fmt.Printf(" [*] Response: %s\n", resp.Message)
		}
	}
}
