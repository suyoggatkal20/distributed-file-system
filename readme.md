# Guide for Installation and use


## Installations

### To run the code you need to have following dependencies fulfilled: 

## CC/G++ Compiler:
Package: build-essential (on Debian/Ubuntu) or gcc and gcc-c++ (on CentOS/Fedora).
Required for compiling C++ code.

## libpthread:
Package: This is usually bundled with the standard C library in most Linux distributions, so no separate installation is typically needed.
Required for threading support (-pthread flag).

## MySQL Client Library:
Package: libmysqlclient-dev (Debian/Ubuntu) or mysql-devel (CentOS/Fedora).
Provides the -lmysqlclient dependency for MySQL database operations.

## CURL Library:
Package: libcurl4-openssl-dev (Debian/Ubuntu) or libcurl-devel (CentOS/Fedora).
Needed for -lcurl dependency to handle HTTP requests and related network operations.


## Boost Filesystem Library (-lboost_filesystem)
Package: libboost-filesystem-dev (Debian/Ubuntu) or boost-devel (CentOS/Fedora).
Used for filesystem operations.

## Boost System Library (-lboost_system)
Package: libboost-system-dev (Debian/Ubuntu) or boost-devel (CentOS/Fedora).
Used internally by the Boost Filesystem library for handling errors.

## XXHash Library (-lxxhash)
A fast hashing library for checksum operations.
Package:
libxxhash-dev (Debian/Ubuntu)
xxhash-devel (CentOS/Fedora)



### Here are the command to install these dependencies:
```bash
sudo apt update
sudo apt install build-essential libmysqlclient-dev libcurl4-openssl-dev

sudo apt install libboost-filesystem-dev libboost-system-dev

sudo apt install libxxhash-dev
```


## Database setup

###  MySql server need to be installed on machine and following tables need to be created where Master node

Create table using following commands for Master node.

## Nodes table
```MySql
CREATE TABLE `nodes` (
  `node_id` int NOT NULL AUTO_INCREMENT,
  `node_ip` varchar(50) NOT NULL,
  `total_storage` bigint NOT NULL,
  `free_space` bigint NOT NULL,
  `status` enum('ACTIVE','INACTIVE') DEFAULT 'ACTIVE',
  `last_heartbeat` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`node_id`),
  UNIQUE KEY `node_ip` (`node_ip`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This tables stores the information about nodes.
we need to populate the information about all the worker nodes. Names of the columns are self explanatory.
Also note that node_ip column should contain ipaddress and port number saperated by colon(:).
<ID>:<PORT>

## Directories table
```MySql
CREATE TABLE `directories` (
  `directory_id` int NOT NULL AUTO_INCREMENT,
  `parent_directory_id` int DEFAULT NULL,
  `directory_name` varchar(255) NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`directory_id`),
  UNIQUE KEY `unique_parent_directory_name` (`parent_directory_id`,`directory_name`),
  CONSTRAINT `directories_ibfk_1` FOREIGN KEY (`parent_directory_id`) REFERENCES `directories` (`directory_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=51 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This tables stores the information about directories.
Initially this table should contain the information of root directory.
```
INSERT INTO directories (directory_name, parent_directory_id)
VALUES ('/', NULL);
```

## Files table

```
CREATE TABLE `files` (
  `file_id` int NOT NULL AUTO_INCREMENT,
  `file_name` varchar(255) NOT NULL,
  `file_path` varchar(255) NOT NULL,
  `file_size` bigint NOT NULL,
  `num_chunks` int NOT NULL,
  `directory_id` int NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`file_id`),
  UNIQUE KEY `unique_file_name_directory` (`file_name`,`directory_id`),
  KEY `directory_id` (`directory_id`),
  CONSTRAINT `files_ibfk_1` FOREIGN KEY (`directory_id`) REFERENCES `directories` (`directory_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This table stores the information of files stored in file system.

## Chunks table
```
CREATE TABLE `chunks` (
  `chunk_id` int NOT NULL AUTO_INCREMENT,
  `file_id` int NOT NULL,
  `chunk_order` int NOT NULL,
  `chunk_size` bigint NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`chunk_id`),
  KEY `file_id` (`file_id`)
) ENGINE=InnoDB AUTO_INCREMENT=11103 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This table stores the information about the chunks and their association with files along with sequence.

## Chunk Replicas table
```
CREATE TABLE `chunk_replicas` (
  `replica_id` int NOT NULL AUTO_INCREMENT,
  `chunk_id` int NOT NULL,
  `node_id` int NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`replica_id`),
  KEY `chunk_id` (`chunk_id`),
  KEY `node_id` (`node_id`),
  CONSTRAINT `chunk_replicas_ibfk_1` FOREIGN KEY (`chunk_id`) REFERENCES `chunks` (`chunk_id`) ON DELETE CASCADE,
  CONSTRAINT `chunk_replicas_ibfk_2` FOREIGN KEY (`node_id`) REFERENCES `nodes` (`node_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=597 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This table store the replicas associated with chunks and their location in the set of worker nodes.



## Chunk replicas slave
```
CREATE TABLE `chunk_replicas_slave` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `replica_id` bigint unsigned NOT NULL,
  `chunk_id` bigint DEFAULT NULL,
  `chunk_order` bigint DEFAULT NULL,
  `chunk_size` bigint DEFAULT NULL,
  `file_id` bigint DEFAULT NULL,
  `file_path` varchar(2100) DEFAULT NULL,
  `data_hash` varchar(32) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_replica_id` (`replica_id`)
) ENGINE=InnoDB AUTO_INCREMENT=597 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```
This table stores the information of blocks on worker nodes.

###  MySql server need to be installed on machine and following tables need to be created where worker node is going to run.

## Chunk Replicas Slave table
```
CREATE TABLE `chunk_replicas_slave` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `replica_id` bigint unsigned NOT NULL,
  `chunk_id` bigint DEFAULT NULL,
  `chunk_order` bigint DEFAULT NULL,
  `chunk_size` bigint DEFAULT NULL,
  `file_id` bigint DEFAULT NULL,
  `file_path` varchar(2100) DEFAULT NULL,
  `data_hash` varchar(32) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_replica_id` (`replica_id`)
) ENGINE=InnoDB AUTO_INCREMENT=597 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
```

