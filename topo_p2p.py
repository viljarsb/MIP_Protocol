""" Orakel IN3230/4230 - Point-to-Point topology
    host A <----> host B
    Usage: sudo mn --mac --custom topo_p2p.py --topo mytopo
"""

from mininet.topo import Topo

class MyTopo( Topo ):
    "Point to Point topology"

    def __init__( self ):
        "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts
        leftHost = self.addHost( 'h1' )
        rightHost = self.addHost( 'h2' )

        # Add link
        self.addLink( leftHost, rightHost )

topos = { 'mytopo': ( lambda: MyTopo() ) }
