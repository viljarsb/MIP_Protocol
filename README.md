# MIP_Protocol

A part of my exam in IN3230, Networks at uio department of informatics. 

This repo contains a working network stack called MIP (Minimal Interconnection Protocol).

The 'MIP daemon' implements the network layer, and a instance of this program runs permanently in the background of each host/client in the network.
The daemon communicates with user-applications above and needs to wrap the payload Service Data Unit (SDU) in a MIP datagram and to fill in the headerfields, 
creating a MIP-packet. The daemon also communicates with the ethernet-layer below, and must frame the MIP-packet in a ethernet-frame before passing it down. 
The program also has support for a arp-protocol named MIP-ARP. This means that the daemon is able to automaticly discover and map other hosts mac-addresses to
more user-friendly MIP-Addresses, allowing the user-applications to use these over physical ethernet-addresses. These translations are stored in a cache on each host, 
due to the impractical nature of doing these look ups for each outgoing transmission. The daemon is always listening for packets from above (outgoing transmission) 
and below (incoming transmissions), by using raw and unix-domain sockets. There is also a persistant socket for communication with the 'MIP-Routing-Daemon'.

The 'MIP-Routing-Daemon' also runs permanently on each host. It is responsible for the routing protocol and keeps track of the routing table and the
the forwarding engine. The main task of the program is to answer queries by the mip daemon for paths for outgoing transmissions. The routing daemon dynamicly discovers 
other nodes in the network and constructs routing-tables. This process is based on a Distance Vector Routing (DVR) scheme, with Poisoned Reverse loop protection.

The repo also contains a simple mininet topology and some test programs. 
The programs are tested on a virtual network hosted in mininet. 
