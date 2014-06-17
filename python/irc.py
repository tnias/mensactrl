#!/usr/bin/env python2

import sys
import socket
import string
import client

def printline(nick, msg, me=False):
  l = len(nick) + 3
  if me:
    l = len(nick) + 1
    nick += " "
  else:
    nick = "<%s> " % nick
  while msg:
    client.writeline("%s%s" % (nick, msg[:96-len(nick)]))
    nick = " " * len(nick)
    msg = msg[96-len(nick):].strip()

HOST="irc.freenode.net"
PORT=6667
NICK="mensadisplay"
IDENT="mensadisplay"
REALNAME="MensaDisplay"
readbuffer=""

s=socket.socket( )
s.connect((HOST, PORT))
s.send("NICK %s\r\n" % NICK)
s.send("USER %s %s bla :%s\r\n" % (IDENT, HOST, REALNAME))
s.send("JOIN #stratum0\r\n")
while 1:
  readbuffer=readbuffer+s.recv(1024)
  temp=string.split(readbuffer, "\n")
  readbuffer=temp.pop( )

  for line in temp:
    line = string.rstrip(line)
    line = string.split(line)
    print line
    if line[1] == "PRIVMSG":
      nick = line[0].split("!")[0][1:]
      msg = " ".join(line[3:])[1:]
      try:
        if msg.startswith("\x01ACTION"):
          #me
          printline(nick, msg[8:-1].decode("utf8"), True)
        else:
          printline(nick, msg.decode("utf8"))
      except UnicodeEncodeError:
        pass
    if(line[0]=="PING"):
      s.send("PONG %s\r\n" % line[1])