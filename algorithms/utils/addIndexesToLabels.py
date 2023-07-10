
a_file = open("eu-core-cleaned copy.txt", "r")

lines = a_file.readlines()
a_file.close()

new_file = open("eu-core-cleaned-labels.txt", "w")
i = 0
for line in lines:
    newline = str(i) + " " + line
    new_file.write(newline)
    i = i+1
new_file.close()