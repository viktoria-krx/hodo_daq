#!/bin/bash

SOURCE_DIR="/home/hododaq/DAQ/data/"
DEST_DIR="/eos/experiment/asacusa/hodoscope/2025/"

inotifywait -m -e close_write --format '%w%f' "$SOURCE_DIR""data_root/" | while read NEWFILE
do
    echo "New file detected: $NEWFILE"
    rsync -a -r "$SOURCE_DIR" "$DEST_DIR"
done