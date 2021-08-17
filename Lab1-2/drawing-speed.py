
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import font_manager


#读取数据

data = pd.read_excel('D:\\UCAS\\Computer Network Lab\\network-labs\\Lab1-2\\data.xlsx',engine='openpyxl')
 
plt.figure(figsize=(20,10))#设置画布的尺寸
plt.xlabel(u'Bandwidth Improvement ',fontsize=20)#设置x轴，并设定字号大小
plt.ylabel(u'Speed Improvement ',fontsize=20)#设置y轴，并设定字号大小
 
#color：颜色，linewidth：线宽，linestyle：线条类型，label：图例，marker：数据点的类型
plt.plot(data['带宽'],data['1MBs'],'om',color="deeppink",linewidth=1.5,linestyle='-',label='1MB')
plt.plot(data['带宽'],data['10MBs'],'sm',color="royalblue",linewidth=1.5,linestyle='-',label='10MB')
plt.plot(data['带宽'],data['100MBs'],'^m',color="aqua",linewidth=1.5,linestyle='-',label='100MB')
plt.plot(data['带宽'],data['equal'],color="orange",linewidth=1.5,linestyle='-',label='balance')


plt.legend(loc=2)#图例展示位置，数字代表第几象限
plt.savefig('speed.svg')

plt.show()#显示图像

