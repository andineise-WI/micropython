.. _esp32_quickref_twai_enhanced:

Enhanced TWAI (CAN) bus
========================

The ESP32 TWAI (Two-Wire Automotive Interface) implementation provides a comprehensive
CAN 2.0 interface with advanced features inspired by professional implementations.

.. note::
   This enhanced version includes FreeRTOS task-based interrupt handling,
   comprehensive callback system, and extended queue management capabilities.

Constructing a TWAI object
--------------------------

.. class:: TWAI(tx, rx, baudrate, mode)

   Construct a TWAI object on the given pins. Example::

      from machine import TWAI
      
      # Standard initialization
      twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
      twai.init()

Advanced Features
-----------------

Interrupt Callbacks
~~~~~~~~~~~~~~~~~~~

Set up receive callbacks to handle incoming messages asynchronously::

   def rx_callback(status):
       if status == 0:
           print("First message received")
       elif status == 1:
           print("RX queue full")
       elif status == 2:
           print("RX queue overflow")
   
   twai.rxcallback(rx_callback)

Queue Management
~~~~~~~~~~~~~~~~

Advanced queue control for high-throughput applications::

   # Clear queues
   success = twai.clear_tx_queue()
   success = twai.clear_rx_queue()
   
   # Check queue status
   tx_pending, rx_pending, tx_err, rx_err, arb_lost, bus_err = twai.info()

Enhanced Statistics
~~~~~~~~~~~~~~~~~~~

Comprehensive statistics tracking::

   stats = twai.stats()
   print(f"Messages received: {stats['msg_rx_count']}")
   print(f"Messages transmitted: {stats['msg_tx_count']}")
   print(f"Bus off events: {stats['bus_off_count']}")
   print(f"Error warning count: {stats['error_warning_count']}")

Extended Baudrate Support
~~~~~~~~~~~~~~~~~~~~~~~~~~

Support for additional baudrates::

   # Standard rates
   twai = TWAI(tx=21, rx=22, baudrate=1000000)  # 1 Mbps
   twai = TWAI(tx=21, rx=22, baudrate=500000)   # 500 kbps
   twai = TWAI(tx=21, rx=22, baudrate=250000)   # 250 kbps
   twai = TWAI(tx=21, rx=22, baudrate=125000)   # 125 kbps
   
   # Extended rates (if supported by ESP-IDF version)
   twai = TWAI(tx=21, rx=22, baudrate=20000)    # 20 kbps
   twai = TWAI(tx=21, rx=22, baudrate=16000)    # 16 kbps
   twai = TWAI(tx=21, rx=22, baudrate=12500)    # 12.5 kbps
   twai = TWAI(tx=21, rx=22, baudrate=10000)    # 10 kbps
   twai = TWAI(tx=21, rx=22, baudrate=5000)     # 5 kbps
   twai = TWAI(tx=21, rx=22, baudrate=1000)     # 1 kbps

Bus Recovery
~~~~~~~~~~~~

Enhanced bus recovery with timeout handling::

   try:
       twai.restart()  # Automatic recovery with 5-second timeout
       print("Bus recovered successfully")
   except OSError as e:
       if e.errno == 110:  # ETIMEDOUT
           print("Bus recovery timed out")
       else:
           print(f"Recovery failed: {e}")

State Constants
~~~~~~~~~~~~~~~

Extended state information::

   state = twai.state()
   if state == TWAI.STATE_RUNNING:
       print("Bus is running normally")
   elif state == TWAI.STATE_BUS_OFF:
       print("Bus is in bus-off state")
   elif state == TWAI.STATE_RECOVERING:
       print("Bus is recovering")
   elif state == TWAI.STATE_STOPPED:
       print("Bus is stopped")

Complete Example
----------------

High-performance CAN node with callbacks::

   from machine import TWAI
   import time
   
   # Message counters
   rx_count = 0
   
   def on_message(status):
       global rx_count
       if status == 0:  # New message
           try:
               data, msg_id, ext, rtr = twai.recv(timeout=0)
               rx_count += 1
               print(f"RX {rx_count}: ID=0x{msg_id:X}, Data={data}")
           except OSError:
               pass  # No message available
   
   # Setup TWAI with callback
   twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
   twai.init()
   twai.rxcallback(on_message)
   
   # Send periodic messages
   msg_id = 0x123
   for i in range(100):
       data = bytes([i, i+1, i+2, i+3])
       try:
           twai.send(data, id=msg_id, timeout=1000)
           print(f"TX {i+1}: ID=0x{msg_id:X}, Data={data}")
       except OSError as e:
           print(f"Send failed: {e}")
       
       time.sleep_ms(100)
   
   # Show final statistics
   stats = twai.stats()
   print("\nFinal Statistics:")
   for key, value in stats.items():
       print(f"  {key}: {value}")
   
   twai.deinit()

Error Handling
--------------

The enhanced implementation provides detailed error information::

   try:
       twai.send(data, id=0x123, timeout=1000)
   except OSError as e:
       if e.errno == 110:  # ETIMEDOUT
           print("Send timeout")
       elif e.errno == 5:  # EIO
           print("Bus error")
       else:
           print(f"Unknown error: {e}")

Performance Notes
-----------------

- The FreeRTOS task handles interrupts efficiently without blocking the main thread
- Callbacks are scheduled using MicroPython's scheduler for thread safety
- Queue management functions provide real-time control over message flow
- Statistics are updated atomically to ensure consistency

Hardware Requirements
---------------------

- ESP32 with TWAI peripheral (all ESP32 variants)
- CAN transceiver (e.g., MCP2551, TJA1050)
- Proper termination resistors (120Ω at each end of the bus)
- Stable power supply for reliable operation

.. note::
   This enhanced implementation requires ESP-IDF v4.2 or later for full
   TWAI driver support. Some extended baudrates may not be available
   on older ESP-IDF versions.
