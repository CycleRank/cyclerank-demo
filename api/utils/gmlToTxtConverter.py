import sys
import os
import math

# step 1. convert
path = sys.argv[1]
description = sys.argv[2]
filepath = sys.argv[3]
old_file = open(path, "r")

if path.endswith('.gml'):
    path = path[:-4]
else:
    sys.exit("File does not have correct format")
new_file = open(path+'.txt', "w")

# print("reading file")

lines = old_file.readlines()
old_file.close()

i = -1
edgeCounter = 0
lastNode = 0

edges = []
labels = []

# print("reading lines")

for line in lines:
    i = i + 1
    if "edge [" in line:
        edges.append(lines[i+1].split("source ")[1].replace('\n', '') + " " + lines[i+2].split("target ")[1].replace('\n', '') + "\n")
        edgeCounter = edgeCounter + 1

    if "node [" in line:
        lastNode = lines[i+1].split("id ")[1].replace('\n', '')
        labels.append(lastNode + " " + lines[i+2].split("label ")[1].replace('"', '').replace('\n', '') + "\n")


# print("writing first line")
lastNode_int = int(lastNode) + 1
new_file.write(str(lastNode_int) + " " + str(edgeCounter) + "\n")

for i in range(len(edges)):
    new_file.write(edges[i])

for i in range(len(labels)):
    new_file.write(labels[i])


new_file.close()

# step 2. create ini
source = path+'.txt'
dest = path+'.ini'

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

new_file.write("name = \"" + path.replace(filepath + '/', '') + "\"\n")
# new_file.write("name = \"" + path + "\"\n")
new_file.write("size = \"" + "{:,}".format(size) + "\"\n")
new_file.write("info = \"" + description + "\" \n")

new_file.close()
