"""
Simple ESP32 TWAI (CAN) Example - ESP-IDF v5.x+ Compatible
Minimal example showing basic send/receive functionality

This is a simple example compatible with ESP-IDF v5.x+ that demonstrates
the basic TWAI functionality using the new esp_twai API.

IMPORTANT: Requires ESP-IDF v5.0 or later!

Wiring with TCAN332 transceiver (Texas Instruments):
- CAN TX: GPIO 4  -> TCAN332 TXD pin
- CAN RX: GPIO 5  -> TCAN332 RXD pin
- TCAN332 VCC: 3.3V or 5V
- TCAN332 GND: GND
- CAN_H/CAN_L: Connect to CAN bus with 120Ω termination
"""

from machine import TWAI
import time

def simple_twai_demo():
    """Simple demonstration of TWAI send/receive"""
    print("Simple ESP32 TWAI Demo - ESP-IDF v5.x+ Compatible")
    print("=" * 50)
    
    # Create TWAI instance with TCAN332 pins
    # Note: mode 0 = NORMAL in new API
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
    
    try:
        # Initialize TWAI
        twai.init()
        print("✅ TWAI initialized successfully")
        
        # Check initial state
        state = twai.state()
        print(f"Initial bus state: {state}")
        
        # Simple callback to show received messages
        def on_message_received(status):
            if status == 0:  # New message available
                while twai.any():
                    try:
                        data, msg_id, ext_frame, rtr = twai.recv(timeout=0)
                        if rtr:
                            print(f"📨 Received RTR: ID=0x{msg_id:X}")
                        else:
                            print(f"📨 Received: ID=0x{msg_id:X}, Data={data.hex()}")
                    except OSError:
                        break
        
        # Set up callback
        twai.rxcallback(on_message_received)
        print("✅ RX callback configured")
        
        # Send some test messages
        print("\nSending test messages...")
        
        for i in range(5):
            # Create test data
            test_data = bytes([0x01, i, i*2, i*3])
            
            try:
                # Send message (new API automatically handles node management)
                twai.send(test_data, id=0x123 + i, timeout=1000)
                print(f"📤 Sent message {i+1}: ID=0x{0x123+i:X}, Data={test_data.hex()}")
                
            except OSError as e:
                print(f"❌ Failed to send message {i+1}: {e}")
            
            time.sleep(1)
        
        # Send an RTR frame
        try:
            twai.send(None, id=0x200, rtr=True, timeout=1000)
            print("📤 Sent RTR frame: ID=0x200")
        except OSError as e:
            print(f"❌ Failed to send RTR: {e}")
        
        # Wait for any responses
        print("\nWaiting for responses...")
        time.sleep(3)
        
        # Show final statistics
        stats = twai.stats()
        print(f"\nStatistics:")
        print(f"  Messages transmitted: {stats.get('msg_tx_count', 0)}")
        print(f"  Messages received: {stats.get('msg_rx_count', 0)}")
        print(f"  TX errors: {stats.get('tx_error_counter', 0)}")
        print(f"  RX errors: {stats.get('rx_error_counter', 0)}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        
    finally:
        # Clean up
        twai.rxcallback(None)  # Disable callback
        twai.deinit()
        print("✅ TWAI deinitialized")


def loopback_test():
    """Test TWAI in loopback mode for self-testing"""
    print("\nLoopback Test - ESP-IDF v5.x+ Compatible")
    print("-" * 40)
    
    # Note: mode 2 = LISTEN_ONLY, mode 1 = NO_ACK (closest to loopback)
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=1)  # NO_ACK mode
    
    try:
        twai.init()
        print("✅ TWAI in NO_ACK mode initialized")
        
        received_count = 0
        
        def loopback_callback(status):
            nonlocal received_count
            if status == 0:
                while twai.any():
                    try:
                        data, msg_id, ext_frame, rtr = twai.recv(timeout=0)
                        received_count += 1
                        print(f"🔄 Loopback RX #{received_count}: ID=0x{msg_id:X}, Data={data.hex()}")
                    except OSError:
                        break
        
        twai.rxcallback(loopback_callback)
        
        # Send test messages
        for i in range(3):
            test_data = bytes([0xAA, 0xBB, 0xCC, i])
            try:
                twai.send(test_data, id=0x555, timeout=1000)
                print(f"📤 Sent loopback message {i+1}")
                time.sleep(0.5)
            except OSError as e:
                print(f"❌ Loopback send failed: {e}")
        
        time.sleep(2)
        print(f"Loopback test completed. Received: {received_count} messages")
        
    except Exception as e:
        print(f"❌ Loopback test error: {e}")
        
    finally:
        twai.deinit()


def filter_test():
    """Test message filtering with new API"""
    print("\nFilter Test - ESP-IDF v5.x+ Compatible")
    print("-" * 38)
    
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
    
    try:
        twai.init()
        
        # Configure filter to only accept IDs 0x100-0x10F
        # New API: setfilter(mode, mask, id1, [id2])
        twai.setfilter(mode=0, mask=0x7F0, id1=0x100)
        print("✅ Filter configured: Accept IDs 0x100-0x10F")
        
        received_ids = []
        
        def filter_callback(status):
            if status == 0:
                while twai.any():
                    try:
                        data, msg_id, ext_frame, rtr = twai.recv(timeout=0)
                        received_ids.append(msg_id)
                        print(f"✅ Filtered RX: ID=0x{msg_id:X}")
                    except OSError:
                        break
        
        twai.rxcallback(filter_callback)
        
        # Send messages with different IDs
        test_ids = [0x050, 0x100, 0x105, 0x10F, 0x110, 0x200]
        
        for msg_id in test_ids:
            try:
                test_data = bytes([0x01, 0x02, 0x03, 0x04])
                twai.send(test_data, id=msg_id, timeout=1000)
                print(f"📤 Sent: ID=0x{msg_id:X}")
                time.sleep(0.2)
            except OSError as e:
                print(f"❌ Send failed for ID 0x{msg_id:X}: {e}")
        
        time.sleep(2)
        
        print(f"Filter test results:")
        print(f"  Sent IDs: {[hex(id) for id in test_ids]}")
        print(f"  Received IDs: {[hex(id) for id in received_ids]}")
        print(f"  Filter working: {set(received_ids).issubset(set(range(0x100, 0x110)))}")
        
    except Exception as e:
        print(f"❌ Filter test error: {e}")
        
    finally:
        twai.deinit()


if __name__ == "__main__":
    try:
        # Basic functionality test
        simple_twai_demo()
        
        # Loopback test
        loopback_test()
        
        # Filter test
        filter_test()
        
        print("\n🎉 All tests completed!")
        
    except KeyboardInterrupt:
        print("\n⏹️  Tests interrupted by user")
    except Exception as e:
        print(f"❌ Main error: {e}")
