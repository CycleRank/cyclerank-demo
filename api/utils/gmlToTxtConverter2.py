import sys

path = sys.argv[1]
old_file = open(path, "r")

if path.endswith('.gml'):
    path = path[:-4]
new_file = open(path+'.txt', "w")

print("reading file")

lines = old_file.readlines()
old_file.close()

i = -1
edgeCounter = 0
lastNode = 0

edges = []
labels = []

print("reading lines")

for line in lines:
    i = i + 1
    if "edge [" in line:
        edges.append(lines[i+1].split("source ")[1].replace('\n', '') + " " + lines[i+2].split("target ")[1].replace('\n', '') + "\n")
        edgeCounter = edgeCounter + 1

    if "node [" in line:
        lastNode = lines[i+1].split("id ")[1].replace('\n', '')
        labels.append(lastNode + " " + lines[i+2].split("label ")[1].replace('"', '').replace('\n', '') + "\n")


print("writing first line")
lastNode_int = int(lastNode) + 1
new_file.write(str(lastNode_int) + " " + str(edgeCounter) + "\n")

for i in range(len(edges)):
    new_file.write(edges[i])

for i in range(len(labels)):
    new_file.write(labels[i])


new_file.close()

print("completed.")