"""
ESP32 CAN Test Script for MicroPython Integration
Based on the original esp32_can.py example
"""

from esp32 import CAN
import time


def send_and_check(can_bus, name, id, expected_result=True, extended=False):
    """Helper function to send a message and check if it was received"""
    can_bus.clear_tx_queue()
    can_bus.clear_rx_queue()
    can_bus.send([], id, extframe=extended)
    time.sleep_ms(100)
    if can_bus.any() == expected_result:
        print(f"{name}: OK")
        if expected_result:
            can_bus.recv()
    else:
        print(f"{name}: FAILED")


def test_esp32_can():
    """Main test function for ESP32 CAN integration"""
    print("Starting ESP32 CAN Integration Test...")
    
    try:
        # Initialize CAN with loopback mode for testing
        # GPIO 4 and 5 must be connected to each other for hardware loopback
        print("Initializing CAN interface...")
        dev = CAN(0, tx=5, rx=4, mode=CAN.SILENT_LOOPBACK, baudrate=50000)
        print("CAN interface initialized successfully")
        
        # Test send/receive message without filters
        print("\nLoopback Test: no filter - Standard frames")
        send_and_check(dev, "No filter", 0x100, True)

        # Set filter1 and test
        print("\nLoopback Test: one filter - Standard frames")
        dev.setfilter(0, CAN.FILTER_ADDRESS, [0x101, 0])
        send_and_check(dev, "Passing Message", 0x101, True)
        send_and_check(dev, "Blocked Message", 0x100, False)

        # Set filter2 and test
        print("\nLoopback Test: second filter - Standard frames")
        dev.setfilter(0, CAN.FILTER_ADDRESS, [0x102, 0])
        send_and_check(dev, "Passing Message - Bank 1", 0x102, True)
        send_and_check(dev, "Passing Message - Bank 0", 0x101, True)
        send_and_check(dev, "Blocked Message", 0x100, False)

        # Remove filter and test
        print("\nLoopback Test: clear filter - Standard frames")
        dev.clearfilter()
        send_and_check(dev, "Passing Message - Bank 1", 0x102, True)
        send_and_check(dev, "Passing Message - Bank 0", 0x101, True)
        send_and_check(dev, "Passing any Message", 0x100, True)

        # Extended message tests
        print("\nLoopback Test: no filter - Extended frames")
        send_and_check(dev, "No filter (Extended)", 0x100, True, extended=True)

        # Set filter1 for extended frames
        print("\nLoopback Test: one filter - Extended frames")
        dev.setfilter(0, CAN.FILTER_ADDRESS, [0x101], extframe=True)
        send_and_check(dev, "Passing Message (Extended)", 0x101, True, extended=True)
        send_and_check(dev, "Blocked Message (Extended)", 0x100, False, extended=True)

        # Remove filter for extended frames
        print("\nLoopback Test: clear filter - Extended frames")
        dev.clearfilter()
        send_and_check(dev, "Passing Message - Bank 0 (Extended)", 0x101, True, extended=True)
        send_and_check(dev, "Passing any Message (Extended)", 0x100, True, extended=True)

        # Test CAN state and info
        print(f"\nCAN State: {dev.state()}")
        info = dev.info()
        print(f"CAN Info: TX_ERR={info[0]}, RX_ERR={info[1]}, WARNS={info[2]}, PASSIVE={info[3]}, BUS_OFF={info[4]}, TX_PENDING={info[5]}, RX_PENDING={info[6]}")

        # Test with actual data payload
        print("\nTesting data transmission...")
        dev.clearfilter()
        test_data = [0x12, 0x34, 0x56, 0x78, 0xAB, 0xCD, 0xEF, 0x00]
        dev.send(test_data, 0x123)
        time.sleep_ms(50)
        
        if dev.any():
            received = dev.recv()
            print(f"Sent: {test_data}")
            print(f"Received ID: 0x{received[0]:X}, Extended: {received[1]}, RTR: {received[2]}, Data: {list(received[3])}")
            
            if list(received[3]) == test_data:
                print("Data transmission: OK")
            else:
                print("Data transmission: FAILED - Data mismatch")
        else:
            print("Data transmission: FAILED - No message received")

        print("\nDeinitializing CAN interface...")
        dev.deinit()
        print("ESP32 CAN Integration Test completed successfully!")
        
        return True
        
    except Exception as e:
        print(f"ESP32 CAN Integration Test failed: {e}")
        import sys
        sys.print_exception(e)
        return False


if __name__ == "__main__":
    test_esp32_can()