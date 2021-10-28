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
            uart.interval = float(arg)
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

def handler(signum, frame):
    global is_exit
    is_exit = True
    #print ("receive a signal %d, is_exit = %d"%(signum, is_exit))

if __name__ == '__main__':

    parse_opt(sys.argv[1:])
    datacon.id = 1
    datacon.datalength = uart.length
    recv_msg = ''
    send_num = 0
    error_num = 0
    recv_num = 0
    flag = 0

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)

    try:
        ser = serial.Serial(uart.device, uart.rate,uart.length,uart.valid,uart.sbit,timeout=uart.timeout)
    except Exception as e:
        print("Can't open %s device"%(uart.device))
        print(e)
        sys.exit()
    

    #print(is_exit)
    while (int(datacon.id) < int(uart.count)+1) and not is_exit:
        datacon.data = 1<<int(datacon.datalength)
        crc_data = str(datacon.id) + str(datacon.datalength) + str(datacon.data)
        datacon.crc = crc_sum(crc_data, len(crc_data))
        send = datacon.start + str(datacon.id).zfill(4) + str(datacon.datalength).zfill(2) + str(datacon.data).zfill(3) + str(datacon.crc).zfill(4) + datacon.end
        send = send.encode('utf-8')
        start_time = time.time()
        if (ser.write(send) == 15):
            datacon.id += 1
            send_num += 1
        else:
            print("send failed! retry")
            continue
        time.sleep(0.1)
        while True:
            while ser.inWaiting() > 0:
                recv_msg += ser.read(1).decode()
            #print(recv_msg)
            if (len(recv_msg) == 15):
                break
            
            end_time = time.time()

            if ((end_time - start_time + 0.1) > uart.timeout):
                flag = 1
                error_num += 1
                print("读取超时，已读内容：%s"%(recv_msg))
                break

        if (recv_msg == send.decode('utf-8')):
            recv_num += 1
            print("收到%d个完整包"%(recv_num), end='\r', flush=True)
        elif (not flag):
            error_num += 1
            print("收到%d个错误包 数据内容:%s"%(error_num,recv_msg), end='\n\r',flush=True)
        flag = 0
        recv_msg = ''
        send = ''
        time.sleep(uart.interval)
    
    print("已发送：%d， 已接收：%d, 错误包：%d, 错包率 = %.2f%%"%(send_num, recv_num, error_num, float(error_num)/float(send_num)*100))
    
    ser.close()