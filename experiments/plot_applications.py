import matplotlib.pyplot as plt
import numpy as np
import glob
import json
import sys

# Path can include regex characters
# Returns the filename, assuming exactly 1 match
def get_postgres_throughput_latency(path):
    for filename in glob.glob(path):
        with open(filename, "r") as f:
            data = json.load(f)
            return [data["Goodput (requests/second)"], data["Latency Distribution"]["Median Latency (microseconds)"]]

def main():
    mode = sys.argv[1]
    if mode != "latency" and mode != "throughput":
        print("Usage: python3 " + sys.argv[0] + " [latency|throughput]")
        return

    [throughput_trusted_normal, latency_trusted_normal] = get_postgres_throughput_latency("trusted_normal/*.summary.json")
    [throughput_trusted_rollbaccine, latency_trusted_rollbaccine] = get_postgres_throughput_latency("trusted_rollbaccine/*.summary.json")

    fig, ax = plt.subplots(1, 1, figsize=(8, 4))
    bar_types = ("Postgres", )
    x = np.arange(len(bar_types))
    
    multiplier = 0

    if mode == "latency":
        weight_counts = {
            "normal": np.array([latency_trusted_normal]),
            "Rollbaccine": np.array([latency_trusted_rollbaccine])
        }
        ax.set_ylabel("Latency (µs)")
        legend_loc = "left"
    else:
        weight_counts = {
            "normal": np.array([throughput_trusted_normal]),
            "Rollbaccine": np.array([throughput_trusted_rollbaccine])
        }
        ax.set_ylabel("Goodput (requests/second)")
        legend_loc = "right"

    width = 1 / (len(weight_counts) + 1)
    for label, weight_count in weight_counts.items():
        offset = width * multiplier
        rects = ax.bar(x + offset, weight_count, width, label=label)
        ax.bar_label(rects, fmt="%d", label_type="center")
        multiplier += 1

    ax.set_xticks(x + width / 2, bar_types)
    ax.legend(loc="upper " + legend_loc)

    plt.savefig("applications_" + mode + ".pdf", format="pdf", bbox_inches="tight")

if __name__ == "__main__":
    main()