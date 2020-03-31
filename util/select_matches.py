import bz2
import json
import random

random.seed(42)

with open("matches.json", "rt") as f_in, bz2.open("selected_matches.json.bz2", "wt") as f_out:
    for line in f_in:
        doc = json.loads(line)
        if random.random() < 0.1:
            doc["query_user_id"] = 0
            doc["sick_user_id"] = 1
            for row in doc["query_rows"]:
                row["user_id"] = 0
            for row in doc["sick_rows"]:
                row["user_id"] = 1
            for row in doc["query_samples"]:
                row["user_id"] = 0
            for row in doc["sick_samples"]:
                row["user_id"] = 1
            json.dump(doc, f_out)
            f_out.write("\n")
