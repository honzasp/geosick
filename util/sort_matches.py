import re
import json

matches = []
with open("selected_matches.json", "rt") as f_in:
    for line in f_in:
        m = re.search(r'"score": ([0-9.e+-]+),', line)
        matches.append((float(m[1]), line))

matches.sort(reverse=True)
i = 0
while len(matches) > 0:
    with open(f"matches_{i}.json", "wt") as f_out:
        for _, line in matches[0:10]:
            f_out.write(line)
            f_out.write("\n")
    matches = matches[10:]
    i += 1
