
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager


#读取数据

data = pd.read_table('client_loss10.txt',sep = '\t',names = ['time','cwnd'])

plt.figure(figsize=(15,10))#设置画布的尺寸
plt.xlabel(u'time',fontsize=20)#设置x轴，并设定字号大小
plt.ylabel(u'cwnd (bytes)',fontsize=20)#设置y轴，并设定字号大小
#color：颜色，linewidth：线宽，linestyle：线条类型，label：图例，marker：数据点的类型
plt.plot( data['time'] /1000000,data['cwnd'] * 1460,color="coral",linewidth=1.5,linestyle='-',label='cwnd loss = 10')

plt.legend(loc=1)#图例展示位置，数字代表第几象限
plt.savefig('cwnd_loss10.svg')

plt.show()#显示图像

