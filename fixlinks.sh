#!/bin/bash

files=$(find . -type l -print)

for file in $files; do
    oldpath=$(realpath $file)
    newpath=$(realpath $oldpath --relative-to=.)
    
    oldlink=$(readlink $file)
    newlink=$(realpath $newpath --relative-to=$file --no-symlinks)
    
    #echo "$file ($oldpath -> $newpath)"
    #echo "$file ($oldlink -> $newlink)"

    #echo $oldpath $newpath $oldlink $newlink
    
    echo "unlink $f && ln -s $file $newpath"
    #unlink $file
    #ln -s $file $newpath

done
