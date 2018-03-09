#!/usr/bin/python
# .config/argos
import urllib2
rime6 = urllib2.urlopen("https://api.thingspeak.com/channels/427675/feeds.json?api_key=10GYWVH2CGPEOTWF&results=1").read()

parteFinale = rime6.find("created_at")
parteFinale = rime6.find("created_at",parteFinale+1)

fine = rime6[parteFinale:]
data = fine[13:23]
ora = fine[24:32]

dp = fine.find("\":\"")
vv = fine.find("\",\"")

dp = fine.find("\":\"",dp+1)
vv = fine.find("\",\"",vv+1)
temp = fine[dp+3:vv]

dp = fine.find("\":\"",dp+1)
vv = fine.find("\",\"",vv+1)
humi = fine[dp+3:vv]

dp = fine.find("\":\"",dp+1)
vv = fine.find("\",\"",vv+1)
temB = fine[dp+3:vv]

dp = fine.find("\":\"",dp+1)
vv = fine.find("\",\"",vv+1)
pres = fine[dp+3:vv]

dp = fine.find("\":\"",dp+1)
vv = fine.find("\",\"",vv+1)
volt = fine[dp+3:vv]

print '%.1fC' % (float(temp))
print "---"
print "temp\thumi\tpres\tvolt\tora\t\tdata"
print '%.1fC\t%.1fRH\t%.0fmb\t%.1fV\t%s\t%s' % (float(temp), float(humi), float(pres), float(volt),ora,data)

URL="https://thingspeak.com/channels/427675/private_show"
#DIR=$(dirname "$0")

print "---"
print "Rime6 on ThingSpeak.com | iconName=applications-internet href='",URL,"'"



