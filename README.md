2CAN is both hardware and software

Hardware: Atmel AT90CAN32 + Microchip MCP2515 + FTDI FT232R 
Software: embedded (avr-gcc) + host (python/tornado) + UI (HTML5/websockets)

The goal is to build a flexible CAN analyzer which can be inserted in-line
in a working CAN environment for the purposes of reverse-engineering an
existing network.  To facilitate this, frames are captured going in both
directions, and individual frames can be filtered out to observe how the
system behavior is affected.

# Host Software Overview

The python script '2con.py' is both an interactive debug console and 
the HTTP server for the UI.

It requires Python 2.6.  I am using a native Windows python build and not
the one with Cygwin.

When run you should see something like this:

    C:\p4_2\2can>python 2con.py
    Found device 0 named 2CAN
    {'serial': 'AHUT0TGP', 'type': 5, 'id': 67330049, 'description': '2CAN'}
    Virtual COM port is COM3
    Opened serial port COM3
    

2con.py uses a python library for the FTDI USB to serial chip that is
windows specific (D2XX).  This is not strictly necessary; the main purpose
it serves is to automatically detect which emulated serial port the 2CAN
is connected to.  Above you can see it determined automatically that 
COM3 was the correct port by enumerating all FTDI chips and looking for
the one with the matching device description.

TODO: details on programming FTDI chip info

# Web Interface

2con.py starts a web server on port 8888.  Connect to http://localhost:8888 with a modern browser that supports websockets.
