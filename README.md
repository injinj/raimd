# Readme for Rai md

[![copr status](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/raimd/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/injinj/gold/package/raimd/)

## What is Rai md

These are message codecs for Tib and RMDS market data messages.  They are only
for the messages, not the transports or any kind of networking.

They are designed for streaming, so there is nothing that will cache or
organize messaging data.  Streaming in this context means iterating over fields
of a market data message without a lot of overhead.  No allocation, no hashing,
or any other value added.

## How build Rai md

This project is make based.  There is a dist\_rpm target to build an RPM
for installation and a dist\_dpkg target for Debian based packaging.

```console
$ sudo dnf install gcc-c++ make chrpath
$ git clone git@github.com:raitechnology/raimd.git
$ cd raimd
$ make
$ FC28_x86_64/bin/test_write
```

## How is Rai md used

These test programs illustrate some of the features

- [Write messages](test/write_msg.cpp)

- [Read messages](test/read_msg.cpp)

- [Use dictionaries](test/test_dict.cpp)

