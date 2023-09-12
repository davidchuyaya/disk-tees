import matplotlib.pyplot as plt
import numpy as np
import glob
import json
import sys

# Path can include regex characters
# Returns the filename, assuming exactly 1 match
def get_throughput_latency(path):
    for filename in glob.glob(path):
        with open(filename, "r") as f:
            data = json.load(f)
            return [data["Goodput (requests/second)"], data["Latency Distribution"]["Median Latency (microseconds)"]]

def main():
    mode = sys.argv[1]
    if mode != "latency" and mode != "throughput":
        print("Usage: python3 " + sys.argv[0] + " [latency|throughput]")
        return

    [throughput_untrusted_normal, latency_untrusted_normal] = get_throughput_latency("untrusted_normal/*.summary.json")
    [throughput_untrusted_tmpfs, latency_untrusted_tmpfs] = get_throughput_latency("untrusted_tmpfs/*.summary.json")
    [throughput_untrusted_fuse, latency_untrusted_fuse] = get_throughput_latency("untrusted_fuse/*.summary.json")
    [throughput_trusted_normal, latency_trusted_normal] = get_throughput_latency("trusted_normal/*.summary.json")
    [throughput_trusted_tmpfs, latency_trusted_tmpfs] = get_throughput_latency("trusted_tmpfs/*.summary.json")
    [throughput_trusted_fuse, latency_trusted_fuse] = get_throughput_latency("trusted_fuse/*.summary.json")
    [throughput_trusted_rollbaccine, latency_trusted_rollbaccine] = get_throughput_latency("trusted_rollbaccine/*.summary.json")

    fig, ax = plt.subplots(layout="constrained")
    bar_types = ("VM", "CVM")
    x = np.arange(len(bar_types))
    width = 0.2
    multiplier = 0

    if mode == "latency":
        weight_counts = {
            "tmpfs": np.array([latency_untrusted_tmpfs, latency_trusted_tmpfs]),
            "fuse": np.array([latency_untrusted_fuse, latency_trusted_fuse]),
            "normal": np.array([latency_untrusted_normal, latency_trusted_normal]),
            "rollbaccine": np.array([np.nan, latency_trusted_rollbaccine])
        }
        ax.set_ylabel("Transaction latency (µs)")
        legend_loc = "left"
    else:
        weight_counts = {
            "tmpfs": np.array([throughput_untrusted_tmpfs, throughput_trusted_tmpfs]),
            "fuse": np.array([throughput_untrusted_fuse, throughput_trusted_fuse]),
            "normal": np.array([throughput_untrusted_normal, throughput_trusted_normal]),
            "rollbaccine": np.array([np.nan, throughput_trusted_rollbaccine])
        }
        ax.set_ylabel("Goodput (requests/second)")
        legend_loc = "right"
    
    for label, weight_count in weight_counts.items():
        offset = width * multiplier
        rects = ax.bar(x + offset, weight_count, width, label=label)
        ax.bar_label(rects, padding=3)
        multiplier += 1

    ax.set_xticks(x + width, bar_types)
    ax.legend(loc="upper " + legend_loc)

    plt.savefig("postgres_compare_" + mode + ".pdf", format="pdf")

if __name__ == "__main__":
    main()