import csv

f1 = open('qlen-10/cwnd.txt','r')
f2 = open('qlen-10/cwnd.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","cwnd"])
for line in f1:
        line = line.strip('\n')
        if(line.find('cwnd') > -1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('cwnd:')
            line = line[1].split(' ')
            if(len(line)>1):
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])

f1 = open('qlen-10/qlen.txt','r')
f2 = open('qlen-10/qlen.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","qlen"])
for line in f1:
        line = line.strip('\n')
        line = line.split(', ',1)
        if(len(line)>1):
            #f2.write(line[0]+'\t'+line[1]+'\n')
            csv_writer.writerow([float(line[0]),float(line[1])])

f1 = open('qlen-10/rtt.txt','r')
f2 = open('qlen-10/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1 = open('qlen-10/test.log','r')
f2 = open('qlen-10/iperf_log.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","bandwidth"])
for line in f1:
        line = line.strip('\n')
        if(line.find('bits/sec')>-1):
            line = line.split('] ',1)
            line = line[1].split('-',1)
            time = line[0]
            line = line[1].split('Bytes  ',1)
            line = line[1].split(' ',1)
            bandwidth = line[0]
            #f2.write(time+'\t'+bandwidth+'\n')
            csv_writer.writerow([float(time),float(bandwidth)])
f1 = open('qlen-100/cwnd.txt','r')
f2 = open('qlen-100/cwnd.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","cwnd"])
for line in f1:
        line = line.strip('\n')
        if(line.find('cwnd') > -1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('cwnd:')
            line = line[1].split(' ')
            if(len(line)>1):
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])

f1 = open('qlen-100/qlen.txt','r')
f2 = open('qlen-100/qlen.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","qlen"])
for line in f1:
        line = line.strip('\n')
        line = line.split(', ',1)
        if(len(line)>1):
            #f2.write(line[0]+'\t'+line[1]+'\n')
            csv_writer.writerow([float(line[0]),float(line[1])])

f1 = open('qlen-100/rtt.txt','r')
f2 = open('qlen-100/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1 = open('qlen-100/test.log','r')
f2 = open('qlen-100/iperf_log.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","bandwidth"])
for line in f1:
        line = line.strip('\n')
        if(line.find('bits/sec')>-1):
            line = line.split('] ',1)
            line = line[1].split('-',1)
            time = line[0]
            line = line[1].split('Bytes  ',1)
            line = line[1].split(' ',1)
            bandwidth = line[0]
            #f2.write(time+'\t'+bandwidth+'\n')
            csv_writer.writerow([float(time),float(bandwidth)])
f1 = open('qlen-200/cwnd.txt','r')
f2 = open('qlen-200/cwnd.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","cwnd"])
for line in f1:
        line = line.strip('\n')
        if(line.find('cwnd') > -1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('cwnd:')
            line = line[1].split(' ')
            if(len(line)>1):
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])

f1 = open('qlen-200/qlen.txt','r')
f2 = open('qlen-200/qlen.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","qlen"])
for line in f1:
        line = line.strip('\n')
        line = line.split(', ',1)
        if(len(line)>1):
            #f2.write(line[0]+'\t'+line[1]+'\n')
            csv_writer.writerow([float(line[0]),float(line[1])])

f1 = open('qlen-200/rtt.txt','r')
f2 = open('qlen-200/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1 = open('qlen-200/test.log','r')
f2 = open('qlen-200/iperf_log.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","bandwidth"])
for line in f1:
        line = line.strip('\n')
        if(line.find('bits/sec')>-1):
            line = line.split('] ',1)
            line = line[1].split('-',1)
            time = line[0]
            line = line[1].split('Bytes  ',1)
            line1 = line[1].strip(' ')
            line = line1.split(' ',1)
            bandwidth = line[0]
            #f2.write(time+'\t'+bandwidth+'\n')
            csv_writer.writerow([float(time),float(bandwidth)])
f1 = open('taildrop/rtt.txt','r')
f2 = open('taildrop/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1 = open('red/rtt.txt','r')
f2 = open('red/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1 = open('codel/rtt.txt','r')
f2 = open('codel/rtt.csv','w')
csv_writer = csv.writer(f2,delimiter='\t')
csv_writer.writerow(["time","rtt"])
for line in f1:
        line = line.strip('\n')
        if(line.find('time=')>-1):
            line = line.split(',',1)
            time = line[0]
            line = line[1].split('time=')
            if(len(line)>1):
                line = line[1].split(' ')
                #f2.write(time+'\t'+line[0]+'\n')
                csv_writer.writerow([float(time),float(line[0])])
f1.close()
f2.close()