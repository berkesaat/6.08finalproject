#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Apr 28 11:39:44 2019

@author: alicezhang
"""
import sqlite3
import datetime
mood_db = '__HOME__/mood.db'
usermood_db = '__HOME__/usermood.db'


html1 = '''
<!DOCTYPE html>
<html>
<body bgcolor="f5f4ff">

<h1 align="center">MY M.O.O.D. DATA</h1>

<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;margin:0px auto;}
.tg td{font-family:Arial, sans-serif;font-size:14px;padding:7px 20px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:black;}
.tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:7px 20px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:black;}
.tg .tg-sd2d{font-family:"Courier New", Courier, monospace !important;;background-color:#9698ed;color:#000000;border-color:inherit;text-align:center;vertical-align:top}
.tg .tg-u3sm{font-family:"Courier New", Courier, monospace !important;;background-color:#ecf4ff;color:#000000;border-color:inherit;text-align:center;vertical-align:top}
.tg .tg-a4si{font-family:"Courier New", Courier, monospace !important;;background-color:#6665cd;color:#ffffff;border-color:#000000;text-align:center;vertical-align:top}
.tg .tg-tto9{font-family:"Courier New", Courier, monospace !important;;background-color:#cbcefb;color:#000000;border-color:inherit;text-align:center;vertical-align:top}
.tg .tg-9qzh{font-family:"Courier New", Courier, monospace !important;;background-color:#cbcefb;border-color:inherit;text-align:center;vertical-align:top}
.tg .tg-fzse{font-family:"Courier New", Courier, monospace !important;;background-color:#ecf4ff;border-color:inherit;text-align:center;vertical-align:top}
</style>
<table class="tg">
  <tr>
    <th class="tg-a4si" rowspan="2">:)</th>
    <th class="tg-sd2d" colspan="7">DATES</th>
  </tr>
'''
  
html2 = '''
</table>
</body>
</html>
'''

def request_handler(request):
    
    if request['method'] == 'GET':
        sconn = sqlite3.connect(mood_db)  # connect to that database (will create if it doesn't already exist)
        uconn = sqlite3.connect(usermood_db)  # connect to that database (will create if it doesn't already exist)
        s = sconn.cursor()  # make cursor into database (allows us to execute commands)
        u = uconn.cursor()  # make cursor into database (allows us to execute commands)
        
        today = datetime.datetime.now()
        datelist =[]
        for i in range(7):
            day = today - datetime.timedelta(days = i)
            day = day.date()
            datelist += [day,]
        
        datelist = datelist[::-1]
        stresslist = []
        moodlist = []
        for date in datelist: 
            mood = u.execute('''SELECT * FROM usermood_table WHERE mydate = ?;''',(date,)).fetchall() 
            stress = s.execute('''SELECT stress FROM mood_table WHERE mydate = ?;''',(date,)).fetchall()

            total = len(stress)
            if total>0:
                count = 0
                for x in stress:
                    count += x[0]
                stresslist += [str(count/(total*100)*100)[0:4],]
            else: 
                stresslist += ["No Data",]
    
            if len(mood)>0: 
                moodlist += [mood[0][0],]
            else: 
                moodlist += ["No Data",]
             
        sconn.commit() # commit commands
        uconn.commit()
        sconn.close() # close connection to database
        uconn.close() 

        tablehtml = ""
        tablelist = [datelist,stresslist,moodlist]
        for row in tablelist:
            tablehtml += "<tr>\n"
            if row == stresslist:
                tablehtml += "<td class=\"tg-u3sm\">STRESS</td>"
            if row == moodlist:
                tablehtml += "<td class=\"tg-u3sm\">MOOD</td>"
            for cell in row:
                tablehtml += "<td class=\"tg-u3sm\">"
                if row == datelist:
                    tablehtml += cell.strftime('%b %d')
                else:
                    tablehtml += str(cell)
                tablehtml += "</td>\n"
            tablehtml += "</tr>\n"
        
        return html1+tablehtml+html2
    
    
    