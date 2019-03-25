import BaseHTTPServer, SimpleHTTPServer
import ssl, os

HOST_NAME = '192.168.223.3'
PORT_NUMBER=8080

class MyRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'
    timeout = 3000 * 60

    def setup(self):
        "Sets a timeout on the socket"
        self.request.settimeout(self.timeout)
        SimpleHTTPServer.SimpleHTTPRequestHandler.setup(self)

    def do_POST(self):
        #self.protocol_version = 'HTTP/1.0'
        self.send_response(200)
        self.end_headers()
        varLen = int(self.headers['Content-Length'])
        postVars = self.rfile.read(varLen)
        client_ip = self.client_address[0]
        print "Connected: " + client_ip + "\n"
        if not os.path.exists(client_ip):
            os.makedirs(client_ip)
        
        if str.startswith(self.path, "/upload_file="):
            print self.path[13:]
            f = open(client_ip + "/" + self.path[13:], 'wb')
            f.write(postVars)
            f.close()

        #print "file length: " + str(varLen) + "\n"
        print self.path

Handler = MyRequestHandler
httpd = BaseHTTPServer.HTTPServer((HOST_NAME, PORT_NUMBER), Handler)
httpd.socket = ssl.wrap_socket (httpd.socket, keyfile='./key.pem', certfile='./certificate.pem', server_side=True)
httpd.serve_forever()
