#encoding:utf-8
#from termios import INPCK
import serial
import threading
import time
import signal
import sys, getopt


is_exit = False

class UartInfo(object):
    def __init__(self, device, rate, dbit, valid, sbit, length, interval, timeout, count):
        self.device = device
        self.rate = rate
        self.dbit = dbit
        self.valid = valid
        self.sbit = sbit
        self.length = length
        self.interval = interval
        self.timeout = timeout
        self.count = count


uart = UartInfo("/dev/ttyUSB0", 115200, 0, 0, 0, 0, 0, 0, 0)


class DataInfo(object):
    def __init__(self, start, id, datalength, data, crc, end):
        self.start = start
        self.id = id
        self.datalength = datalength
        self.data = data
        self.crc = crc
        self.end = end

datacon = DataInfo("S", 0, 0, "", 0, "D")
send_list = []
read_list = []

def parse_opt(argv):
    try:
        opts, args = getopt.getopt(argv,"d:r:b:v:s:l:i:t:c:h",["rate=", \
            "data-bits=", "valid=", "stop-bits=", "length=", "interval=", \
            "timeout=", "count="])
    except getopt.GetoptError:
        print ("test.py -d <device> -i <rate> -b <data-bits> -v <valid> -s"),
        print("<stop-bits> -l <length> -i <interval> -t <timeout> -c <count>")
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print ("test.py -d <device> -i <rate> -b <data-bits> -v <valid> -s"),
            print("<stop-bits> -l <length> -i <interval> -t <timeout> -c <count>")
            sys.exit()
        elif opt in ("-d"):
            uart.device = arg
        elif opt in ("-r", "--rate"):
            uart.rate = int(arg)
        elif opt in ("-b", "--data-bits"):
            uart.dbit = int(arg)
        elif opt in ("-v", "--valid"):
            uart.valid = arg
        elif opt in ("-s", "--stop-bits"):
            uart.sbit = int(arg)
        elif opt in ("-l", "--length"):
            uart.length = int(arg)
        elif opt in ("-i", "--interval"):
            uart.interval = int(arg)
        elif opt in ("-t", "--timeout"):
            uart.timeout = float(arg)
        elif opt in ("-c", "--count"):
            uart.count = int(arg)
        else:
            print ("test.py -d <device> -i <rate> -b <data-bits> -v <valid> -s"),
            print("<stop-bits> -l <length> -i <interval> -t <timeout> -c <count>")
            sys.exit()

    if (len(argv) < 18):
        print ("test.py -d <device> -i <rate> -b <data-bits> -v <valid> -s"),
        print("<stop-bits> -l <length> -i <interval> -t <timeout> -c <count>")
        sys.exit()

def crc_sum(data, data_len):
    crc = 0
    i = 0

    try:
        while(i < data_len):
            crc += ord(data[i])
            i += 1
        return crc & 0x00FF
    except Exception as e:
        print("---Exception---:", e)
        exit
 
def sends(ser):
    datacon.id = 1
    datacon.datalength = uart.length
    print('send',time.time())
    while (datacon.id<uart.count+1) and (not is_exit):
        datacon.data = 1<<int(datacon.datalength)
        crc_data = str(datacon.id) + str(datacon.datalength) + str(datacon.data)
        datacon.crc = crc_sum(crc_data, len(crc_data))
        send = datacon.start + str(datacon.id).zfill(4) + str(datacon.datalength).zfill(2) + str(datacon.data).zfill(3) + str(datacon.crc).zfill(4) + datacon.end
        send = send.encode('utf-8')
        #print(send)
        ser.write(send)
        send_list.append(send)
        datacon.id += 1
    print('send',time.time())
    print("发送完成")

 
 
def reads(ser):
    global is_exit
    global send_list
    out = ''
    i = 0
    read_over = 0
    read_data = ''
    error_data = 0
    print('read',time.time())
    while not is_exit:
        #print(ser.inWaiting())
        while ser.inWaiting() > 0 and (not is_exit):
            #print(ser.inWaiting())
            out += ser.read(1).decode()
        if out != '':
            read_data += out
            #print(read_data)
            out = ''
            #print (read_data)
        if len(read_data) / len(send_list[i]) == uart.count:
            break
    print('read',time.time())
    while (i < len(send_list)):
        #print(send_list[i])
        #print(read_data)
        if (send_list[i].decode('utf-8') in read_data):
            # print("收到包个数：%d"%(i+1))
            read_over += 1
        else:
            print("错误包：%s"%(send_list[i].decode('utf-8')))
            error_data += 1 
        i+=1
    
    print("已发送：%d， 已接收：%d, 错报包：%d, 误码率 = %.2f%%"%(len(send_list), read_over, error_data, float(error_data)/float(len(send_list))*100))

def handler(signum, frame):
    global is_exit
    is_exit = True
    # print ("receive a signal %d, is_exit = %d"%(signum, is_exit))

if __name__ == '__main__':

    parse_opt(sys.argv[1:])

    try:
        ser = serial.Serial(uart.device, uart.rate,uart.length,uart.valid,uart.sbit,timeout=uart.timeout)
    except Exception as e:
        print("Can't open %s device"%(uart.device))
        print(e)
        sys.exit()


    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)

    t1 = threading.Thread(target=sends, name='sends', args=(ser,))
    t1.setDaemon(True)
    t1.start()
    t2 = threading.Thread(target=reads, name='reads', args=(ser,))
    t2.setDaemon(True)
    t2.start()

    while True:
        alive = False
        alive = alive or t1.isAlive() or t2.isAlive()
        if not alive:
            break

    ser.close()
    