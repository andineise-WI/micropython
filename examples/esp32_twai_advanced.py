"""
ESP32 TWAI/CAN Example - Advanced Usage
======================================

This example demonstrates comprehensive usage of the ESP32 TWAI (CAN) implementation
including sending, receiving, filtering, error handling, and statistics monitoring.

Hardware Setup:
- Connect TX pin (default GPIO 21) to CAN transceiver TX
- Connect RX pin (default GPIO 22) to CAN transceiver RX  
- Ensure proper CAN bus termination (120Ω resistors)
- Use isolated CAN transceiver (e.g., MCP2551, TJA1050)
"""

from machine import TWAI
import time
import ubinascii

class CANMonitor:
    """Enhanced CAN bus monitor with statistics and error handling"""
    
    def __init__(self, tx_pin=21, rx_pin=22, baudrate=500000, mode=TWAI.NORMAL):
        self.twai = TWAI(tx_pin, rx_pin, baudrate=baudrate, mode=mode)
        self.message_count = 0
        self.error_count = 0
        self.start_time = time.ticks_ms()
        
    def send_message(self, data, msg_id, extended=False, rtr=False):
        """Send a CAN message with error handling"""
        try:
            self.twai.send(data, msg_id, extframe=extended, rtr=rtr)
            print(f"[TX] ID:0x{msg_id:x} {'EXT' if extended else 'STD'} {data.hex() if data else 'RTR'}")
            return True
        except OSError as e:
            print(f"[TX ERROR] {e}")
            self.error_count += 1
            return False
    
    def receive_message(self, timeout=1000):
        """Receive a CAN message with detailed parsing"""
        try:
            if self.twai.any():
                data, msg_id, ext_frame, rtr = self.twai.recv(timeout=timeout)
                frame_type = "EXT" if ext_frame else "STD"
                msg_type = "RTR" if rtr else "DATA"
                
                print(f"[RX] ID:0x{msg_id:x} {frame_type} {msg_type}", end="")
                if not rtr and data:
                    print(f" [{len(data)}] {data.hex()}")
                    # Try to decode as ASCII if possible
                    try:
                        ascii_data = data.decode('ascii')
                        if ascii_data.isprintable():
                            print(f"      ASCII: '{ascii_data}'")
                    except UnicodeDecodeError:
                        pass
                else:
                    print()
                
                self.message_count += 1
                return True
        except OSError as e:
            print(f"[RX ERROR] {e}")
            self.error_count += 1
        return False
    
    def show_statistics(self):
        """Display comprehensive statistics"""
        print("\n" + "="*50)
        print("TWAI/CAN Statistics")
        print("="*50)
        
        # Runtime information
        runtime = time.ticks_diff(time.ticks_ms(), self.start_time) / 1000
        print(f"Runtime: {runtime:.1f} seconds")
        
        # Basic info
        info = self.twai.info()
        print(f"State: {info.get('state', 'unknown')}")
        print(f"Baudrate: {info.get('baudrate', 'unknown')} bps")
        print(f"TX Pin: GPIO{info.get('tx_pin', '?')}")
        print(f"RX Pin: GPIO{info.get('rx_pin', '?')}")
        
        # Message statistics
        stats = self.twai.stats()
        print(f"\nMessage Statistics:")
        print(f"  Messages sent: {stats.get('msg_tx_count', 0)}")
        print(f"  Messages received: {stats.get('msg_rx_count', 0)}")
        print(f"  Monitor RX count: {self.message_count}")
        print(f"  Monitor errors: {self.error_count}")
        
        # Error counters
        print(f"\nError Counters:")
        print(f"  TX errors: {stats.get('tx_error_counter', 0)}")
        print(f"  RX errors: {stats.get('rx_error_counter', 0)}")
        print(f"  Bus errors: {stats.get('bus_error_count', 0)}")
        print(f"  Arbitration lost: {stats.get('arb_lost_count', 0)}")
        print(f"  Error warnings: {stats.get('error_warning_count', 0)}")
        print(f"  Error passive: {stats.get('error_passive_count', 0)}")
        print(f"  Bus-off events: {stats.get('bus_off_count', 0)}")
        
        # Queue status
        print(f"\nQueue Status:")
        print(f"  TX pending: {info.get('tx_pending', 0)}")
        print(f"  RX pending: {info.get('rx_pending', 0)}")
        
        print("="*50)
    
    def check_bus_health(self):
        """Check bus health and handle errors"""
        state = self.twai.state()
        if state == "bus_off":
            print("[WARNING] Bus-off condition detected!")
            print("Attempting recovery...")
            try:
                self.twai.restart()
                time.sleep_ms(100)
                print("Bus recovery successful")
                return True
            except OSError as e:
                print(f"Bus recovery failed: {e}")
                return False
        elif state in ["error_warning", "error_passive"]:
            print(f"[WARNING] Bus state: {state}")
        
        return True
    
    def setup_filters(self, filter_type="standard"):
        """Setup acceptance filters"""
        if filter_type == "standard":
            # Accept only standard automotive IDs (0x100-0x7FF)
            self.twai.setfilter(mask=0x700, id1=0x100, mode=TWAI.FILTER_SINGLE)
            print("Filter: Standard automotive IDs (0x100-0x7FF)")
        elif filter_type == "obd":
            # Accept OBD-II diagnostic IDs
            self.twai.setfilter(mask=0x7F8, id1=0x7E8, mode=TWAI.FILTER_SINGLE)
            print("Filter: OBD-II diagnostic responses (0x7E8-0x7EF)")
        elif filter_type == "dual":
            # Dual filter example
            self.twai.setfilter(mask=0x7FF, id1=0x123, id2=0x456, mode=TWAI.FILTER_DUAL)
            print("Filter: Dual filter for IDs 0x123 and 0x456")
        else:
            # Accept all messages
            self.twai.setfilter(mask=0x000, id1=0x000, mode=TWAI.FILTER_SINGLE)
            print("Filter: Accept all messages")

