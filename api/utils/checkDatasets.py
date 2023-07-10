import sys
import os
import math
import subprocess


languages = ["enwiki", "dewiki",  "eswiki", "frwiki", "itwiki", "nlwiki", "plwiki", "ruwiki", "svwiki"]
languages_full = ["English", "German", "Spanish", "French", "Italian", "Dutch", "Polish", "Russian", "Swedish"]
dates = ["2003-03-01", "2008-03-01", "2012-03-01", "2013-03-01", "2018-03-01"]

i = -1

for language in languages:
    i = i+1
    for date in dates:
        args = ("bin/cyclerank", "-s", "0", "-k", "3",  "-f", "data/" +  language + ".wikigraph." + date + ".cleaned.csv")
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        popen.wait()
        output = popen.stdout.read()
        print(output)
        print(language, " ", date, "completed.")
