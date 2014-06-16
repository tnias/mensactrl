#!/usr/bin/env python2

import zmq, sys

context = zmq.Context()

# Socket facing clients
frontend = context.socket(zmq.ROUTER)
frontend.bind("tcp://*:5571")

# Socket facing services
backend = context.socket(zmq.DEALER)
backend.connect(sys.argv[1])

zmq.device(zmq.QUEUE, frontend, backend)

# We never get here...
frontend.close()
backend.close()
context.term()

