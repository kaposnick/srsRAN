FROM ubuntu:focal

RUN apt update \
 && DEBIAN_FRONTEND=noninteractive apt install -y \
        libfftw3-dev \
        libmbedtls-dev \ 
        libboost-program-options-dev \
        libconfig++-dev \
        libsctp-dev \ 
        libzmq3-dev \
		libmicrohttpd-dev \
		curl \
		iputils-ping \
		iproute2 \
		iptables

COPY ./srsran /opt/srsran
COPY ./srsran/share/srsran/enb.conf.example /root/.config/srsran/enb.conf
COPY ./srsran/share/srsran/rb.conf.example /root/.config/srsran/rb.conf
COPY ./srsran/share/srsran/rr.conf.example /root/.config/srsran/rr.conf
COPY ./srsran/share/srsran/sib.conf.example /root/.config/srsran/sib.conf
COPY ./srsran/share/srsran/ue.conf.example /root/.config/srsran/ue.conf 

ENV LD_LIBRARY_PATH /opt/srsran/lib:$LD_LIBRARY_PATH
ENV PATH /opt/srsran/bin:$PATH
