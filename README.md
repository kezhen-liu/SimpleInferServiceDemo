# SimpleInferServiceDemo

------

This is a simple demo use case of setting up an inference server that is open for incoming images inference workload. Currently for every incoming image, the server will do image decoding using OpenCV and followed by a detection and attribute classificaiton based on Intel Openvino toolkit. This repo contains not only the server implementation, but also the example client that communicates to the server.

## Dependencies
This repository have the following components as prerequsites.

- Boost 1.71
- OpenCV 4.4
- Openvino 2020.4
- Redis
- redis-plus-plus, which is a Redis client library written in C++, available from https://github.com/sewenew/redis-plus-plus

Additionally we also had a base64 decoding/encoding library written by René Nyffenegger rene.nyffenegger@adp-gmbh.ch as a component, available at https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp

### Prerequisite install script
A very simple install script is also provided along with this repo available at `installPrerequisite.sh`

## Features

- Pure C++ implementation of the entire stack, including http server/client, inference workload etc.
- Client accepts multiple images and wrap them in a single request to server
- Each request carries on two inference successively, one for detection and one for attribute classification
- Image transmission from client to server is in form of base64 encoding for good readability and robustness
- Http messages involving file transfer are based on http standard multi-part message
- The http stack is built upon boost asio from socket level. Fine grained control over threads, handlers and workloads
- Server wraps inference results into human-readable json format and sends back to client
- A copy of result is saved on database upon the server. Database is implemented on Redis. Fast and powerful

## Build

Before build make sure you have all the prerequisites installed.

### Build procedure

```shell
source /your/path/to/openvino/bin/setupvars.sh
mkdir build && cd build
cmake ..
make -j12
```
The cmake script has been configured to build Release by default and also some security flags and definitions on. If you would like to build Debug version then add `-DCMAKE_BUILD_TYPE=Debug` at the cmake command

## Run

Make sure your redis database is up and running on localhost 6379 port.
Start the server.
```shell
./SISDServer
```
Then run client.
```shell
cd build/bin
cp ../../models/* .
# have your input image ready as well
./SISDClient -t predict -i 1.jpeg -i 2.jpeg
```

## Client options

The client application provides the following options
```
Example usages: SISDClient -i 1.jpeg -i 2.jpeg

Options:
  -h [ --help ]         Produce this help message
  -i [ --image ] arg    Input image to be inferenced. Can specify multiple 
                        times
  -t [ --type ] arg     Request type. Value could be predict or history

```

## Models

All models used in this repository are from Intel Open Model Zoo. A copy of some specific models are also available at `models`