## Environment setup
Create the config.properties file in each directory (i.e. wfs, fsclient, mfs, mycommon)and put following properties in it.

## fsclient
```
log_res=1
log_req=1
max-record-size=20
block-size=500
read-chunk-size=100
server-addr=<IP OF MASTER NODE> // default port is 8080
```

## fsclient
```
log_res=1
max_record_size=20
block_size=500
PORT=8080
MAX_CONNECTION=10
LOG_REQ=true
log-full-req=1
log-partial-req=0

db.host=<ip of db typically 127.0.0.1>
db.username=<username>
db.password=<password>
db.db_name=<database name>

dfs.replication=2
dfs.replica-storage-policy=least_loaded
dfs.chunk-size=500000
dfs.mins-to-inactive=288000
```


## wfs
```
log_res=1
max_record_size=20
block_size=500
PORT=8080
MAX_CONNECTION=10
LOG_REQ=true
log-full-req=1
log-partial-req=0

db.host=<ip of db typically 127.0.0.1>
db.username=<username>
db.password=<password>
db.db_name=<database name>

dfs.replication=2
dfs.replica-storage-policy=least_loaded
dfs.chunk-size=500
dfs.mins-to-inactive=288000

dfs.fs-tree-branch-number=3
dfs.fs-files-per-dir=5
dfs.magic-number=32876735353287673535
```

## How to run

Execute the following steps in sequence to get desired output.
## mycommon
This is common library that is used in all the following programs so this need to be build first before anything.
run make command in the root directory of this directory to build it.(make sure all the build tools and libraries are installed before this step)
```
make
```

### mfs
This directory contains code for master node.
run following command inside mfs directory to build and run the code
```
make
./bin/app
```
If everything is right you will see message that server is listening on port 8080


### wfs
This directory contains code for worker node.
run following command inside wfs directory to build and run the code
```
make
./bin/app 8081
```
If everything is right you will see message that server is listening on port 8081
start all the nodes declared in nodes table.


### fsclient
This directory contains code for the dfs client program. Before running this we must have mfs and wfs running.
To build this run following command:
```
make
```
use:

### list directories/files in directory
```
./bin/app -l <path of dir from root>
./bin/app -l /docs/hello

```

### Create directory
```bash
./bin/app -c <path of dir from root>
./bin/app -c /docs/hello  #hello will be created inside docs
```

### insert/put file
```bash
./bin/app -p <local dir path> <dfs dir path>
./bin/app -p /home/user/hello.txt /docs/imp/hello.txt  #hello.txt from local will be uploaded to dfs at /docs/imp
```

### get/fetch file
```bash
./bin/app -p <dfs dir path> <local dir path>
./bin/app -l /docs/imp/hello.txt /home/user/hello.txt  #hello.txt dfs will be fetched to local
```





