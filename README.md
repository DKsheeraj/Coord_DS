# Distributed Coordination System

KGPKeeper is a c++ based distributed coordination service

## Overview


This project implements a distributed coordination system where an admin can dynamically add servers with IPs and ports to a servers.json file. Clients initially connect to a Loadbalancer server to connect to any existing server


## Directory Structure

```
project_root/
│── CMakeLists.txt              # CMake build file
│
├── data/
│   └── servers.json            # JSON storage
│   └── loadbalancers.json       
│   └── portNumbers*.json       # Log files and other info
│
├── include/
│   ├── coordination.h        # Header files
│   ├── Node.h
│   └── loadbalancer.h
│
├── src/
│   ├── server.cpp
│   ├── client.cpp
│   ├── admin.cpp
│   ├── coordination.cpp
│   ├── server_api.cpp
│   ├── client_api.cpp
│   ├── init.cpp
│   └── loadbalancer.cpp
│
└── build/                      # (Generated build files)
```

## Features

Admin Control: Add servers dynamically to servers.json.

Client-LoadBalancer Communication: Clients can request server information.

JSON Storage: Server details are saved in data/servers.json.

C++ with CMake: Modular structure with separate utilities and CMake build system.


## Dependencies
Ensure you have the following installed:

C++20 or later

CMake 3.10+

nlohmann-json library (Install with sudo apt install nlohmann-json3-dev on Ubuntu)

crow

asio

curl

## Build Instructions

```python
# Create and navigate to the build directory
mkdir -p build && cd build

# Run CMake to generate build files
cmake .. && cmake --build .
```

## Running the Programs
First run init file for initial configuration setup
Ensure that you are running servers and loadbalancer before running client

```python
# Admin (Add servers)
./build/admin

# Init
./build/init

# Server
./build/server_api ip port

# Client
./build/client_api 

# Loadbalancer
./build/loadbalancer ip port
```


## License

[MIT](https://choosealicense.com/licenses/mit/)
