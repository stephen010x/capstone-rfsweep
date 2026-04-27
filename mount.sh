#!/bin/bash
mkdir -p mount
sshfs -o password_stdin user@10.42.0.1:/home/user mount <<< 'pass'
