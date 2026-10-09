FROM peridigm/trilinos 
LABEL maintainer="John Foster <johntfosterjr@gmail.com>"

ENV HOME /root

WORKDIR /

RUN sed -i -e 's|^deb http://deb.debian.org/debian bookworm main|# deb http://deb.debian.org/debian bookworm main|' \
           -e 's|^deb http://security.debian.org/debian-security bookworm-security main|# deb http://security.debian.org/debian-security bookworm-security main|' \
           -e 's|^deb http://deb.debian.org/debian bookworm-updates main|# deb http://deb.debian.org/debian bookworm-updates main|' \
           -e 's|^# deb http://snapshot.debian.org|deb http://snapshot.debian.org|' \
           /etc/apt/sources.list
RUN apt-get -yq -o Acquire::Check-Valid-Until=false update
RUN apt-get -yq install openmpi-bin
RUN apt-get -yq install openssh-server
RUN apt-get -yq install python3-netcdf4 python3-lxml

#Build Peridigm
RUN mkdir peridigm
WORKDIR /peridigm
ADD src src
ADD test test
ADD scripts scripts
ADD examples examples 
ADD CMakeLists.txt .
RUN mkdir /peridigm/build

WORKDIR /peridigm/build/
RUN cmake \
    -D CMAKE_BUILD_TYPE:STRING=Release \
    -D CMAKE_INSTALL_PREFIX:PATH=/usr/local/peridigm \
    -D TRILINOS_DIR:PATH=/usr/local/trilinos \
    -D CMAKE_CXX_COMPILER:STRING="mpicxx" \
    -D CMAKE_POLICY_DEFAULT_CMP0057:STRING=NEW \
    .. && \
    make -j$(nproc) && make install && \
    cd .. && \
    rm -rf peridigm/build

VOLUME /output
WORKDIR /output
ENV LD_LIBRARY_PATH /usr/local/netcdf/lib
ENV PATH /usr/local/peridigm/bin:$PATH
ENV PATH /usr/local/trilinos/bin:$PATH

RUN mkdir /var/run/sshd
EXPOSE 22
CMD    ["/usr/sbin/sshd", "-D"]
