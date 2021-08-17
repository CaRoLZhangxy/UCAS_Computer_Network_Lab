
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager


#读取数据

data = pd.read_excel('data.xlsx',engine='openpyxl')
 
plt.figure(figsize=(15,10))#设置画布的尺寸
plt.xlabel(u'Relative Bandwidth Improvement ',fontsize=20)#设置x轴，并设定字号大小
plt.ylabel(u'Relative FCT Improvement ',fontsize=20)#设置y轴，并设定字号大小
plt.yscale("log")
plt.xscale("log")
#color：颜色，linewidth：线宽，linestyle：线条类型，label：图例，marker：数据点的类型
plt.plot(data['带宽'],data['100MB'],'Dm',color="chartreuse",linewidth=1.5,linestyle=':',label='flow size = 100MB')
plt.plot(data['带宽'],data['10MB'],'^m',color="blue",linewidth=1.5,linestyle=':',label='10MB')
plt.plot(data['带宽'],data['1MB'],'sm',color="black",linewidth=1.5,linestyle=':',label='1MB')


plt.plot(data['带宽'],data['equal'],color="red",linewidth=1.5,linestyle='-',label='Equal improvement in FCT and bandwidth')


plt.legend(loc=2)#图例展示位置，数字代表第几象限
plt.savefig('FCT.svg')

plt.show()#显示图像

