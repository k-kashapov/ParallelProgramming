# MPI labs

This reposirory contains solutions for parallel programming tasks.

All measured timings are in the respective timings.txt file.

## Message Timing

This programm measures average time needed to send and receive a single message using MPI.

It sends user specified amount of messages and divides the overall time by the same amount.

Avg time: 3 us

## Pi Calculation

This programm calculates Pi using the arctan(x) series at x = 1.

## Lab 1

This programm solves the one-dimensional heat equation `T(x, t), {{x, x_start, x_end}, {t, 0, t_end}})` numerically.
The initial conditions required are T(x_start, t), T(x_end, t), T(x, 0)
The output is temperature distribution T(x, t_end)

Example output
![Example of heatmap](https://github.com/k-kashapov/ParallelProgramming/blob/master/img/Example.png)
