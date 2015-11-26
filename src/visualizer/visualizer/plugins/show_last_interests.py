# import gobject
# import gtk

# import ns.core
# import ns.network
# import ns.visualizer

# from visualizer.base import InformationWindow
# from visualizer.higcontainer import HIGContainer
# from kiwi.ui.objectlist import ObjectList, Column

# class ShowLastInterests(InformationWindow):
#     class PacketList(gtk.ScrolledWindow):
#         (
#             COLUMN_TIME,
#             COLUMN_INTERFACE,
#             COLUMN_CONTENTS,
#             ) = range(3)

#         def __init__(self):
#             super(ShowLastInterests.PacketList, self).__init__()
#             self.set_properties(hscrollbar_policy=gtk.POLICY_AUTOMATIC,
#                                 vscrollbar_policy=gtk.POLICY_AUTOMATIC)
#             self.table_model = gtk.ListStore(*([str]*3))
#             treeview = gtk.TreeView(self.table_model)
#             treeview.show()
#             self.add(treeview)

#             def add_column(descr, colid):
#                 column = gtk.TreeViewColumn(descr, gtk.CellRendererText(), text=colid)
#                 treeview.append_column(column)

#             add_column("Time", self.COLUMN_TIME)
#             add_column("Interface", self.COLUMN_INTERFACE)
#             add_column("Contents", self.COLUMN_CONTENTS)

#         def update(self, node, packet_list):
#             self.table_model.clear()
#             for sample in packet_list:
#                 if sample.device is None:
#                     interface_name = "(unknown)"
#                 else:
#                     interface_name = ns.core.Names.FindName(sample.device)
#                     if not interface_name:
#                         interface_name = "%i" % sample.device.GetIfIndex()

#                 newpkt = sample.packet.Copy ()

#                 pppHeader = ns.point_to_point.PppHeader()
#                 newpkt.RemoveHeader (pppHeader)
#                 int    nano@nano-VirtualBox:~/ChronoChat$ ./waf configureSetting top to                       	: /home/nano/ChronoChat
#     Setting out to                       	: /home/nano/ChronoChat/build
#     Checking for 'gcc' (c compiler)      	: /usr/bin/gcc
#     Checking for 'g++' (c++ compiler)    	: /usr/bin/g++
#     Checking allowed flags for c++ compiler  :  
#     Checking for program pkg-config      	: /usr/bin/pkg-config
#     Checking for 'libsync'               	: yes
#     Checking for 'protobuf'              	: yes
#     Checking for program protoc          	: /usr/bin/protoc
#     Checking for program qmake-qt4       	: /usr/bin/qmake-qt4
#     Checking for program qmake4          	: not found
#     Checking for program qmake           	: /usr/bin/qmake
#     Checking for program uic-qt3         	: not found
#     Checking for program uic3            	: /usr/bin/uic3
#     Checking for program uic-qt4         	: /usr/bin/uic-qt4
#     Checking for uic version             	: version 4.8.1
#     Checking for program moc-qt4         	: /usr/bin/moc-qt4
#     Checking for program rcc             	: /usr/bin/rcc
#     Checking for program lrelease-qt4    	: /usr/bin/lrelease-qt4
#     Checking for program lupdate-qt4     	: /usr/bin/lupdate-qt4
#     Found the Qt4 libraries in           	: /usr/lib/i386-linux-gnu
#     Checking for pkg-config version >= '0.1' : yes
#     Checking for 'QtCore_debug'          	: not found
#     Checking for 'QtGui_debug'           	: not found
#     Checking for 'QtUiTools_debug'       	: not found
#     Checking for 'QtNetwork_debug'       	: not found
#     Checking for 'QtOpenGL_debug'        	: not found
#     Checking for 'QtSql_debug'           	: not found
#     Checking for 'QtSvg_debug'           	: not found
#     Checking for 'QtTest_debug'          	: not found
#     Checking for 'QtXml_debug'           	: not found
#     Checking for 'QtXmlPatterns_debug'   	: not found
#     Checking for 'QtWebKit_debug'        	: not found
#     Checking for 'Qt3Support_debug'      	: not found
#     Checking for 'QtHelp_debug'          	: not found
#     Checking for 'QtScript_debug'        	: not found
#     Checking for 'QtDeclarative_debug'   	: not found
#     Checking for 'QtDesigner_debug'      	: not found
#     Checking for 'QtCore'                	: yes
#     Checking for 'QtGui'                 	: yes
#     Checking for 'QtUiTools'             	: yes
#     Checking for 'QtNetwork'             	: yes
#     Checking for 'QtOpenGL'              	: yes
#     Checking for 'QtSql'                 	: yes
#     Checking for 'QtSvg'                 	: yes
#     Checking for 'QtTest'                	: yes
#     Checking for 'QtXml'                 	: yes
#     Checking for 'QtXmlPatterns'         	: yes
#     Checking for 'QtWebKit'              	: yes
#     Checking for 'Qt3Support'            	: yes
#     Checking for 'QtHelp'                	: yes
#     Checking for 'QtScript'              	: yes
#     Checking for 'QtDeclarative'         	: yes
#     Checking for 'QtDesigner'            	: yes
#     Checking boost includes              	: 1_48
#     Checking boost libs                  	: ok
#     Checking for boost linkage           	: Could not link against boost libraries using supplied options
#     The configuration failed
#     (complete log in /home/nano/ChronoChat/build/config.log)
    
