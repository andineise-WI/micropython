.. currentmodule:: machine
.. _machine.TWAI:

class TWAI -- TWAI/CAN bus protocol
====================================

TWAI (Two-Wire Automotive Interface) is ESP32's implementation of the CAN (Controller Area Network) protocol.
It's commonly used in automotive and industrial applications for reliable communication between devices.

Constructors
------------

.. class:: TWAI(tx, rx, *, baudrate=125000, mode=TWAI.NORMAL)

   Construct a TWAI object on the given pins.

   The arguments are:

     - *tx* is the transmit pin.
     - *rx* is the receive pin.
     - *baudrate* is the bus speed in bits per second. Common values are 125000, 250000, 500000, 1000000.
     - *mode* is the TWAI operating mode.

Methods
-------

.. method:: TWAI.init(*, baudrate=125000, mode=TWAI.NORMAL)

   Initialize the TWAI peripheral with the given parameters:

     - *baudrate* sets the communication speed
     - *mode* sets the operating mode

.. method:: TWAI.deinit()

   Turn off the TWAI peripheral.

.. method:: TWAI.send(data, id, *, extframe=False, rtr=False, timeout=1000)

   Send a CAN message.

   - *data* is the message data as bytes or bytearray (max 8 bytes)
   - *id* is the CAN message identifier (11-bit for standard, 29-bit for extended)
   - *extframe* specifies if this is an extended frame (29-bit ID)
   - *rtr* specifies if this is a Remote Transmission Request
   - *timeout* is the timeout in milliseconds

.. method:: TWAI.recv(*, timeout=1000)

   Receive a CAN message. Returns a tuple (data, id, extframe, rtr).

   - *timeout* is the timeout in milliseconds

.. method:: TWAI.any()

   Returns the number of messages waiting in the receive buffer.

.. method:: TWAI.setfilter(*, mode=TWAI.FILTER_SINGLE, mask=0x7FF, id1=0, id2=0)

   Set the acceptance filter for incoming messages.

   - *mode* is the filter mode (SINGLE or DUAL)
   - *mask* is the acceptance mask
   - *id1* is the first acceptance ID
   - *id2* is the second acceptance ID (for dual filter mode)

.. method:: TWAI.state()

   Returns the current bus state as a string.

.. method:: TWAI.info()

   Returns detailed information about the TWAI peripheral status as a dictionary.

.. method:: TWAI.stats()

   Returns comprehensive statistics about message transmission and errors.

.. method:: TWAI.restart()

   Restart the TWAI peripheral (useful after bus-off recovery).

Constants
---------

.. data:: TWAI.NORMAL
          TWAI.NO_ACK 
          TWAI.LISTEN_ONLY

   TWAI operating modes:
   
   - ``NORMAL`` - Standard bidirectional communication
   - ``NO_ACK`` - Transmit without acknowledgment
   - ``LISTEN_ONLY`` - Receive-only mode (silent monitoring)

.. data:: TWAI.FILTER_SINGLE
          TWAI.FILTER_DUAL

   Filter modes for message acceptance.

Example Usage
-------------

Basic CAN communication::

    from machine import TWAI

    # Initialize TWAI on pins 21 (TX) and 22 (RX) at 500 kbps
    can = TWAI(21, 22, baudrate=500000)

    # Send a message
    can.send(b'hello', id=0x123)

    # Receive a message  
    if can.any():
        data, msg_id, ext_frame, rtr = can.recv()
        print(f"Received: {data} from ID: 0x{msg_id:x}")

Advanced filtering example::

    from machine import TWAI

    can = TWAI(21, 22, baudrate=250000)
    
    # Only accept messages with IDs 0x100-0x1FF
    can.setfilter(mask=0x700, id1=0x100)

    # Check statistics
    stats = can.stats()
    print(f"Messages sent: {stats['msg_tx_count']}")
    print(f"Messages received: {stats['msg_rx_count']}")

Extended frame example::

    from machine import TWAI

    can = TWAI(21, 22, baudrate=1000000)
    
    # Send extended frame (29-bit ID)
    can.send(b'extended', id=0x12345678, extframe=True)

    # Receive and check frame type
    if can.any():
        data, msg_id, ext_frame, rtr = can.recv()
        if ext_frame:
            print(f"Extended frame: ID=0x{msg_id:08x}")
        else:
            print(f"Standard frame: ID=0x{msg_id:03x}")

Bus monitoring example::

    from machine import TWAI
    import time

    # Listen-only mode for bus analysis
    can = TWAI(21, 22, baudrate=500000, mode=TWAI.LISTEN_ONLY)
    
    print("Monitoring CAN bus...")
    while True:
        if can.any():
            data, msg_id, ext_frame, rtr = can.recv(timeout=100)
            frame_type = "EXT" if ext_frame else "STD"
            rtr_flag = "RTR" if rtr else "DATA"
            print(f"[{frame_type}] ID:0x{msg_id:x} {rtr_flag} {data.hex()}")
        time.sleep_ms(10)

Error handling and recovery::

    from machine import TWAI
    import time

    can = TWAI(21, 22, baudrate=125000)

    while True:
        try:
            # Check bus state
            state = can.state()
            if state == "bus_off":
                print("Bus-off detected, restarting...")
                can.restart()
                time.sleep(1)
                continue
                
            # Normal operation
            can.send(b'test', id=0x123)
            
            # Get error statistics
            info = can.info()
            if info['tx_error_counter'] > 100:
                print(f"High TX error count: {info['tx_error_counter']}")
                
        except OSError as e:
            print(f"TWAI error: {e}")
            time.sleep(1)

Notes
-----

- TWAI uses GPIO pins for TX and RX communication
- Common baudrates: 125000, 250000, 500000, 1000000 bps
- Maximum data length is 8 bytes per message
- Extended frames support 29-bit identifiers instead of 11-bit
- Bus-off condition requires manual restart
- Listen-only mode doesn't affect bus communication
- Proper termination resistors (120Ω) are required on the CAN bus
