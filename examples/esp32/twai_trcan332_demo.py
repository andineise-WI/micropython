"""
ESP32 TWAI Example for TCAN332 Transceiver (Texas Instruments)
Simple CAN communication using GPIO 4 (TX) and GPIO 5 (RX)

Hardware Setup:
ESP32        TCAN332 (Texas Instruments SO-8)
------       --------
GPIO4   ->   TXD (pin 1)
GPIO5   ->   RXD (pin 4)  
3.3V    ->   VCC (pin 3)
GND     ->   GND (pin 2)
             CAN_H (pin 7) -> CAN Bus High
             CAN_L (pin 6) -> CAN Bus Low

Note: 120Ω termination resistor required at each end of CAN bus
TCAN332 Datasheet: https://www.ti.com/product/TCAN332
"""

from machine import TWAI
import time

def tcan332_demo():
    """Simple demo for TCAN332 setup"""
    print("ESP32 TWAI Demo with TCAN332 Transceiver (Texas Instruments)")
    print("=" * 60)
    print("Hardware: GPIO4 (TX) -> TCAN332 TXD")
    print("          GPIO5 (RX) -> TCAN332 RXD")
    print("=" * 60)
    
    # Initialize TWAI with TCAN332 pin configuration
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)  # 500 kbit/s, normal mode
    
    try:
        # Initialize the TWAI interface
        twai.init()
        print("✅ TWAI initialized successfully")
        print(f"   Baudrate: 500000 bps")
        print(f"   TX Pin: GPIO4 -> TCAN332 TXD")
        print(f"   RX Pin: GPIO5 -> TCAN332 RXD")
        
        # Set up message reception callback
        received_messages = []
        
        def on_can_message(status):
            if status == 0:  # New message available
                while twai.any():
                    try:
                        data, msg_id, extended, rtr = twai.recv(timeout=0)
                        received_messages.append((msg_id, data, extended, rtr))
                        
                        if rtr:
                            print(f"📨 RTR Request: ID=0x{msg_id:03X}")
                        else:
                            print(f"📨 Data Frame: ID=0x{msg_id:03X}, Data={data.hex().upper()}")
                    except OSError:
                        break
        
        twai.rxcallback(on_can_message)
        print("✅ RX callback configured")
        
        # Send test messages
        print("\n🚀 Sending test messages...")
        
        test_messages = [
            (0x123, b'\x01\x02\x03\x04', "Sensor Data 1"),
            (0x456, b'\xAA\xBB\xCC\xDD', "Control Command"),
            (0x789, b'\x11\x22\x33\x44\x55\x66\x77\x88', "Full Frame"),
            (0x100, b'\xFF\x00\xFF\x00', "Status Update"),
        ]
        
        for i, (msg_id, data, description) in enumerate(test_messages):
            try:
                twai.send(data, id=msg_id, timeout=1000)
                print(f"📤 Sent #{i+1}: {description}")
                print(f"     ID=0x{msg_id:03X}, Data={data.hex().upper()}")
                
                # Wait a bit between messages
                time.sleep(0.5)
                
            except OSError as e:
                print(f"❌ Send failed #{i+1}: {e}")
        
        # Send an RTR frame (Remote Transmission Request)
        print("\n📡 Sending RTR frame...")
        try:
            twai.send(None, id=0x7DF, rtr=True, timeout=1000)  # OBD-II functional request
            print("📤 RTR sent: ID=0x7DF (OBD-II request)")
        except OSError as e:
            print(f"❌ RTR send failed: {e}")
        
        # Wait for responses
        print("\n⏳ Waiting for responses (5 seconds)...")
        time.sleep(5)
        
        # Show statistics
        print(f"\n📊 Session Statistics:")
        print(f"   Messages received: {len(received_messages)}")
        
        if received_messages:
            print("   Received message details:")
            for i, (msg_id, data, extended, rtr) in enumerate(received_messages[-5:]):  # Show last 5
                frame_type = "RTR" if rtr else "Data"
                ext_flag = "Ext" if extended else "Std"
                print(f"     #{i+1}: ID=0x{msg_id:03X} ({ext_flag}) {frame_type}: {data.hex().upper()}")
        
        # Driver statistics
        stats = twai.stats()
        print(f"\n📈 Driver Statistics:")
        for key, value in stats.items():
            print(f"   {key}: {value}")
        
        # Bus state
        state = twai.state()
        state_names = {0: "ERROR_ACTIVE", 1: "ERROR_WARNING", 2: "ERROR_PASSIVE", 3: "BUS_OFF"}
        print(f"   Bus state: {state_names.get(state, f'Unknown({state})')}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        
    finally:
        # Clean shutdown
        try:
            twai.rxcallback(None)  # Disable callback
            twai.deinit()
            print("\n✅ TWAI interface closed cleanly")
        except:
            pass


def tcan332_loopback_test():
    """Test TCAN332 in loopback-like mode"""
    print("\n🔄 TCAN332 Loopback Test")
    print("-" * 30)
    
    # Use NO_ACK mode for testing without other nodes
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=1)  # mode 1 = NO_ACK
    
    try:
        twai.init()
        print("✅ TCAN332 in NO_ACK mode initialized")
        
        received_count = 0
        
        def loopback_rx(status):
            nonlocal received_count
            if status == 0:
                while twai.any():
                    try:
                        data, msg_id, ext, rtr = twai.recv(timeout=0)
                        received_count += 1
                        print(f"🔄 Loop RX #{received_count}: ID=0x{msg_id:03X}, Data={data.hex().upper()}")
                    except OSError:
                        break
        
        twai.rxcallback(loopback_rx)
        
        # Send test data
        for i in range(3):
            test_data = bytes([0xCA, 0xFE, 0xBA, 0xBE + i])
            try:
                twai.send(test_data, id=0x555, timeout=1000)
                print(f"📤 Loop TX #{i+1}: ID=0x555, Data={test_data.hex().upper()}")
                time.sleep(0.3)
            except OSError as e:
                print(f"❌ Loop send #{i+1} failed: {e}")
        
        time.sleep(2)
        print(f"Loopback test result: {received_count} messages received")
        
    except Exception as e:
        print(f"❌ Loopback test error: {e}")
        
    finally:
        twai.deinit()


