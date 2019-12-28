# try and use python to scrape DDE messages
# http://www.nlsa.com/docs/nfw_dde.txt



# found this handy stack overflow snippet, I am not first to go here :)
# https://stackoverflow.com/questions/30152460/create-dde-server-in-python-and-send-data-continuously
# but this is going the wrong direction. I want to be a client of this code.

import time
import win32ui, dde
#from pywin.mfc import object


# class DDETopic(object.Object):
#     def __init__(self, topicName):
#         self.topic = dde.CreateTopic(topicName)
#         object.Object.__init__(self, self.topic)
#         self.items = {}

#     def setData(self, itemName, value):
#         try:
#             self.items[itemName].SetData( str(value) )
#         except KeyError:
#             if itemName not in self.items:
#                 self.items[itemName] = dde.CreateStringItem(itemName)
#                 self.topic.AddItem( self.items[itemName] )
#                 self.items[itemName].SetData( str(value) )


# ddeServer = dde.CreateServer()
# ddeServer.Create('Orbitron')
# ddeTopic = DDETopic('Tracking')
# ddeServer.AddTopic(ddeTopic)

# while True:
#     yourData = time.ctime() + ' UP0 DN145000001 UMusb DMfm AZ040 EL005 SNNO SATELLITE'
#     ddeTopic.setData('Tracking', yourData)
#     win32ui.PumpWaitingMessages(0, -1)
#     time.sleep(0.1)

# https://stackoverflow.com/questions/28931475/get-data-via-dde-in-python-by-bypassing-excel#45449221

#apparently "servers" talk to "servers"
server = dde.CreateServer()
#servers get names but I'm not sure what use this name
#has if you're acting like a client
server.Create("TestClient")  
#use our server to start a conversation
conversation = dde.CreateConversation(server)

# RunAny is the server name, and RunAnyCommand is the topic
#conversation.ConnectTo("NFW32", "NFW_DATA")
conversation.ConnectTo("Orbitron", "Tracking")
# DoSomething is the command to execute
#conversation.Exec("NFW_SERVER")
# For my case I also needed the request function
# request somedata and save the response in requested_data.
requested_data = conversation.Request("TrackingData")
#requested_data = conversation.Request("NFW_SERVER")
#requested_data = conversation.Request("TRACK ON")
#requested_data = conversation.Poke("NFW_SERVER", "TRACK ON\x00")