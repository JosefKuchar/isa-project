#!/bin/bash
# NEODEVZDAVAT PROSIM !!!

# Get port from stdin
port=$1

# Function that performs call to server
# @param $1: string with payload
function call_server {
    # Print payload to stdout as string
    echo -n "Sending payload: "
    echo -ne "$1" | xxd -c 32
    # Call server
    echo -ne "$1" | nc -u -w 0 localhost $port
    # Sleep for 1 second
    # sleep 1
}

# Nullbytes
call_server "\x00"
call_server "\x00\x00"
call_server "\x00\x00\x00"
call_server "\x00\x00\x00\x00"
call_server "\x00\x00\x00\x00\x00"
call_server "\x00\x00\x00\x00\x00\x00"
call_server "\x00\x00\x00\x00\x00\x00\x00"
call_server "\x00\x00\x00\x00\x00\x00\x00\x00"

# Missing stuff
call_server "\x00\x02\x00\x00"
call_server "\x00\x02aaaaaa"
call_server "\x00\x02bbbbbb\x00"
call_server "\x00\x02cccccc\x00\x00"
call_server "\x00\x02dddddd\x00octet"
call_server "\x00\x02eeeeee\x00octetttt"

# Invalid opcodes
call_server "\xff\x02abcd\x00octet\x00"
call_server "\x00\xffabcd\x00octet\x00"
call_server "\xff\xffabcd\x00octet\x00"

# Invalid filepath
call_server "\x00\x02/root/\x00octet\x00"
call_server "\x00\x02/root/a\x00octet\x00"
call_server "\x00\x02\x00octet\x00"
call_server "\x00\x02/\x00octet\x00"

# Invalid mode
call_server "\x00\x02123456\x00octett\x00"
call_server "\x00\x02asdfasdf\x00textasciii\x00"
call_server "\x00\x02asdfasf\x00randomgarbage\x00"

# Options parsing general handling
call_server "\x00\x021\x00octet\x00tsize"
call_server "\x00\x022\x00octet\x00tsize\x00"
call_server "\x00\x023\x00octet\x00tsize\x001234"
call_server "\x00\x024\x00octet\x00tsize\x001234\x00yes"

# Test invalid tsize
call_server "\x00\x025\x00octet\x00tsize\x00-100\x00"
call_server "\x00\x026\x00octet\x00tsize\x00asdf\x00"
call_server "\x00\x027\x00octet\x00tsize\x00999999999999999999\x00"

# Test invalid blocksize
call_server "\x00\x028\x00octet\x00blksize\x000\x00"
call_server "\x00\x029\x00octet\x00blksize\x00-100\x00"
call_server "\x00\x0210\x00octet\x00blksize\x007\x00"
call_server "\x00\x0211\x00octet\x00blksize\x0065465\x00"
call_server "\x00\x0212\x00octet\x00blksize\x00asdf\x00"
call_server "\x00\x0213\x00octet\x00blksize\x00999999999999999999\x00"

# Test invalid timeout
call_server "\x00\x0215\x00octet\x00blksize\x00-100\x00"
call_server "\x00\x0216\x00octet\x00timeout\x00256\x00"
call_server "\x00\x0217\x00octet\x00timeout\x00asdf\x00"
call_server "\x00\x0218\x00octet\x00timeout\x00x00999999999999999999\x00"

# Nonexistent options (this is not an error but potentially can cause problems)
call_server "\x00\x0219\x00octet\x00neexistuje\x00123\x00"
call_server "\x00\x0220\x00octet\x00neexistuje\x00asdf\x00"
