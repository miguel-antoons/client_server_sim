import matplotlib.pyplot as plt
import numpy as np
import csv

x = []
y = []
  
with open('myFile.csv','r') as csvfile:
    plots = csv.reader(csvfile, delimiter = ',')

    print(plots)
      
    for row in plots:
        print(row)
        x.append(int(row[0]))
        y.append(row[1]) 
  
plt.plot(x, y)
plt.xlabel('Number of request')
plt.ylabel('time')
plt.title('number of request per second')
plt.legend()
plt.show()
