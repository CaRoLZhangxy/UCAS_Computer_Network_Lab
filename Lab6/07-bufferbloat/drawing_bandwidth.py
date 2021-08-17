
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager


#读取数据

data1 = pd.read_csv('qlen-10/iperf_log.csv',sep = '\t')
data2 = pd.read_csv('qlen-100/iperf_log.csv',sep = '\t')
data3 = pd.read_csv('qlen-200/iperf_log.csv',sep = '\t')
zero_count_10 = 0
zero_count_100 = 0
zero_count_200 = 0
count10 = 0
count100 = 0
count200 = 0
for data in data1['bandwidth']:
    count10 += 1
    if (data == 0.00):
        zero_count_10 += 1
for data in data2['bandwidth']:
    count100 += 1
    if (data == 0.00):
        zero_count_100 += 1
for data in data3['bandwidth']:
    count200 += 1
    if (data == 0.00):
        zero_count_200 += 1
data3=data3[~(data3['bandwidth'].isin(['0.0']))]
data2=data2[~(data2['bandwidth'].isin(['0.0']))]
data1=data1[~(data1['bandwidth'].isin(['0.0']))]
plt.figure(figsize=(15,10))#设置画布的尺寸
plt.xlabel(u'time',fontsize=20)#设置x轴，并设定字号大小
plt.ylabel(u'bandwidth',fontsize=20)#设置y轴，并设定字号大小
#color：颜色，linewidth：线宽，linestyle：线条类型，label：图例，marker：数据点的类型
plt.plot(data1['time']-data1['time'][0],data1['bandwidth'],color="coral",linewidth=1.5,linestyle='-',label='qlen = 10 bandwidth=0:{:.2%}'.format(zero_count_10/count10))
plt.plot(data2['time']-data2['time'][0],data2['bandwidth'],color="SlateBlue",linewidth=1.5,linestyle='-',label='qlen = 100 bandwidth=0:{:.2%}'.format(zero_count_100/count100))
plt.plot(data3['time']-data3['time'][0],data3['bandwidth'],color="LimeGreen",linewidth=1.5,linestyle='-',label='qlen = 200 bandwidth=0:{:.2%}'.format(zero_count_200/count200))


plt.legend(loc=1)#图例展示位置，数字代表第几象限
plt.savefig('bandwidth_1.svg')

plt.show()#显示图像

