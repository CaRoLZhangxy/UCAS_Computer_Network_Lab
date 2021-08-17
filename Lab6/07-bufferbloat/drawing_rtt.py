
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager


#读取数据

data1 = pd.read_csv('qlen-10/rtt.csv',sep = '\t')
data2 = pd.read_csv('qlen-100/rtt.csv',sep = '\t')
data3 = pd.read_csv('qlen-200/rtt.csv',sep = '\t')
plt.figure(figsize=(15,10))#设置画布的尺寸
plt.xlabel(u'time',fontsize=20)#设置x轴，并设定字号大小
plt.ylabel(u'rtt',fontsize=20)#设置y轴，并设定字号大小
#color：颜色，linewidth：线宽，linestyle：线条类型，label：图例，marker：数据点的类型
plt.plot(data1['time']-data1['time'][0],data1['rtt'],color="coral",linewidth=1.5,linestyle='-',label='qlen = 10')
plt.plot(data2['time']-data2['time'][0],data2['rtt'],color="SlateBlue",linewidth=1.5,linestyle='-',label='qlen = 100')
plt.plot(data3['time']-data3['time'][0],data3['rtt'],color="LimeGreen",linewidth=1.5,linestyle='-',label='qlen = 200')


plt.legend(loc=1)#图例展示位置，数字代表第几象限
plt.savefig('rtt.svg')

plt.show()#显示图像

