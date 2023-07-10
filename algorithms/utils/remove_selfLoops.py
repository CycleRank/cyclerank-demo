
a_file = open("eu-core.txt", "r")

lines = a_file.readlines()
a_file.close()

new_file = open("eu-core-cleaned.txt", "w")
for line in lines:
    sourceAndDest = [int(s) for s in line.split() if s.isdigit()]
    if sourceAndDest[0] != sourceAndDest[1]:
        new_file.write(line)
new_file.close()