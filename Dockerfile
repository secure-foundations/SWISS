FROM ubuntu:latest

# https://askubuntu.com/questions/909277/avoiding-user-interaction-with-tzdata-when-installing-certbot-in-a-docker-contai
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y clang
RUN apt-get install -y python3-pip

RUN pip3 install matplotlib
RUN pip3 install z3-solver
RUN pip3 install networkx
RUN pip3 install typing-extensions

RUN apt-get install -y wget

### cvc4

COPY cvc4 /home/root/cvc4

WORKDIR /home/root/cvc4
RUN ./contrib/get-antlr-3.4
RUN apt-get install -y cmake
RUN apt-get install -y libgmp3-dev
RUN pip3 install toml
RUN apt-get install -y default-jre
RUN ./configure.sh
WORKDIR /home/root/cvc4/build
RUN make -j4
RUN make install

### swiss dependencies

RUN apt-get install -y python2
COPY scripts/installing/get-pip.py /home/root/get-pip.py
WORKDIR /home/root
RUN python2 get-pip.py
RUN pip2 install ply
RUN pip2 install tarjan

ENV LD_LIBRARY_PATH=/usr/local/lib/

# z3

#RUN pip2 install z3-solver
RUN ln -s /usr/bin/python2 /usr/bin/python
COPY scripts/installing/install-z3.sh /home/root/install-z3.sh
RUN bash install-z3.sh

ENV PYTHONPATH=/usr/local/lib/python2.7/site-packages

# swiss

RUN mkdir /home/root/swiss
COPY mypyvy /home/root/swiss/mypyvy
COPY ivy /home/root/swiss/ivy
COPY Makefile /home/root/swiss/Makefile
COPY LICENSE /home/root/swiss/LICENSE
COPY README.md /home/root/swiss/README.md
COPY benchmarks /home/root/swiss/benchmarks
COPY run-simple.sh /home/root/swiss/run-simple.sh
COPY run.sh /home/root/swiss/run.sh
COPY save.sh /home/root/swiss/save.sh
COPY scripts /home/root/swiss/scripts
COPY src /home/root/swiss/src

WORKDIR /home/root/swiss
RUN mkdir logs
RUN make -j4
