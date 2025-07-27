"""
Advanced ESP32 TWAI (CAN) Example - ESP-IDF v5.x+ Compatible
Enhanced version with callbacks, statistics, and queue management

This example demonstrates the TWAI implementation compatible with
ESP-IDF v5.x+ using the new esp_twai API architecture.

IMPORTANT: This requires ESP-IDF v5.0 or later!
"""

from machine import TWAI
import time
import _thread

class CANNode:
    def __init__(self, tx_pin=4, rx_pin=5, baudrate=500000, node_id=1):
        # Note: mode constants changed in new API
        # NORMAL=0, NO_ACK=1, LISTEN_ONLY=2
        self.twai = TWAI(tx=tx_pin, rx=rx_pin, baudrate=baudrate, mode=0)  # NORMAL mode
        self.node_id = node_id
        self.rx_count = 0
        self.tx_count = 0
        self.error_count = 0
        self.running = False
        
    def rx_callback(self, status):
        """Enhanced RX callback with comprehensive status handling"""
        if status == 0:  # New message available
            self._process_received_messages()
        elif status == 1:  # Queue full (not used in new API)
            print(f"[WARNING] RX queue full - processing backlog")
            self._process_received_messages()
        elif status == 2:  # Queue overflow
            print(f"[ERROR] RX queue overflow - messages lost!")
            self.error_count += 1
    
    def _process_received_messages(self):
        """Process all pending received messages"""
        while self.twai.any():
            try:
                data, msg_id, ext_frame, rtr = self.twai.recv(timeout=0)
                self.rx_count += 1
                
                if rtr:
                    print(f"RX {self.rx_count}: RTR frame ID=0x{msg_id:X}")
                    # Respond to RTR if it's for our node
                    if msg_id == 0x700 + self.node_id:
                        self._send_status_response()
                else:
                    print(f"RX {self.rx_count}: ID=0x{msg_id:X}, "
                          f"Ext={ext_frame}, Data={data.hex()}")
                    
                    # Handle specific message types
                    if msg_id == 0x100:  # Broadcast command
                        self._handle_broadcast_command(data)
                    elif msg_id == 0x200 + self.node_id:  # Node-specific command
                        self._handle_node_command(data)
                        
            except OSError:
                break  # No more messages
    
    def _handle_broadcast_command(self, data):
        """Handle broadcast commands"""
        if len(data) > 0:
            cmd = data[0]
            if cmd == 0x01:  # Reset command
                print(f"[INFO] Received reset command")
                self._send_ack(0x100)
            elif cmd == 0x02:  # Status request
                print(f"[INFO] Received status request")
                self._send_status_response()
    
    def _handle_node_command(self, data):
        """Handle node-specific commands"""
        if len(data) > 0:
            cmd = data[0]
            print(f"[INFO] Node command 0x{cmd:02X} received")
            self._send_ack(0x200 + self.node_id)
    
    def _send_ack(self, original_id):
        """Send acknowledgment message"""
        ack_data = bytes([0xFF, self.node_id])
        try:
            self.twai.send(ack_data, id=0x600 + self.node_id, timeout=1000)
            print(f"[INFO] ACK sent for ID 0x{original_id:X}")
        except OSError as e:
            print(f"[ERROR] Failed to send ACK: {e}")
    
    def _send_status_response(self):
        """Send node status response"""
        stats = self.twai.stats()
        status_data = bytes([
            self.node_id,
            (self.rx_count >> 8) & 0xFF,
            self.rx_count & 0xFF,
            (self.tx_count >> 8) & 0xFF,
            self.tx_count & 0xFF,
            self.error_count & 0xFF,
            stats['tx_error_counter'] & 0xFF,
            stats['rx_error_counter'] & 0xFF
        ])
        
        try:
            self.twai.send(status_data, id=0x700 + self.node_id, timeout=1000)
            print(f"[INFO] Status response sent")
        except OSError as e:
            print(f"[ERROR] Failed to send status: {e}")
    
    def start(self):
        """Start the CAN node"""
        print(f"Starting CAN node {self.node_id}...")
        
        try:
            self.twai.init()
            self.twai.rxcallback(self.rx_callback)
            self.running = True
            
            # Send startup message
            startup_data = bytes([0x01, self.node_id, 0x00, 0x00])
            self.twai.send(startup_data, id=0x500 + self.node_id, timeout=1000)
            self.tx_count += 1
            
            print(f"CAN node {self.node_id} started successfully")
            print(f"State: {self.twai.state()}")
            
        except Exception as e:
            print(f"Failed to start CAN node: {e}")
            raise
    
    def stop(self):
        """Stop the CAN node"""
        print(f"Stopping CAN node {self.node_id}...")
        self.running = False
        
        try:
            # Send shutdown message
            shutdown_data = bytes([0x02, self.node_id, 0x00, 0x00])
            self.twai.send(shutdown_data, id=0x500 + self.node_id, timeout=1000)
            self.tx_count += 1
        except:
            pass  # Ignore errors during shutdown
        
        self.twai.rxcallback(None)  # Disable callback
        self.twai.deinit()
        print(f"CAN node {self.node_id} stopped")
    
    def send_periodic_heartbeat(self, interval_ms=1000):
        """Send periodic heartbeat messages"""
        while self.running:
            try:
                # Heartbeat with timestamp
                timestamp = time.ticks_ms()
                heartbeat_data = bytes([
                    0x03,  # Heartbeat command
                    self.node_id,
                    (timestamp >> 16) & 0xFF,
                    (timestamp >> 8) & 0xFF,
                    timestamp & 0xFF,
                    (self.rx_count >> 8) & 0xFF,
                    self.rx_count & 0xFF,
                    self.error_count & 0xFF
                ])
                
                self.twai.send(heartbeat_data, id=0x400 + self.node_id, timeout=1000)
                self.tx_count += 1
                
                print(f"[HEARTBEAT] Node {self.node_id}: "
                      f"RX={self.rx_count}, TX={self.tx_count}, ERR={self.error_count}")
                
            except OSError as e:
                print(f"[ERROR] Heartbeat send failed: {e}")
                self.error_count += 1
                
                # Check if we need to restart the bus
                # Note: State constants changed in new API
                if self.twai.state() == TWAI.ERROR_BUS_OFF:
                    print("[WARNING] Bus off detected, attempting recovery...")
                    try:
                        self.twai.restart()
                        print("[INFO] Bus recovery successful")
                    except OSError:
                        print("[ERROR] Bus recovery failed")
            
            time.sleep_ms(interval_ms)
    
    def print_statistics(self):
        """Print comprehensive statistics"""
        stats = self.twai.stats()
        info = self.twai.info()
        
        print(f"\n=== CAN Node {self.node_id} Statistics ===")
        print(f"Application level:")
        print(f"  RX messages: {self.rx_count}")
        print(f"  TX messages: {self.tx_count}")
        print(f"  Errors: {self.error_count}")
        
        print(f"Driver level:")
        print(f"  TX pending: {info[0]}")
        print(f"  RX pending: {info[1]}")
        print(f"  TX error counter: {info[2]}")
        print(f"  RX error counter: {info[3]}")
        print(f"  Arbitration lost: {info[4]}")
        print(f"  Bus error count: {info[5]}")
        
        print(f"Extended statistics:")
        for key, value in stats.items():
            print(f"  {key}: {value}")
        
        print(f"Bus state: {self.twai.state()}")
        print("=" * 40)


