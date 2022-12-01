FROM alpine:3.17

WORKDIR /home

RUN apk add build-base git
RUN git clone https://github.com/joshlengel/cserv && cd cserv && make

ENTRYPOINT [ "/home/cserv/example/bin/cserv_example" ]