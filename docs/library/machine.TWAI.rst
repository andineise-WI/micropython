.. currentmodule:: machine
.. _machine.TWAI:

class TWAI -- Two-Wire Automotive Interface (CAN bus)
====================================================

TWAI (Two-Wire Automotive Interface) implements the CAN 2.0 protocol on ESP32 microcontrollers.
It provides a high-level interface for Controller Area Network (CAN) communication, commonly
used in automotive and industrial applications.

The TWAI peripheral supports standard CAN 2.0A (11-bit identifiers) and extended CAN 2.0B 
(29-bit identifiers) message formats, with automatic message filtering and error handling.

Example usage::

    from machine import TWAI
    
    # Initialize TWAI with 500 kbit/s baudrate
    can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
    can.init()
    
    # Send a CAN message
    can.send(b"Hello", 0x123)
    
    # Receive a CAN message
    data, msg_id, extframe, rtr = can.recv(timeout=1000)
    
    can.deinit()

Constructors
------------

.. class:: TWAI(tx, rx, baudrate=125000, mode=TWAI.NORMAL)

   Construct a TWAI object with the given parameters:

   - *tx* is the GPIO pin number for the transmit line (CTX)
   - *rx* is the GPIO pin number for the receive line (CRX)  
   - *baudrate* is the bus speed in bits per second. Supported rates:
     25000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000
   - *mode* is the operating mode (see constants below)

Methods
-------

.. method:: TWAI.init()

   Initialize and start the TWAI peripheral. Must be called before sending
   or receiving messages.

.. method:: TWAI.deinit()

   Deinitialize the TWAI peripheral and free resources.

.. method:: TWAI.send(data, id, timeout=1000, extframe=False, rtr=False)

   Send a CAN message with the specified parameters:

   - *data* is the message payload as bytes (max 8 bytes for CAN 2.0)
   - *id* is the message identifier (11-bit for standard, 29-bit for extended)
   - *timeout* is the maximum time to wait in milliseconds
   - *extframe* if True, use extended 29-bit identifier format
   - *rtr* if True, send as Remote Transmission Request (no data)

   Raises OSError on timeout or bus error.

.. method:: TWAI.recv(timeout=1000)

   Receive a CAN message from the bus.

   - *timeout* is the maximum time to wait in milliseconds

   Returns a tuple: ``(data, id, extframe, rtr)`` where:
   
   - *data* is the message payload as bytes (None for RTR messages)
   - *id* is the message identifier
   - *extframe* is True if extended frame format was used
   - *rtr* is True if this was a Remote Transmission Request

   Raises OSError on timeout or receive error.

.. method:: TWAI.any()

   Return the number of messages waiting in the receive queue.

.. method:: TWAI.setfilter(mode=TWAI.FILTER_SINGLE, mask=0x7FF, id1=0, id2=0)

   Configure message acceptance filtering. Must be called before init().

   - *mode* is the filter type (FILTER_SINGLE or FILTER_DUAL)
   - *mask* is the bit mask for filtering (0 = don't care, 1 = must match)
   - *id1* is the first acceptance ID
   - *id2* is the second acceptance ID (for dual filter mode)

   In single filter mode, only messages matching ``(msg_id & mask) == (id1 & mask)`` are accepted.
   In dual filter mode, both id1 and id2 are checked separately.

.. method:: TWAI.state()

   Return the current bus state:
   
   - ``-1`` TWAI stopped
   - ``0`` Running normally  
   - ``1`` Bus-off state (too many errors)
   - ``2`` Error state

.. method:: TWAI.info()

   Return detailed status information as a tuple:
   ``(tx_pending, rx_pending, tx_errors, rx_errors, arb_lost, bus_errors)``

.. method:: TWAI.restart()

   Restart the TWAI controller after a bus-off condition.

Constants
---------

Operating modes:

.. data:: TWAI.NORMAL

   Normal operation mode - acknowledge received messages

.. data:: TWAI.NO_ACK  

   No acknowledge mode - don't send ACK for received messages
   (useful for monitoring/testing)

.. data:: TWAI.LISTEN_ONLY

   Listen-only mode - receive messages but never transmit
   (bus monitoring mode)

Filter modes:

.. data:: TWAI.FILTER_SINGLE

   Single filter mode - use one acceptance filter

.. data:: TWAI.FILTER_DUAL

   Dual filter mode - use two independent acceptance filters

Hardware Notes
--------------

The ESP32 TWAI peripheral requires external CAN transceivers to interface with
a physical CAN bus. Common transceivers include:

- MCP2551/MCP2561 (5V tolerant)
- SN65HVD230/SN65HVD231 (3.3V)
- TJA1050/TJA1051

Typical connections::

    ESP32 GPIO21 (TX) -----> CTX (transceiver)
    ESP32 GPIO22 (RX) <----- CRX (transceiver)
    CAN_H <----------------> CAN bus H line  
    CAN_L <----------------> CAN bus L line

The CAN bus requires 120Ω termination resistors at both ends of the bus.

For development and testing, you can connect two ESP32 boards directly
without transceivers by connecting their TX/RX pins cross-wise, but this
only works for short distances and low speeds.

Error Handling
--------------

The TWAI module raises OSError exceptions for various error conditions:

- ``ETIMEDOUT`` - Operation timed out
- ``EINVAL`` - Invalid state or parameters  
- ``EPERM`` - Operation not permitted in current state
- ``EIO`` - General I/O error

Bus errors are automatically handled by the hardware, including:

- Bit errors
- Stuff errors  
- CRC errors
- Form errors
- ACK errors

The controller will automatically attempt error recovery and may enter
bus-off state if error counts become too high.
