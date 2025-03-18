#from alpine/git
from ubuntu:24.04

#RUN apk add --no-cache curl
#todo install without cache
run apt update
run apt install curl git build-essential nano -y
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

#ENTRYPOINT ['bash']
