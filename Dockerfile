FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y install build-essential gdb vim git

COPY startup.sh /root
ENV LD_LIBRARY_PATH /usr/lib
