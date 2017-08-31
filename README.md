# simple-ftp
a simple ftp for reviewing my knowledge about ftp

## Implement ftp protocol command:<br>
PASV<br>
RETER<br>
STOR<br>
QUIT<br>

## How to use:<br>

1.run server by visual studio<br>
2.run client by visual studio<br>
3.send PASV to open data transfer socket<br>
then send command<br><br>
e.g.<br><br>
RETER test.txt //download test.txt from server<br>
STOR 123.txt //upload 123.txt to server<br>
QUIT //close the connnection<br>
