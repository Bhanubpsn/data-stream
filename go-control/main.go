package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"
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
		fmt.Print("Add IP >> ")
		if !scanner.Scan() {
			break
		}

		input := strings.TrimSpace(scanner.Text())

		if input == "exit" {
			break
		}

		if input == "" {
			continue
		}

		//Send the RPC Command to C++
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		response, err := client.AddSubscriber(ctx, &pb.SubscriberRequest{
			IpAddress: input,
			Port:      6001, //standard port for video streaming humari videos
		})
		cancel()

		if err != nil {
			fmt.Printf(" [!] Error: Could not reach C++ Engine: %v\n", err)
		} else if response.Success {
			fmt.Printf(" [+] Success: %s is now receiving the stream!\n", input)
		} else {
			fmt.Printf(" [-] Failed: %s\n", response.Message)
		}
	}
}