def demo_enhanced_twai():
    """Demonstration of enhanced TWAI features - ESP-IDF v5.x+ compatible"""
    print("Enhanced ESP32 TWAI (CAN) Demo")
    print("Compatible with ESP-IDF v5.x+ using new esp_twai API")
    print("=" * 55)
    
    # Create CAN node with TRCan332 connections
    node = CANNode(tx_pin=4, rx_pin=5, baudrate=500000, node_id=1)
    
    try:
        # Start the node
        node.start()
        
        # Start heartbeat in a separate thread
        _thread.start_new_thread(node.send_periodic_heartbeat, (2000,))
        
        # Main loop - send test messages
        for i in range(20):
            # Send test data
            test_data = bytes([0x10 + i, i, (i * 2) & 0xFF, (i * 3) & 0xFF])
            
            try:
                node.twai.send(test_data, id=0x300 + i, timeout=1000)
                node.tx_count += 1
                print(f"[TEST] Sent message {i+1}: ID=0x{0x300 + i:X}, Data={test_data.hex()}")
                
            except OSError as e:
                print(f"[ERROR] Test message {i+1} failed: {e}")
                node.error_count += 1
            
            time.sleep(1)
        
        # Send RTR frame
        try:
            node.twai.send(None, id=0x700 + node.node_id, rtr=True, timeout=1000)
            print("[TEST] RTR frame sent")
        except OSError as e:
            print(f"[ERROR] RTR send failed: {e}")
        
        # Wait a bit more for responses
        time.sleep(5)
        
        # Print final statistics
        node.print_statistics()
        
    except KeyboardInterrupt:
        print("\n[INFO] Demo interrupted by user")
        
    except Exception as e:
        print(f"[ERROR] Demo failed: {e}")
        
    finally:
        # Clean shutdown
        node.stop()
        print("[INFO] Demo completed")


def test_new_api_features():
    """Test new ESP-IDF v5.x API specific features"""
    print("\nTesting New API Features")
    print("-" * 30)
    
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)  # mode 0 = NORMAL
    
    try:
        twai.init()
        
        # Test new state constants
        state = twai.state()
        if state == TWAI.ERROR_ACTIVE:
            print("✅ Bus is in ERROR_ACTIVE state (normal operation)")
        elif state == TWAI.ERROR_WARNING:
            print("⚠️  Bus is in ERROR_WARNING state")
        elif state == TWAI.ERROR_PASSIVE:
            print("⚠️  Bus is in ERROR_PASSIVE state")
        elif state == TWAI.ERROR_BUS_OFF:
            print("❌ Bus is in ERROR_BUS_OFF state")
        else:
            print(f"ℹ️  Bus state: {state}")
        
        # Test callback system
        rx_count = 0
        def test_callback(status):
            nonlocal rx_count
            rx_count += 1
            print(f"📨 RX callback triggered (#{rx_count}), status: {status}")
        
        twai.rxcallback(test_callback)
        print("✅ Callback system configured")
        
        # Test filter configuration (new API style)
        twai.setfilter(mode=0, mask=0x7F0, id1=0x100)  # Single filter
        print("✅ Single filter configured")
        
        # Test statistics (new format)
        stats = twai.stats()
        print(f"✅ Statistics retrieved: {len(stats)} items")
        
        # Send a test message
        test_data = b'\xAA\xBB\xCC\xDD'
        twai.send(test_data, id=0x123, timeout=1000)
        print("✅ Test message sent")
        
        time.sleep(1)
        
        # Check final stats
        final_stats = twai.stats()
        if final_stats['msg_tx_count'] > 0:
            print(f"✅ TX confirmed: {final_stats['msg_tx_count']} messages")
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        
    finally:
        twai.deinit()


if __name__ == "__main__":
    try:
        print("ESP32 TWAI Demo - ESP-IDF v5.x+ Compatible")
        print("=" * 45)
        
        # Test new API features first
        test_new_api_features()
        
        # Run the enhanced demo
        demo_enhanced_twai()
        
    except Exception as e:
        print(f"Main error: {e}")
    
    print("\nDemo finished!")
