import sys
import re
import os
import math

# step 1. convert
path = sys.argv[1]
description = sys.argv[2]
filepath = sys.argv[3]
old_file = open(path, "r")

if path.endswith('.csv'):
    path = path[:-4]
else:
    sys.exit("File does not have correct format")
new_file = open(path+'.txt', "w")

lines = old_file.readlines()
old_file.close()

lines = list(filter(lambda k: '\n' != k, lines))

i = -1
edgeCounter = len(lines)

edges = []
labels = {}

for line in lines:
    tokens = re.split(r',|;|\t|\n', line)
    first = tokens[0]
    second = tokens[1]
    # print(first + " " + second + "\n")
    edges.append((first, second))
    if (first not in labels):
        i = i + 1
        labels[first] = i

    if (second not in labels):
        i = i + 1
        labels[second] = i


new_file.write(str(i+1) + " " + str(edgeCounter) + "\n")

for i in range(len(edges)):
    if edges[i][0] in labels:
        new_file.write(str(labels[edges[i][0]]))
    else:
        new_file.write(edges[i][0])
    new_file.write(" ")
    if edges[i][1] in labels:
        new_file.write(str(labels[edges[i][1]]))
    else:
        new_file.write(edges[i][1])
    new_file.write("\n")

for i in labels:
    new_file.write(str(labels[i]) + " " + i + "\n")


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
