FROM ubuntu:latest

# Install build-essential which includes gcc and g++
# RUN apt-get update && apt-get install -y build-essential && apt-get install libmysqlclient-dev && apt-get install libcurl4-openssl-dev

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libmysqlclient-dev \
    libcurl4-openssl-dev \
    mysql-server \
    libxxhash-dev \
    libboost-filesystem-dev \
    libboost-system-dev \
    && apt install libboost-filesystem-dev libboost-system-dev   \
    # added now
    && rm -rf /var/lib/apt/lists/*


# Expose ports 3001, 3002, 3003
EXPOSE 31001 31002 31003

# COPY server.cpp /app/server.cpp

WORKDIR /app

# Compile the C++ server application
# RUN g++ -o server server.cpp

# Set the default command to run when the container starts
CMD ["bash"]