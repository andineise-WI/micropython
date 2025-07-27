"""
Basic TWAI (Two-Wire Automotive Interface) test for ESP32.

This test demonstrates basic TWAI functionality including:
- Initialization
- Sending messages
- Receiving messages
- Filter configuration
- State monitoring

Note: For actual CAN bus communication, you need proper CAN transceivers
and a physical CAN bus setup.
"""

from machine import TWAI
import time


def test_basic_twai():
    """Test basic TWAI operations."""
    print("Testing TWAI basic operations...")
    
    # Create TWAI instance with default pins (TX=21, RX=22)
    twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
    
    # Initialize TWAI
    print("Initializing TWAI...")
    twai.init()
    
    # Check state
    state = twai.state()
    print(f"TWAI state: {state}")
    
    # Get info
    info = twai.info()
    print(f"TWAI info: {info}")
    
    # Check for pending messages
    pending = twai.any()
    print(f"Pending messages: {pending}")
    
    # Clean up
    twai.deinit()
    print("TWAI deinitialized.")


def test_twai_send_receive():
    """Test TWAI send and receive (requires loopback mode or proper setup)."""
    print("Testing TWAI send/receive...")
    
    # Use NO_ACK mode for testing without a proper bus
    twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NO_ACK)
    twai.init()
    
    try:
        # Send a test message
        test_data = b"Hello"
        test_id = 0x123
        
        print(f"Sending message: ID=0x{test_id:03X}, Data={test_data}")
        twai.send(test_data, test_id, timeout=1000)
        print("Message sent successfully!")
        
        # Try to receive (will timeout if no message available)
        try:
            data, msg_id, extframe, rtr = twai.recv(timeout=1000)
            print(f"Received: ID=0x{msg_id:03X}, Data={data}, ExtFrame={extframe}, RTR={rtr}")
        except OSError as e:
            print(f"No message received (timeout): {e}")
        
    except Exception as e:
        print(f"Error during send/receive: {e}")
    
    finally:
        twai.deinit()


def test_twai_filter():
    """Test TWAI filter configuration."""
    print("Testing TWAI filter configuration...")
    
    twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.LISTEN_ONLY)
    
    # Configure filter before initialization
    print("Setting up single filter for ID 0x123...")
    twai.setfilter(mode=TWAI.FILTER_SINGLE, mask=0x7FF, id1=0x123)
    
    twai.init()
    
    print("Filter configured. TWAI in listen-only mode.")
    
    twai.deinit()


def test_twai_constants():
    """Test TWAI constants."""
    print("Testing TWAI constants...")
    
    print(f"TWAI.NORMAL = {TWAI.NORMAL}")
    print(f"TWAI.NO_ACK = {TWAI.NO_ACK}")
    print(f"TWAI.LISTEN_ONLY = {TWAI.LISTEN_ONLY}")
    print(f"TWAI.FILTER_SINGLE = {TWAI.FILTER_SINGLE}")
    print(f"TWAI.FILTER_DUAL = {TWAI.FILTER_DUAL}")


def main():
    """Run all TWAI tests."""
    print("=== TWAI Test Suite ===")
    
    try:
        test_twai_constants()
        print()
        
        test_basic_twai()
        print()
        
        test_twai_filter()
        print()
        
        test_twai_send_receive()
        print()
        
        print("=== All tests completed ===")
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
