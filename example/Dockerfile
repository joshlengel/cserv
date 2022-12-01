FROM alpine:3.17

WORKDIR /home
COPY bin/cserv_example ./cserv_example
RUN chmod +x ./cserv_example

ENTRYPOINT [ "/home/cserv_example" ]