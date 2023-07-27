import sys
import os
import math

path = sys.argv[1]
source = path
description = sys.argv[2]
filepath = sys.argv[3]

if path.endswith('.txt'):
    path = path[:-4]
else:
    sys.exit("File does not have correct format")
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
