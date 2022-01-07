FROM srsenb

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
		iptables \
        net-tools \
        build-essential \
        cmake

WORKDIR /srsran

COPY ./ /srsran/

WORKDIR /srsran/build
RUN cmake -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug ../
RUN make -j 4 && make install && ldconfig
RUN ./srsran_install_configs.sh user

# COPY ./srsran /opt/srsran
# COPY ./srsran/share/srsran/enb.conf.example /root/.config/srsran/enb.conf
# COPY ./srsran/share/srsran/rb.conf.example /root/.config/srsran/rb.conf
# COPY ./srsran/share/srsran/rr.conf.example /root/.config/srsran/rr.conf
# COPY ./srsran/share/srsran/sib.conf.example /root/.config/srsran/sib.conf
# COPY ./srsran/share/srsran/ue.conf.example /root/.config/srsran/ue.conf
# COPY ./srsran/share/srsran/epc.conf.example /root/.config/srsran/epc.conf
# COPY ./srsran/share/srsran/mbms.conf.example /root/.config/srsran/mbms.conf
# COPY ./srsran/share/srsran/user_db.csv.example /root/.config/srsran/user_db.csv

# RUN ldconfig

# ENV LD_LIBRARY_PATH /usr/lib/gcc/x86_64-linux-gnu/7/:/opt/srsran/lib:$LD_LIBRARY_PATH
# ENV PATH /opt/srsran/bin:$PATH
