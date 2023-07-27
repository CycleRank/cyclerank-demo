import networkx as nx
import sys

path = sys.argv[1]
G = nx.read_gml(path)

if path.endswith('.gml'):
    path = path[:-4]
new_file = open(path+'.txt', "w")
new_file.write(str(G.number_of_nodes()) + " " + str(G.number_of_edges()) + "\n")

listGraph = list(G)

for edge in G.edges(data=True):
    new_file.write(str(listGraph.index(edge[0])) + " " + str(listGraph.index(edge[1])) + "\n")

counter = 0
for node in G.nodes():
    new_file.write(str(counter)+ " " + node + "\n")
    counter = counter+1

new_file.close()