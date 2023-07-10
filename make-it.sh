#!/bin/bash

autoreconf -fi
LDFLAGS="-L/usr/local/lib" ./configure
make
