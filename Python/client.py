import ctypes, struct, threading, socket, re, time, logging

fmt = 'I'

class Client(object):
    def __init__(self):
        self.socket = None
        self.socketReader = None
        self.socketWriter = None
        self.magic = ctypes.c_uint32(0x9E2B83C1).value
        self.seqNum = 0

    def isconnected(self):
        return self.socket != None

    def connect(self, host = 'localhost', port = 9000):
        if not self.socket:
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((host, port))
                self.socket = s
                self.socketReader = s.makefile('rb', 1024)
                self.socketWriter = s.makefile('wb', 1024)

            except Exception as e:
                print('Can not connect to', host, ":", port)
                print(e)
                self.socket = None 

            if self.socket != None: # connection established, waiting for welcome message
                response = self.receive()
                return response
            return False  

    def disconnect(self):
        self.socketReader.close()
        self.socketWriter.close()
        self.socket.close()
        self.socket = None

    def request(self, cmd):
        raw_message = '%d' % (self.seqNum) + ':' + cmd
        self.send(raw_message)
        self.seqNum = self.seqNum + 1
        # wait for response
        response = self.receive()
        
        print(response)

        status = response.split(" ")[0].split(":")[1]
        if status == "error":
            return None

        return response

    def receive(self):
        rawMagic = self.socketReader.read(4)
        magic = struct.unpack(fmt, rawMagic)[0] 
        
        if magic != self.magic:
            print('Error: magic number is not matched')
            return False

        raw_payload_size = self.socketReader.read(4)
        payload_size = struct.unpack('I', raw_payload_size)[0]

        payload = ''
        remain_size = payload_size
        while remain_size > 0:
            data = self.socketReader.read(remain_size)
            if not data:
                return False

            payload += data.decode("utf-8")
            bytes_read = len(data) # len(data) is its string length, but we want length of bytes
            remain_size -= bytes_read
        return payload

    def send(self, cmd):
        try:
            self.socketWriter.write(struct.pack(fmt, self.magic))
            self.socketWriter.write(struct.pack(fmt, ctypes.c_uint32(len(cmd)).value))
            self.socketWriter.write(str.encode(cmd))
            self.socketWriter.flush()
            return True
        except Exception as e:
            print('Fail to send message ', cmd, e)
            return False

# define an instance of TCP client 
client = Client()
