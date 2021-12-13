FROM ubuntu:focal

RUN apt update && \
	DEBIAN_FRONTEND=noninteractive apt install -y \ 
	build-essential \
	cmake \
	libfftw3-dev \ 
	libmbedtls-dev \ 
	libboost-program-options-dev \ 
	libconfig++-dev \
	libsctp-dev \
	libzmq3-dev \
	curl \
	iputils-ping \
	iproute2 \
	iptables \
	unzip \
	git

WORKDIR /srsran

# Pinned git commit used for this example
ARG COMMIT=5275f33360f1b3f1ee8d1c4d9ae951ac7c4ecd4e

# Download and build
RUN git clone https://github.com/srsRAN/srsRAN.git ./
RUN git fetch origin ${COMMIT}
RUN git checkout ${COMMIT}

WORKDIR /srsran/build

RUN cmake ../
RUN make
RUN make install

WORKDIR /srsran

	

	
	
 