def tcan332_filter_test():
    """Test message filtering with TCAN332"""
    print("\n🔍 TCAN332 Filter Test")
    print("-" * 25)
    
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
    
    try:
        twai.init()
        
        # Set filter to accept only messages with ID 0x200-0x2FF
        twai.setfilter(mode=0, mask=0x700, id1=0x200)
        print("✅ Filter set: Accept IDs 0x200-0x2FF only")
        
        filtered_ids = []
        
        def filter_rx(status):
            if status == 0:
                while twai.any():
                    try:
                        data, msg_id, ext, rtr = twai.recv(timeout=0)
                        filtered_ids.append(msg_id)
                        print(f"✅ Filtered IN: ID=0x{msg_id:03X}")
                    except OSError:
                        break
        
        twai.rxcallback(filter_rx)
        
        # Send messages with various IDs
        test_ids = [0x100, 0x200, 0x250, 0x2FF, 0x300, 0x400]
        
        for msg_id in test_ids:
            test_data = bytes([0x01, 0x02, 0x03, 0x04])
            try:
                twai.send(test_data, id=msg_id, timeout=1000)
                print(f"📤 Sent: ID=0x{msg_id:03X}")
                time.sleep(0.2)
            except OSError as e:
                print(f"❌ Send failed ID=0x{msg_id:03X}: {e}")
        
        time.sleep(2)
        
        print(f"\nFilter Test Results:")
        print(f"  Sent IDs: {[hex(id) for id in test_ids]}")
        print(f"  Received IDs: {[hex(id) for id in filtered_ids]}")
        
        # Check if filter worked correctly
        expected_range = set(range(0x200, 0x300))
        received_set = set(filtered_ids)
        filter_working = received_set.issubset(expected_range)
        print(f"  Filter working correctly: {'✅ YES' if filter_working else '❌ NO'}")
        
    except Exception as e:
        print(f"❌ Filter test error: {e}")
        
    finally:
        twai.deinit()


if __name__ == "__main__":
    try:
        print("🚗 ESP32 + TCAN332 CAN Bus Demo")
        print("===============================")
        
        # Main demo
        tcan332_demo()
        
        # Additional tests
        tcan332_loopback_test()
        tcan332_filter_test()
        
        print("\n🎉 All TCAN332 tests completed!")
        
    except KeyboardInterrupt:
        print("\n⏹️  Demo interrupted by user")
    except Exception as e:
        print(f"❌ Main error: {e}")
    
    print("\n💡 Hardware Check:")
    print("   - Verify TCAN332 connections")
    print("   - Check CAN bus termination (120Ω)")
    print("   - Ensure proper power supply (3.3V/5V)")
    print("   - Connect to active CAN network for full test")
