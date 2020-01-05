import csv
import os

files = os.listdir()
cnt = 0
for file in files:
    if '.csv' == file[-4:]:
        with open(file, 'rt', encoding="utf-8") as csvfile:
            reader = csv.reader(csvfile)
            column2 = [row[2] for row in reader]
            column3 = [row[3] for row in reader]
            for i in range(len(csvfile.readlines())):
                if column2[i] != column3[i]:
                    cnt += 1

print('cnt = ' + str(cnt))

