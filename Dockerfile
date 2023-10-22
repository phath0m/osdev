FROM centos:7

RUN yum update -y
RUN yum install -y gcc gcc-c++ vim git wget m4 perl-Data-Dumper patch gmp-devel mpfr-devel libmpc-devel make nasm grub2 xorriso 

CMD ["/bin/bash"] 