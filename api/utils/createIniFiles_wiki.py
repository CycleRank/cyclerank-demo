import sys
import os
import math

languages = ["enwiki/", "dewiki/",  "eswiki/", "frwiki/", "itwiki/", "nlwiki/", "plwiki/", "ruwiki/", "svwiki/"]
languages_full = ["English", "German", "Spanish", "French", "Italian", "Dutch", "Polish", "Russian", "Swedish"]
dates = ["2013-03-01/"]

i = -1

for language in languages:
    i = i+1
    for date in dates:
        source = "/mnt/fluiddata/cyclerank/code/cyclerank-api/data/" + language.replace("/", "") + ".wikigraph." + date.replace("/", "") + ".cleaned.csv"
        dest = "/mnt/fluiddata/cyclerank/code/cyclerank-api/data/" + language.replace("/", "") + ".wikigraph." + date.replace("/", "") + ".cleaned.ini"

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

        new_file.write("name = \"" + "wikilinkgraph " + language.replace("wiki/", "") + " " + date.replace("/", "") + "\"\n")
        new_file.write("size = \"" + "{:,}".format(size) + "\"\n")
        new_file.write("info = \"" + "Snapshot of " + date.replace("/", "") + "'s " + languages_full[i] + " Wikipedia." + "\" \n")

        new_file.close()
        print(language, " ", date, " .ini completed.")
