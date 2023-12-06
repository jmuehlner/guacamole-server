#!/bin/bash

set -e
set -x

# Enable fancy pattern matching on filenames (provides wildcards that can match
# multiple contiguous digits, among others)
shopt -s extglob

# Strip architecture suffix
for LIB in /quasi-msys2/root/mingw64/bin/lib*-x64.dll; do
    ln -sfv "$LIB" "$(echo "$LIB" | sed 's/-x64.dll$/.dll/')"
done

# Automatically add symlinks that strip the library version suffix
for LIB in /quasi-msys2/root/mingw64/bin/lib*-+([0-9]).dll; do
    ln -sfv "$LIB" "$(echo "$LIB" | sed 's/-[0-9]*\.dll$/.dll/')"
done

# Automatically add symlinks that strip the library version suffix
for LIB in /quasi-msys2/root/mingw64/bin/lib*([^0-9.])@([^0-9.-])+([0-9]).dll; do
    ln -sfv "$LIB" "$(echo "$LIB" | sed 's/[0-9]*\.dll$/.dll/')"
done

ln -s /quasi-msys2/root/mingw64 /mingw64