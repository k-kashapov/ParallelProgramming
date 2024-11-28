#! /bin/python3

import matplotlib.pyplot as plt
import numpy as np

with open("timing.csv", "r") as f:
    data = [line.split(",") for line in f.read().split("\n")]

    x = []
    y = []
    for line in data:
        x.append(int(line[0]))
        y.append(float(line[1]))

    # print(x, y)

    x_tgt = np.arange(0.8, x[-1], 1e-2)
    y_tgt = (x_tgt ** -1) * y[0]

    plt.grid()
    plt.plot(x, y, '.-b')
    plt.plot(x_tgt, y_tgt, '-r')
    plt.xticks(range(x[-1] + 1))
    plt.xlabel("Threads")
    plt.ylabel("Time, s")
    plt.savefig("Timing.png")
