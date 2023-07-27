import sys
import os
import math

sources = ["twarc-networks/"]
files = ["8m.txt"]
names = ["International women's day tweets"]
info = ["Twitter network representing interaction between users (retweets, replies, quotes and mentions) with keywords pertinent to '8m'."]

for i in range(1):
    source = "/mnt/fluiddata/cyclerank/datasets/" + sources[i] + files[i]
    dest = "/mnt/fluiddata/cyclerank/code/cyclerank-api/data/" + files[i].replace(".txt", "") + ".ini"

    a_file = open(source, "r")

    nodesAndEdges = a_file.readline()
        
    edges = "{:,}".format(int(nodesAndEdges.split(" ")[1]))
    nodes = "{:,}".format(int(nodesAndEdges.split(" ")[0]))
    a_file.close()
        
    file_size = os.path.getsize(source)
    size = file_size / 1024
    size = math.ceil(size)

    new_file = open(dest, "w")
        
    new_file.write("N = \"" + nodes + "\"\n")
    new_file.write("M = \"" + edges + "\"\n")

    new_file.write("name = \"" + names[i] + "\"\n")
    new_file.write("size = \"" + "{:,}".format(size) + "\"\n")
    new_file.write("info = \"" + info[i] + "\" \n")

    new_file.close()
    print(sources[i], " ", files[i], " .ini completed.")
