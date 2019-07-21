#!/usr/bin/env python3

import socket
import time
from astral import Astral
import RPi.GPIO as GPIO
import lcddriver
from datetime import datetime, date, timezone

def backlightController(lcd, now, sunrise, sunset):
    if now > sunrise and now < sunset:
        lcd.switch_backlight(False)
    else:
        lcd.switch_backlight(True)

def round_strNum(string):
    f = float(string)
    return str((round(f, 1)))

cityName = "hong kong"
a = Astral()
city = a[cityName]
currentDay = date.today()
listOfEvents = city.sun(currentDay)
day = currentDay.day
sunrise = listOfEvents['sunrise']
sunset = listOfEvents['sunset']

HOST = '192.168.1.20'  # The server's hostname or IP address
PORT = 888        # The port used by the server
recvStr = list() # temp, humidity, pressure, dryness, notRaining
if datetime.now(timezone.utc) > sunrise and datetime.now(timezone.utc) < sunset:
    onOff = False
else:
    onOff = True
lcd = lcddriver.lcd(onOff)
lcd.lcd_clear()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while 1:    

        dayForUpdate = date.today()
        if dayForUpdate.day > day:
            newDayEvents = city.sun(dayForUpdate)
            sunrise = newDayEvents['sunrise']
            sunset = newDayEvents['sunset']
            day = dayForUpdate.day
            
        # i have to write something to activate arduino side's connection.
        # only send length of 1 to avoid arduino to be provoked multiple times.
        s.send(b'\n') # activate keyword. if it is not \n, it crashes the Weather server.
        
        while 1:
            strOfpiece = s.recv(1204).decode()
            # print(strOfpiece)
            # recvStr.append(strOfpiece)
            if '\r' in strOfpiece or '\n' in strOfpiece:
                # print("detected break command")
               # recvStr.pop()
                break
            #  or '\r' in recvStr or '\n' in recvStr
            recvStr.append(strOfpiece)
        # time.sleep(1)
        
        lcd.lcd_clear()
        if len(recvStr) == 5:
            backlightController(lcd, datetime.now(timezone.utc), sunrise, sunset)
            lcd.lcd_display_string(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), 1)
            lcd.lcd_display_string("T: " + round_strNum(recvStr[0]) + "c | H: " + recvStr[1] + "%", 2)
            lcd.lcd_display_string("Pressure: " + round_strNum(recvStr[2]) + "hPa", 3)
            if recvStr[4] == '1':
                lcd.lcd_display_string("Dryness: " + recvStr[3] + " No rain", 4)
            else:
                lcd.lcd_display_string("Dryness: " + recvStr[3] + " raining", 4) 
        else:
            backlightController(lcd, datetime.now(timezone.utc), sunrise, sunset)
            lcd.lcd_display_string(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), 1)

        recvStr = []
        time.sleep(1)

