import matplotlib.pyplot as plt
#import seaborn as sns 
import numpy as np 

lanes=["0","1","2","3"]

data=np.genfromtxt('results.txt', delimiter="|", names=["Delay","0","1","2","3"])

labels = [["" if data[n][j] != 1 else "X" for j in range(len(data[n]))] for n in lanes]

fig, ax = plt.subplots(figsize=(10,2))
im = ax.imshow([data['0'],data['1'],data['2'],data['3']])
fig.colorbar(im, ax=ax,shrink=0.6,aspect=5, label="Link quality")

ax.set_xticks(np.arange(len(data['0'])))
ax.set_yticks(np.arange(len(lanes)))
for i, lane in enumerate(lanes):
    for j, value in enumerate(data["0"]):
        text = ax.text(j, i, labels[i][j], ha="center", va="center", color="black")
ax.set_yticklabels(labels=['Lane 0','Lane 1','Lane 2','Lane 3']) 
plt.xlabel("Delay")
plt.tight_layout()
plt.savefig("eye_diagram.png",dpi=200)
plt.show()