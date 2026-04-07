<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hybrid ABR Video Streamer Documentation</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif; line-height: 1.6; color: #24292e; max-width: 900px; margin: 0 auto; padding: 45px; background-color: #f6f8fa; }
        .container { background: white; padding: 45px; border: 1px solid #d1d5da; border-radius: 6px; box-shadow: 0 1px 3px rgba(0,0,0,0.12); }
        h1 { border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; color: #0366d6; }
        h2 { border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; margin-top: 24px; }
        code { background-color: rgba(27,31,35,0.05); padding: 0.2em 0.4em; border-radius: 3px; font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace; font-size: 85%; }
        pre { background-color: #f6f8fa; padding: 16px; overflow: auto; border-radius: 6px; border: 1px solid #dfe2e5; line-height: 1.45; }
        pre code { background: none; padding: 0; font-size: 90%; }
        table { border-collapse: collapse; width: 100%; margin: 16px 0; }
        table th, table td { border: 1px solid #dfe2e5; padding: 8px 13px; }
        table tr:nth-child(even) { background-color: #f6f8fa; }
        .badge { display: inline-block; padding: 3px 10px; font-size: 12px; font-weight: 500; line-height: 18px; border-radius: 2em; background: #0366d6; color: white; margin-bottom: 20px; }
        .warning { background-color: #fffbdd; border: 1px solid #d4a017; padding: 15px; border-radius: 6px; margin: 20px 0; }
    </style>
</head>
<body>

<div class="container">
    <h1>🛰️ Hybrid ABR Video Streamer</h1>
    <div class="badge">Version 1.0 - Adaptive Bitrate Engine</div>
    
    <p>A high-performance video streaming system featuring a <strong>C++ Data Plane</strong> (using <code>poll()</code> for asynchronous UDP reflection) and a <strong>Go Control Plane</strong> (using gRPC). It supports real-time <strong>Adaptive Bitrate (ABR)</strong> switching.</p>

    <h2>🏗️ System Architecture</h2>
    <ul>
        <li><strong>Video Source:</strong> FFmpeg (Windows) generating 3-tier quality ladders.</li>
        <li><strong>Data Plane:</strong> C++ Engine (WSL2) reflecting UDP packets to subscribers via <code>poll.h</code>.</li>
        <li><strong>Control Plane:</strong> Go CLI managing subscribers and quality levels via gRPC.</li>
        <li><strong>Communication:</strong> Protobuf + gRPC Contract.</li>
    </ul>

    <h2>🛠️ Prerequisites</h2>
    
    <h3>Windows Host</h3>
    <ul>
        <li><strong>FFmpeg:</strong> Installed and added to System PATH.</li>
        <li><strong>VLC Media Player:</strong> For stream verification.</li>
    </ul>

    <h3>WSL2 (Ubuntu) Environment</h3>
    <pre><code>sudo apt update
sudo apt install -y build-essential cmake libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev golang-go</code></pre>

    <h2>🚀 Step-by-Step Setup</h2>

    <h3>1. Generate Protobuf Files</h3>
    <p>Run these commands inside your WSL2 project directory to create the Go and C++ bridges:</p>
    <pre><code># Generate Go code
protoc --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative stream.proto

# Generate C++ code
protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` stream.proto</code></pre>

    <h3>2. Compile the C++ Engine</h3>
    <pre><code>g++ -O3 reflector.cpp stream.pb.cc stream.grpc.pb.cc -lgrpc++ -lprotobuf -lpthread -o reflector</code></pre>

    <div class="warning">
        <strong>⚠️ Networking Note:</strong> To allow Windows FFmpeg to talk to WSL2, find your WSL IP using <code>wsl hostname -I</code>. 
    </div>

    <h2>🎮 Execution Guide</h2>

    <h3>Step 1: Start Reflector (WSL2)</h3>
    <pre><code>./reflector</code></pre>

    <h3>Step 2: Start Video Source (Windows PowerShell)</h3>
    <p>Replace <code>172.x.x.x</code> with your WSL IP:</p>
    <pre><code>ffmpeg -re -stream_loop -1 -i test.mp4 -map 0:v -c:v libx264 -b:v:0 3000k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5005?pkt_size=1316" -map 0:v -c:v libx264 -b:v:1 1000k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5006?pkt_size=1316" -map 0:v -c:v libx264 -b:v:2 200k -x264-params "keyint=30:min-keyint=30:scenecut=0" -f mpegts -mpegts_flags resend_headers "udp://172.25.120.110:5007?pkt_size=1316"</code></pre>

    <h3>Step 3: Run Control Plane (Go)</h3>
    <pre><code>go run main.go</code></pre>

    <h2>📝 CLI Commands</h2>
    <table>
        <thead>
            <tr>
                <th>Action</th>
                <th>Command Usage</th>
                <th>Description</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><strong>Add Subscriber</strong></td>
                <td><code>add [IP]</code></td>
                <td>Starts streaming High-Quality video to the target.</td>
            </tr>
            <tr>
                <td><strong>Kick Subscriber</strong></td>
                <td><code>kick [IP]</code></td>
                <td>Instantly removes IP from the reflection list.</td>
            </tr>
            <tr>
                <td><strong>Set Quality</strong></td>
                <td><code>quality [IP] [0/1/2]</code></td>
                <td>Switches quality: 0=High, 1=Mid, 2=Low.</td>
            </tr>
            <tr>
                <td><strong>Exit</strong></td>
                <td><code>exit</code></td>
                <td>Closes the controller.</td>
            </tr>
        </tbody>
    </table>

    <h2>🔍 Troubleshooting</h2>
    <ul>
        <li><strong>Grey Screen:</strong> VLC is waiting for a Keyframe (I-Frame). Our FFmpeg command sends one every 1 second. Wait briefly.</li>
        <li><strong>Firewall Issues:</strong> If FFmpeg cannot reach WSL2, ensure Windows Firewall allows UDP traffic on ports 5005, 5006, and 5007.</li>
        <li><strong>VLC Lag:</strong> Disable "Hardware-accelerated decoding" in VLC Preferences (Input / Codecs) for smoother bitrate transitions.</li>
    </ul>

</div>

</body>
</html>
