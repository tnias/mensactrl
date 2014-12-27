#!/usr/bin/env python2

import datetime
import time
import client
import urllib2
import json
import dateutil.parser

def get_current_talk(talks):
  for talk in talks:
    parsed_date = dateutil.parser.parse(talk['date']).replace(tzinfo=None)
    if(now < parsed_date):
      saal1_talk = talk
      return saal1_talk
      break

greeting = "Stratum 0 Mensadisplay, next talks:"
tick = 0

j = urllib2.urlopen('https://events.ccc.de/congress/2014/Fahrplan/schedule.json')
schedule = json.load(j)
tick = 0
now = datetime.datetime.now()
i = 0 
saal1_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 1'])
saal2_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 2'])
saalG_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal G'])
saal6_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 6'])


while True:
    t = datetime.datetime.now().strftime(" %H:%M:%S")
    if tick > 10:
      j = urllib2.urlopen('https://events.ccc.de/congress/2014/Fahrplan/schedule.json')
      schedule = json.load(j)
      tick = 0
      now = datetime.datetime.now()
      i = 0 
      saal1_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 1'])
      saal2_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 2'])
      saalG_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal G'])
      saal6_talk = get_current_talk(schedule['schedule']['conference']['days'][0]['rooms']['Saal 6'])

    client.write(0,0,greeting)
    client.write(client.NUM_SEG_X - len(t), 0, t);
    client.write(0,1,"Fahrplan Version:" + schedule['schedule']['version'])
    client.write(0,2,"Saal 1 => " + saal1_talk['start'] + " : " + saal1_talk['title'])
    client.write(0,3,"Saal 2 => " + saal2_talk['start'] + " : " + saal2_talk['title'])
    client.write(0,4,"Saal G => " + saalG_talk['start'] + " : " + saalG_talk['title'])
    client.write(0,5,"Saal 6 => " + saal6_talk['start'] + " : " + saal6_talk['title'])
    client.write(0,7,"==> Get your brand new Easterhegg 2015 T-Shirt here! <==")

    time.sleep(0.5)
    tick += 1


    def load_schedule():
      tick = 0