#     trace of end of log:
    
#     Found boost lib libboost_random-mt.a
#     ok
#     ----------------------------------------
#     Checking for boost linkage
#     ==>
#     #include <boost/system/error_code.hpp>
#     int main() { boost::system::error_code c; }
#     <==
#     [1/2] [32mqxx: build/.conf_check_968fbcee789046a8c143bb4a6e302895/test.cpp -> build/.conf_check_968fbcee789046a8c143bb4a6e302895/testbuild/test.cpp.1.o
#     [0m
#     from /home/nano/ChronoChat: Test does not build: Traceback (most recent call last):
#       File "/home/nano/ChronoChat/.waf-1.7.11-ead76ce5682aeedc3f0cf095a369f9cf/waflib/Tools/c_config.py", line 
#     459, in run_c_code
#     	bld.compile()
#       File "/home/nano/ChronoChat/.waf-1.7.11-ead76ce5682aeedc3f0cf095a369f9cf/waflib/Build.py", line 188, in compile
#     	raise Errors.BuildError(self.producer.error)
#     BuildError: Build failed
#     Traceback (most recent call last):
#       File "/home/nano/ChronoChat/.waf-1.7.11-ead76ce5682aeedc3f0cf095a369f9cf/waflib/Task.py", line 103, in process
#     	ret=self.run()
#       File "<string>", line 12, in f
#     TypeError: 'NoneType' object is not iterable


#     from /home/nano/ChronoChat: The configuration failed
# erest = ns.ndnSIM.ndn.InterestHeader.GetInterest (newpkt)

#                 if (str(interest.GetName ()).split('/')[1] == "limit"):
#                     tree_iter = self.table_model.append()
#                     self.table_model.set(tree_iter,
#                                          self.COLUMN_TIME, str(sample.time.GetSeconds()),
#                                          self.COLUMN_INTERFACE, interface_name,
#                                          self.COLUMN_CONTENTS, str(interest.GetName ())
#                                          )


#     def __init__(self, visualizer, node_index):
#         InformationWindow.__init__(self)
#         self.win = gtk.Dialog(parent=visualizer.window,
#                               flags=gtk.DIALOG_DESTROY_WITH_PARENT|gtk.DIALOG_NO_SEPARATOR,
#                               buttons=(gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
#         self.win.connect("response", self._response_cb)
#         self.win.set_title("Last packets for node %i" % node_index) 
#         self.visualizer = visualizer
#         self.viz_node = visualizer.get_node(node_index)
#         self.node = ns.network.NodeList.GetNode(node_index)

#         def smart_expand(expander, vbox):
#             vbox.set_child_packing(expander, expand=True, fill=True, padding=0, pack_type=gtk.PACK_START)

#         self.tx_list = self.PacketList()
#         self.tx_list.show()
#         self.win.vbox.add (self.tx_list)

#         def update_capture_options():
#             self.visualizer.simulation.lock.acquire()
#             try:
#                 self.packet_capture_options = ns.visualizer.PyViz.PacketCaptureOptions ()
#                 self.packet_capture_options.mode = ns.visualizer.PyViz.PACKET_CAPTURE_FILTER_HEADERS_AND
#                 self.packet_capture_options.headers = [ns.ndnSIM.ndn.InterestHeader.GetTypeId ()]
#                 self.packet_capture_options.numLastPackets = 100

#                 self.visualizer.simulation.sim_helper.SetPacketCaptureOptions(
#                     self.node.GetId(), self.packet_capture_options)
#             finally:
#                 self.visualizer.simulation.lock.release()
        
#         # - options
#         update_capture_options ()

#         self.visualizer.add_information_window(self)
#         self.win.set_default_size(600, 300)
#         self.win.show()

#     def _response_cb(self, win, response):
#         self.win.destroy()
#         self.visualizer.remove_information_window(self)
    
#     def update(self):
#         last_packets = self.visualizer.simulation.sim_helper.GetLastPackets(self.node.GetId())
#         self.tx_list.update(self.node, last_packets.lastTransmittedPackets)


# def populate_node_menu(viz, node, menu):
#     menu_item = gtk.MenuItem("Show Last Interests")
#     menu_item.show()

#     def _show_it(dummy_menu_item):
#         ShowLastInterests(viz, node.node_index)

#     menu_item.connect("activate", _show_it)
#     menu.add(menu_item)

# def register(viz):
#     viz.connect("populate-node-menu", populate_node_menu)
