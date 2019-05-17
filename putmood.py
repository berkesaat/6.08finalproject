#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Apr 24 13:07:40 2019

@author: alicezhang
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Feb 26 13:12:12 2019

@author: alicezhang
"""
import sqlite3
import datetime
mood_db = '__HOME__/mood.db'
usermood_db = '__HOME__/usermood.db'

def request_handler(request):
    sconn = sqlite3.connect(mood_db)  # connect to that database (will create if it doesn't already exist)
    uconn = sqlite3.connect(usermood_db)  # connect to that database (will create if it doesn't already exist)
    s = sconn.cursor()  # make cursor into database (allows us to execute commands)
    u = uconn.cursor()  # make cursor into database (allows us to execute commands)
    
    if request['method'] == 'GET':
        return None
       
        
    elif request['method'] == "POST":
        if request['form']['user'] == 'true':
            mymood = request['form']['mood']
            u.execute('''CREATE TABLE IF NOT EXISTS usermood_table (mood text, mydate date UNIQUE);''') # run a CREATE TABLE command
            u.execute('''INSERT or REPLACE into usermood_table VALUES (?,?);''', (mymood, datetime.datetime.now().date()))
        
            uconn.commit() # commit commands
            uconn.close() # close connection to database
            sconn.close()
            return None
    
            
        elif request['form']['user'] == 'false':
            stress = request['form']['stress']
            s.execute('''CREATE TABLE IF NOT EXISTS mood_table (stress int, mydate date);''') # run a CREATE TABLE command
            s.execute('''INSERT into mood_table VALUES (?,?);''', (stress, datetime.datetime.now().date()))
        
            sconn.commit() # commit commands
            sconn.close() # close connection to database
            uconn.close()
            return None
            
        
        
