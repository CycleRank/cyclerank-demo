import sys

path = "/mnt/fluiddata/cyclerank/datasets/wikilinkgraphs/"

languages = ["enwiki/"]

dates = ["2013-03-01/"]

for language in languages:
    for date in dates:
        only_edges = path + language + date + language.replace("/", "") + ".wikigraph.pagerank." + date.replace("/", "") + ".csv"
        only_labels = path + language + date + language.replace("/", "") + ".wikigraph.snapshot." + date.replace("/", "") + ".csv"

        dest = "/mnt/fluiddata/cyclerank/code/cyclerank-api/data/" + language.replace("/", "") + ".wikigraph." + date.replace("/", "") + ".cleaned.csv"

        a_file = open(only_edges, "r")

        edges = a_file.readlines()
        a_file.close()

        b_file = open(only_labels, "r")

        labels = b_file.readlines()
        b_file.close()

        new_file = open(dest, "w")
        for line in edges:
            new_file.write(line)

        for line in labels:
            new_file.write(line)

        new_file.close()
        print(language, " ", date, " completed.")
