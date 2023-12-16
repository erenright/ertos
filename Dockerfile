FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y \
	make \
	lib32z1 \
	wget \
	bzip2 \
	gdb-multiarch

RUN wget -qO - https://files.embeddedts.com//ts-arm-sbc/ts-7250-linux/cross-toolchains/crosstool-linux-gcc-3.3.4-glibc-2.3.2-0.28rc39.tar.bz2 | tar jxf - -C /

ENV PATH="${PATH}:/usr/local/opt/crosstool/arm-linux/gcc-3.3.4-glibc-2.3.2/bin"
