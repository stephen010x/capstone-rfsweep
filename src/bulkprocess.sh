#!/bin/bash


echo $1/data*
for f in $1/data*; do ./process.py "$f" & done; wait
# for f in $1/data*; do ./process.py "$f"; done
