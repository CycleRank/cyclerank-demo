import sys
import re
import os
import math

# step 1. convert
path = sys.argv[1]
description = sys.argv[2]
filepath = sys.argv[3]
old_file = open(path, "r")

if path.endswith('.net'):
    path = path[:-4]
else:
    sys.exit("File does not have correct format")
new_file = open(path+'.txt', "w")

lines = old_file.readlines()
old_file.close()

# remove commented lines and empty lines
lines = list(filter(lambda k: ('\n' != k and not k.startswith('%')), lines))

i = 0
edgeCounter = 0
nodeCounter = 0

isEdgeList = False

edges = []
edgesList = {}
labels = {}

tokens = re.split(r'\t|\n| ', lines[i])

# search for line with the vertices delimiter (should be the first one we encounter)
while tokens[0] != "*Vertices" and tokens[0] != "*vertices":
    i = i + 1
    tokens = re.split(r'\t|\n| ', lines[i])
nodeCounter = tokens[1]
i = i + 1
tokens = re.split(r'\t|\n| ', lines[i])

# take note of all the vertices until we find the edge list delimiter
while tokens[0] != "*arcs" and tokens[0] != "*Arcs" and tokens[0] != "*Edges" and tokens[0] != "*edges" and tokens[0] != "*edgesList" and tokens[0] != "*EdgesList": 
    if '"' in lines[i]:
        tokens[1] = lines[i].split('"')[1]
    else:
        # this line is not needed
        tokens[1] = re.split(r'\t|\n| ', lines[i])[1]
    labels[tokens[1]] = int(tokens[0]) - 1
    i = i + 1
    tokens = re.split(r'\t|\n| ', lines[i])
    # tokens = lines[i].split(" ", 1)

if tokens[0] == "*arcs" or tokens[0] == "*Arcs" or tokens[0] == "*Edges" or tokens[0] == "*edges":
    i = i + 1
    while i < len(lines):
        tokens = re.split(r'\t|\n| ', lines[i])
        edges.append((int(tokens[0]) - 1, int(tokens[1]) - 1))
        edgeCounter = edgeCounter + 1
        i = i + 1
elif tokens[0] != "*edgesList" or tokens[0] != "*EdgesList": 
    isEdgeList = True
    i = i + 1
    while i < len(lines):
        tokens = re.split(r'\t|\n| ', lines[i])
        edgesList[int(tokens[0]) - 1] = list(tokens)
        edgeCounter = edgeCounter + 1
        i = i + 1


new_file.write(str(nodeCounter) + " " + str(edgeCounter) + "\n")

if isEdgeList:
    for node in edgesList:
        for i in range(1,len(edgesList[node])):
            if edgesList[node][i] != '':
                new_file.write(str(node) + " ")
                new_file.write(str(int(edgesList[node][i]) - 1) + "\n")
else:
    for edge in edges:
        new_file.write(str(edge[0]) + " " + str(edge[1]) + "\n")

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
