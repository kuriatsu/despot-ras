#! /usr/bin/python3
# -*- coding: utf-8 -*-

import pickle
import pandas as pd
import xml.etree.ElementTree as ET
import math
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import csv
import glob
import os


sns.set(context='paper', style='whitegrid')
hue_order = ["traffic light", "crossing intention", "trajectory"]
eps=0.01
tl_black_list = [
"3_3_96tl",
"3_3_102tl",
"3_4_107tl",
"3_4_108tl",
"3_5_112tl",
"3_5_113tl",
"3_5_116tl",
"3_5_117tl",
"3_5_118tl",
"3_5_119tl",
"3_5_122tl",
"3_5_123tl",
"3_5_126tl",
"3_5_127tl",
"3_6_128tl",
"3_6_137tl",
"3_7_142tl",
"3_8_153tl",
"3_8_160tl",
"3_9_173tl",
"3_9_174tl",
"3_9_179tl",
"3_10_185tl",
"3_10_188tl",
"3_11_205tl",
"3_12_218tl",
"3_12_221tl",
"3_15_241tl",
"3_16_256tl",
"3_16_257tl",
]
opposite_anno_list = ["3_16_259tl", "3_16_258tl", "3_16_249tl"]

log_data = None
data_path = "/home/kuriatsu/Dropbox/data/pie202203"
for file in glob.glob(os.path.join(data_path, "log*.csv")):
    buf = pd.read_csv(file)
    filename =file.split("/")[-1]
    count = int(filename.replace("log_data_", "").split("_")[-1].replace(".csv", ""))
    print("{}".format(filename))

    if count in [0, 1, 2]:
        print("skipped")
        continue

    ## true positive, false positive, true negative, false negative
    trial = filename.split("_")[-1].replace(".csv", "")
    buf["subject"] = filename.replace("log_data_", "").split("_")[0]
    buf["task"] = filename.replace("log_data_", "").split("_")[1]
    correct_list = [] ## true state
    response_list = [] ## responce tp, fp, tn, fn
    for idx, row in buf.iterrows():
        ## when annotation of TL was wrong, flip evaluation
        if row.id in tl_black_list:
            row.last_state = -2
        if row.last_state == -1: # no intervention
            correct_list.append(-1)
            response_list.append(-1)

        elif int(row.last_state) == int(row.state):
            if row.id in opposite_anno_list:
                correct_list.append(1)
                if row.last_state == 1:
                    response_list.append(3)
                elif row.last_state == 0:
                    response_list.append(0)
                else:
                    print(f"last_state{row.last_state}, state{row.state}")
                    response_list.append(4) # ignored=4
            else:
                correct_list.append(0)
                if row.last_state == 1:
                    response_list.append(1)
                elif row.last_state == 0:
                    response_list.append(2)
                else:
                    print(f"last_state{row.last_state}, state{row.state}")
                    response_list.append(4) # ignored=4
        else:
            if row.id in opposite_anno_list:
                correct_list.append(0)
                if row.last_state == 1:
                    response_list.append(1)
                elif row.last_state == 0:
                    response_list.append(2)
                else:
                    print(f"last_state{row.last_state}, state{row.state}")
                    response_list.append(4) # ignored=4
            else:
                correct_list.append(1)
                if row.last_state == 1:
                    response_list.append(3)
                elif row.last_state == 0:
                    response_list.append(0)
                else:
                    print(f"last_state{row.last_state}, state{row.state}")
                    response_list.append(4) # ignored=4

    buf["correct"] = correct_list
    buf["response"] = response_list
    len(correct_list)
    if log_data is None:
        log_data = buf
    else:
        log_data = log_data.append(buf, ignore_index=True)

task_list = {"int": "crossing intention", "tl": "traffic light", "traj":"trajectory"}
subject_data = pd.DataFrame(columns=["subject", "task", "acc", "int_length", "missing"])
for subject in log_data.subject.drop_duplicates():
    for task in log_data.task.drop_duplicates():
        for length in log_data.int_length.drop_duplicates():
            target = log_data[(log_data.subject == subject) & (log_data.task == task) & (log_data.int_length == length)]
            # acc = len(target[target.correct == 1])/(len(target))
            acc = len(target[target.correct == 1])/(len(target[target.correct == 0]) + len(target[target.correct == 1])+eps)
            missing = len(target[target.correct == -1])/(len(target[target.correct != -2])+eps)
            buf = pd.DataFrame([(subject, task_list.get(task), acc, length, missing)], columns=subject_data.columns)
            subject_data = pd.concat([subject_data, buf])
            
# subject_data.acc = subject_data.acc * 100
# subject_data.missing = subject_data.missing * 100
# sns.barplot(x="task", y="acc", hue="int_length", data=subject_data, ci="sd")
# sns.barplot(x="task", y="acc", data=subject_data, ci="sd")

for task in log_data.subject.drop_duplicates():
    for length in log_data.int_length.drop_duplicates():
        acc = log_data[(log_data.task==task) & (log_data.length == length)].mean()
        print(f"task : {task}, length : {length}, acc = {acc}")

fig, ax = plt.subplots()
sns.pointplot(x="int_length", y="acc", data=subject_data, hue="task", hue_order=hue_order, ax=ax, capsize=0.1, ci="sd")
ax.set_ylim(0.0, 1.0)
ax.set_xlabel("request time [s]", fontsize=18)
ax.set_ylabel("accuracy [%]", fontsize=18)
ax.tick_params(labelsize=14)
ax.legend(fontsize=14)
plt.savefig("accuracy_pie_experiment.svg")


def nd(x, u, si):
    return np.exp(-(x-u)**2/(2*si))/(2*np.pi*si)**0.5


u = 0.68
si = 0.1
data = np.random.normal(u, si, size=10000)
plt.hist(data, bins=50, color="#ff7f00", alpha=0.5, label="pie simulation")

u = 0.95
si = 0.1
data = np.random.normal(u, si, size=10000)
plt.hist(data, bins=50, color="#ff7f00", alpha=0.5, label="tl simulation")

tlr_result = pd.read_csv("tlr_result.csv")
plt.hist(tlr_result["likelihood"], bins=50, alpha=0.5, label="tlr result")
pie_result = pd.read_csv("pie_predict_result_valid.csv")
plt.hist(pie_result["likelihood"], bins=50, alpha=0.5, label="pie result")

plt.xlim([0.0, 1.0])
plt.xlabel("likelihood")
plt.ylabel("count")
plt.savefig("likelihood_hist.svg")

def operator_model(time):
    min_time = 1.0
    min_acc = 0.65
    max_acc = 0.8
    slope = 0.03
    if time < min_time:
        return min_acc

    acc = (time-min_time) * slope + min_acc
    return acc if acc <= max_acc else max_acc

acc_data = []
for time in range(0, 10):
    acc_data.append(operator_model(time))

plt.plot(np.arange(0, 10), acc_data)
plt.xlabel("given time t_req")
plt.ylabel("accuracy")
plt.ylim([0.0, 1.0])
plt.savefig("future_model.svg")

