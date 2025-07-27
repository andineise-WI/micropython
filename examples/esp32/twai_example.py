"""
TWAI (Two-Wire Automotive Interface / CAN) Example for ESP32

This example shows how to use the TWAI interface on ESP32 to send and receive
CAN messages. TWAI is ESP32's implementation of the CAN 2.0 protocol.

Hardware setup:
- Connect a CAN transceiver (like MCP2551 or SN65HVD230) to the ESP32
- TX pin (default: GPIO 21) connects to CTX of transceiver  
- RX pin (default: GPIO 22) connects to CRX of transceiver
- Connect CAN_H and CAN_L to your CAN bus

For testing without a real CAN bus, you can use loopback mode or connect
two ESP32 boards together.
"""

from machine import TWAI
import time

def basic_can_example():
    """Basic CAN communication example."""
    
    # Initialize TWAI with 500 kbit/s baudrate
    can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
    
    # Initialize the CAN controller
    can.init()
    
    print("TWAI initialized. Ready for CAN communication.")
    print(f"Current state: {can.state()}")
    
    try:
        # Send a CAN message
        message_id = 0x123
        data = b"Hello CAN!"
        
        print(f"Sending: ID=0x{message_id:03X}, Data={data}")
        can.send(data, message_id, timeout=1000)
        print("Message sent successfully!")
        
        # Listen for incoming messages
        print("Listening for messages (press Ctrl+C to stop)...")
        
        while True:
            try:
                # Check if there are any pending messages
                if can.any() > 0:
                    # Receive message
                    data, msg_id, extframe, rtr = can.recv(timeout=1000)
                    
                    print(f"Received: ID=0x{msg_id:03X}")
                    print(f"  Data: {data}")
                    print(f"  Extended Frame: {extframe}")
                    print(f"  RTR: {rtr}")
                    print()
                
                time.sleep(0.1)
                
            except OSError as e:
                # Timeout or other error
                if "timeout" not in str(e).lower():
                    print(f"Error receiving: {e}")
                
    except KeyboardInterrupt:
        print("\nStopping...")
        
    finally:
        # Clean up
        can.deinit()
        print("TWAI deinitialized.")

def can_filter_example():
    """Example showing how to use CAN message filters."""
    
    can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.LISTEN_ONLY)
    
    # Set up filter to only accept messages with ID 0x100-0x1FF
    print("Setting up CAN filter...")
    can.setfilter(
        mode=TWAI.FILTER_SINGLE,
        mask=0x700,  # Mask for filtering
        id1=0x100    # Base ID to match
    )
    
    can.init()
    
    print("CAN filter configured. Only accepting IDs 0x100-0x1FF")
    print("Listening for filtered messages...")
    
    try:
        while True:
            if can.any() > 0:
                data, msg_id, extframe, rtr = can.recv(timeout=1000)
                print(f"Filtered message received: ID=0x{msg_id:03X}, Data={data}")
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\nStopping...")
        
    finally:
        can.deinit()

def can_diagnostic_example():
    """Example showing how to monitor CAN bus status."""
    
    can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.LISTEN_ONLY)
    can.init()
    
    print("CAN diagnostic monitor started...")
    
    try:
        while True:
            # Get detailed status information
            info = can.info()
            state = can.state()
            pending = can.any()
            
            print(f"State: {state}, Pending: {pending}")
            print(f"TX pending: {info[0]}, RX pending: {info[1]}")
            print(f"TX errors: {info[2]}, RX errors: {info[3]}")
            print(f"Arbitration lost: {info[4]}, Bus errors: {info[5]}")
            print("-" * 40)
            
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nStopping diagnostic monitor...")
        
    finally:
        can.deinit()

def can_no_ack_test():
    """Test CAN in NO_ACK mode (useful for testing without a bus)."""
    
    print("Testing CAN in NO_ACK mode...")
    
    can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NO_ACK)
    can.init()
    
    try:
        # Send test messages
        for i in range(5):
            message_id = 0x100 + i
            data = f"Test {i}".encode()
            
            print(f"Sending test message {i}: ID=0x{message_id:03X}")
            can.send(data, message_id, timeout=1000)
            time.sleep(0.5)
            
        print("All test messages sent successfully!")
        
    except Exception as e:
        print(f"Error in NO_ACK test: {e}")
        
    finally:
        can.deinit()

def main():
    """Main function to run examples."""
    
    print("=== ESP32 TWAI/CAN Examples ===")
    print()
    
    print("Available examples:")
    print("1. Basic CAN communication")
    print("2. CAN filter example")
    print("3. CAN diagnostic monitor")
    print("4. NO_ACK mode test")
    print()
    
    # For this example, we'll run the NO_ACK test as it doesn't require a real CAN bus
    print("Running NO_ACK mode test (safe without CAN bus)...")
    can_no_ack_test()
    
    print()
    print("To run other examples, uncomment the desired function call below.")
    
    # Uncomment these to run other examples:
    # basic_can_example()
    # can_filter_example() 
    # can_diagnostic_example()

if __name__ == "__main__":
    main()
