# HTTP Proxy Server
------------------------------------------------------------------------------------------------------------------------
Description:

In this assignment, we have implement a simple HTTP proxy server and HTTP command line client. This implementation used HTTP/1.0, which is specified in RFC 1945.
Boyu Li Implemented client.c and some section of helper.c file and Kavya implemented server.c and some section of helper.c file.

--------------------------------------------------------------------------------------------------------------------------
Directory Structure:

bin - contains the binary client and server

include - contains the file common.h

src - contains the source file.

--------------------------------------------------------------------------------------------------------------------------
Running the code:
1. Run the command: make clean
2. Run the command: make all
3. Go to directory ./bin
4. For starting http server: ./http <IP_ADDRESS> <PORT>
5. To start the client: ./client <IP_ADDRESS> <PORT> <WEB_ADDRESS>
  

