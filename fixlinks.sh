#!/bin/bash

files=$(find . -type l -print)
home=$(pwd)

for file in $files; do

    file=$(realpath -s $file)

    pathfrom=$(realpath -s $file --relative-to=.)
    pathto=$(realpath $file --relative-to=.)

    echo "relink $pathfrom -> $pathto"

    cd $(dirname $file)
    


    oldlink=$(readlink $file)
    newlink=$(realpath $oldlink --relative-to=.)

    # echo "cd $(dirname $file)"
    # echo "unlink $file"
    # echo "ln -s $newlink $(basename $file)"
    # echo "cd $home"

    unlink $file
    ln -s $newlink $(basename $file)

    cd $home
done
