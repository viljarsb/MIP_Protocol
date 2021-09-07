# MIP_Protocol

A part of my exam in IN3230, Networks at uio department of informatics. 

This repo contains a working network stack called MIP (Minimal Interconnection Protocol).

The 'MIP daemon' implements the network layer, and a instance of this program runs permanently in the background of each host/client in the network.



A functioning linux network protocol with support for 3rd party applications via linux application sockets. 
Contains functionality for arp-discovery, translation from mac-addr to mip-addr, creation and maintenence of routing-tables and of course packet-switching.

Tested on a virtual network hosted in mininet. 