def demo_basic_communication():
    """Basic send/receive demonstration"""
    print("\n" + "="*30)
    print("Basic Communication Demo")
    print("="*30)
    
    monitor = CANMonitor(baudrate=500000)
    
    # Send test messages
    test_messages = [
        (b"Hello", 0x123, False),
        (b"World!", 0x456, False), 
        (b"Extended", 0x12345678, True),
        (b"", 0x789, False),  # RTR message
    ]
    
    for data, msg_id, extended in test_messages:
        is_rtr = len(data) == 0
        monitor.send_message(data, msg_id, extended, is_rtr)
        time.sleep_ms(100)
    
    # Receive messages for 5 seconds
    print("\nListening for messages (5 seconds)...")
    start = time.ticks_ms()
    while time.ticks_diff(time.ticks_ms(), start) < 5000:
        monitor.receive_message(timeout=100)
        time.sleep_ms(10)
    
    monitor.show_statistics()

def demo_obd_simulation():
    """Simulate OBD-II diagnostic communication"""
    print("\n" + "="*30)
    print("OBD-II Simulation Demo")
    print("="*30)
    
    monitor = CANMonitor(baudrate=500000)
    monitor.setup_filters("obd")
    
    # Simulate OBD-II requests
    obd_requests = [
        (b"\x02\x01\x0C\x00\x00\x00\x00\x00", 0x7DF),  # Engine RPM request
        (b"\x02\x01\x0D\x00\x00\x00\x00\x00", 0x7DF),  # Vehicle speed request
        (b"\x02\x01\x05\x00\x00\x00\x00\x00", 0x7DF),  # Coolant temp request
    ]
    
    print("Sending OBD-II requests...")
    for data, msg_id in obd_requests:
        monitor.send_message(data, msg_id, False, False)
        time.sleep_ms(50)
        
        # Check for responses
        for _ in range(10):  # Wait up to 100ms for response
            if monitor.receive_message(timeout=10):
                break
            time.sleep_ms(10)
    
    monitor.show_statistics()

def demo_bus_monitoring():
    """Passive bus monitoring demonstration"""
    print("\n" + "="*30)
    print("Bus Monitoring Demo")
    print("="*30)
    
    # Use listen-only mode for passive monitoring
    monitor = CANMonitor(mode=TWAI.LISTEN_ONLY, baudrate=500000)
    monitor.setup_filters("all")
    
    print("Monitoring bus traffic (listen-only mode)...")
    print("Press Ctrl+C to stop\n")
    
    try:
        last_stats = time.ticks_ms()
        while True:
            monitor.receive_message(timeout=100)
            
            # Show statistics every 10 seconds
            if time.ticks_diff(time.ticks_ms(), last_stats) > 10000:
                monitor.show_statistics()
                last_stats = time.ticks_ms()
            
            # Check bus health
            if not monitor.check_bus_health():
                break
                
            time.sleep_ms(10)
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")
        monitor.show_statistics()

def demo_stress_test():
    """High-rate message transmission stress test"""
    print("\n" + "="*30)
    print("Stress Test Demo")
    print("="*30)
    
    monitor = CANMonitor(baudrate=1000000)  # 1 Mbps for stress test
    
    print("Starting stress test (1000 messages at 1 Mbps)...")
    start_time = time.ticks_ms()
    
    for i in range(1000):
        # Send messages with incrementing data
        data = bytes([(i >> 8) & 0xFF, i & 0xFF, i % 8])
        msg_id = 0x100 + (i % 256)
        
        if not monitor.send_message(data, msg_id):
            print(f"Failed at message {i}")
            break
        
        # Receive any incoming messages
        monitor.receive_message(timeout=1)
        
        if i % 100 == 0:
            print(f"Progress: {i}/1000 messages")
            if not monitor.check_bus_health():
                break
    
    duration = time.ticks_diff(time.ticks_ms(), start_time)
    print(f"\nStress test completed in {duration}ms")
    monitor.show_statistics()

def main():
    """Main demonstration function"""
    print("ESP32 TWAI/CAN Advanced Example")
    print("================================")
    print("This example demonstrates comprehensive TWAI functionality")
    print("Make sure you have proper CAN bus hardware connected!\n")
    
    try:
        demo_basic_communication()
        time.sleep(2)
        
        demo_obd_simulation()
        time.sleep(2)
        
        # Uncomment for additional demos:
        # demo_stress_test()
        # demo_bus_monitoring()  # This runs indefinitely
        
    except Exception as e:
        print(f"Demo error: {e}")
    
    print("\nDemo completed!")

if __name__ == "__main__":
    main()
