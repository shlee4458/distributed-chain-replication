<h1 align="center"> Replicated Distributed Systems using Primary-Backup protocol </h1>

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
# Table of Contents

- [About](#About)
- [Libraries](#Libraries)
- [Getting Started](#getting-started)
- [Class Structure](#class-structure)
- [Implementation](#implementation)
- [Evaluation](#evaluation)
- [Improvements](#improvements)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->
<br>

# About

<i> Note: Distributed Systems - Multithreaded Factory is a console-based Server and Client Interface that was developed as part of the CS 7610 Foundation of Distributed Systems in Northeastern University.</i> 

This is a replicated distributed system that allows record update operation through the primary server, replicated to back up server nodes. Read operation is supported both by primary server and backup server.
<br>

# Libraries
```
No external libraries other than C++11 Standard Library was used for the program.
```
<br>

# Getting Started
<i>Note: when running the program in the local environment set ip address as '127.0.0.1' and use port number between 10000 and 65536.</i>

1. Clone the repository:

    ```bash
    git clone [repository_url]
    ```
2. Build the executable:

    ```bash
    make
    ```
3. Start a server in one terminal:

    ```bash
    ./server [port num] [unique ID] [num peers] (repeat [ID] [IP] [port num]
    ```
4. Start a client in another terminal:

    ```bash
    # request_type 1: write request
    # request_type 2: read request (without debug)
    # request_type 3: read request (with debug)
    ./client [ip addr] [port num] [num customers] [num orders] [request_type]
    ```
<br>

# Class Structure
```
Server
- ServerMain: main entrance to the server program
- ServerStub: interface that engineer classes use to interact with serversocket to send data
- ServerSocket: communication channel between client
- ServerEngineer: class that interacts with server stub to send messages remotely to clients
- ServerMetadata: stores metadata of the server including;
    - primary_id: primary server's unique id
    - last_index: last index of the log that was executed
    - commited_index: last committed index within the log vector
    - smr_log: vector of server replication logs
    - neighbors: server node instance that encapsulates server id, ip, port
    - primary_sockets: clientsocket instance that is used to communicate with the neighbors
    - customer_record: key-value mapping of the customer number and order number

Client
- ClientMain: main entrance to the client program
- ClientSocket: communication channel between server
- ClientStub: instance included in a client thread that acts as an interface with client socket for communication
- ClientTimer: timer used to measure the latency of the request

Common
- Messages: message that is sent between server and clients including;
    - CustomerInfo: customer_id, order_num, request_type 
    - LaptopInfo: customer_id, order_number, request_type, engineer_id, admin_id
    - CustomerRecord: customer_id, last_order
    - Identifier: identifier - 1; Replication request, and ACK for replication request / 2; Customer Request
    - ReplicationRequest: last_idx, committed_idx, primary_id, op_code, op_arg1, op_arg2
```
<br>

# Implementation

## Communication between server and client

## Serialization and Deserialization

## Synchronization(ID generation, Thread pool queueing)


<br>

# Evaluation
## Experiments
4 experiments were conducted to understand performance of the program. Each experiment was conducted 3 times. 4 client threads were used for 40,000 write orders per customer.

## Table
<i>Unit: latency - microseconds</i>
#### Experiment 1
<img alt="Experiment 1 Table" title="Experiment 1 Table" src="img/table1.png" width="500">
<br>

### Experiment 2
<img alt="Experiment 2 Table" title="Experiment 2 Table" src="img/table2.png" width="500">
<br>

### Experiment 3
<img alt="Experiment 3 Table" title="Experiment 3 Table" src="img/table3.png" width="500">
<br>

### Experiment 4
<img alt="Experiment 4 Table" title="Experiment 4 Table" src="img/table4.png" width="500">
<br>



## Latency Graph
### Experiment 1
<img alt="Experiment 1 Lat-Graph" title="Experiment 1 Lat-Graph" src="img/latency_graph1.png" width="500">
<br>
This is the experiment where the order did not include custom laptop. When number of customer value was small, the experiment have experienced sudden spike in the maximum latency. Max latency were generally higher than the cases where custom orders were made. We may hypothesis that certain delay in sending the data from server to client increases leveling the latency.


# Improvements