"""
TWAI Configuration and Feature Check Script

This script verifies that TWAI is properly configured and available
in the MicroPython build for ESP32.
"""

import sys

def check_twai_support():
    """Check if TWAI is supported on this platform."""
    
    print("=== TWAI Support Check ===")
    
    # Check if we're on ESP32
    try:
        import esp32
        print("✓ Running on ESP32")
    except ImportError:
        print("✗ Not running on ESP32")
        return False
    
    # Check if machine module is available
    try:
        import machine
        print("✓ Machine module available")
    except ImportError:
        print("✗ Machine module not available")
        return False
    
    # Check if TWAI class is available
    try:
        twai_class = getattr(machine, 'TWAI', None)
        if twai_class is not None:
            print("✓ TWAI class available")
        else:
            print("✗ TWAI class not available")
            return False
    except AttributeError:
        print("✗ TWAI class not found in machine module")
        return False
    
    # Check TWAI constants
    try:
        constants = ['NORMAL', 'NO_ACK', 'LISTEN_ONLY', 'FILTER_SINGLE', 'FILTER_DUAL']
        for const in constants:
            if hasattr(machine.TWAI, const):
                value = getattr(machine.TWAI, const)
                print(f"✓ TWAI.{const} = {value}")
            else:
                print(f"✗ TWAI.{const} not available")
                return False
    except Exception as e:
        print(f"✗ Error checking constants: {e}")
        return False
    
    return True

def check_twai_methods():
    """Check if TWAI methods are available."""
    
    print("\n=== TWAI Methods Check ===")
    
    try:
        from machine import TWAI
        
        # List of expected methods
        methods = [
            'init', 'deinit', 'send', 'recv', 'any', 
            'setfilter', 'state', 'info', 'restart'
        ]
        
        # Create instance to check methods
        twai = TWAI(tx=21, rx=22, baudrate=500000)
        
        for method in methods:
            if hasattr(twai, method):
                print(f"✓ {method}() method available")
            else:
                print(f"✗ {method}() method not available")
                return False
        
        return True
        
    except Exception as e:
        print(f"✗ Error checking methods: {e}")
        return False

def test_twai_basic():
    """Test basic TWAI operations (without actual hardware)."""
    
    print("\n=== TWAI Basic Test ===")
    
    try:
        from machine import TWAI
        
        # Create TWAI instance
        print("Creating TWAI instance...")
        twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NO_ACK)
        print("✓ TWAI instance created")
        
        # Test setfilter (before init)
        print("Testing setfilter...")
        twai.setfilter(mode=TWAI.FILTER_SINGLE, mask=0x7FF, id1=0x123)
        print("✓ setfilter() works")
        
        # Test state before init
        print("Testing state before init...")
        state = twai.state()
        print(f"✓ state() = {state} (should be -1 for stopped)")
        
        # Initialize TWAI
        print("Initializing TWAI...")
        twai.init()
        print("✓ TWAI initialized")
        
        # Test state after init
        state = twai.state()
        print(f"✓ state() = {state} (should be 0 for running)")
        
        # Test info
        info = twai.info()
        print(f"✓ info() = {info}")
        
        # Test any
        pending = twai.any()
        print(f"✓ any() = {pending}")
        
        # Test deinit
        print("Deinitializing TWAI...")
        twai.deinit()
        print("✓ TWAI deinitialized")
        
        return True
        
    except Exception as e:
        print(f"✗ Basic test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def print_system_info():
    """Print system information."""
    
    print("\n=== System Information ===")
    
    try:
        import sys
        print(f"MicroPython version: {sys.version}")
    except:
        pass
    
    try:
        import esp32
        print(f"ESP32 flash size: {esp32.flash_size()} bytes")
    except:
        pass
    
    try:
        import machine
        print(f"Unique ID: {machine.unique_id().hex()}")
        print(f"Frequency: {machine.freq()} Hz")
    except:
        pass

def main():
    """Run all checks."""
    
    print("TWAI Configuration and Feature Check")
    print("=" * 40)
    
    # Print system info first
    print_system_info()
    
    # Check TWAI support
    if not check_twai_support():
        print("\n❌ TWAI support check failed!")
        print("Make sure MicroPython was built with TWAI support enabled.")
        return False
    
    # Check TWAI methods
    if not check_twai_methods():
        print("\n❌ TWAI methods check failed!")
        return False
    
    # Basic functionality test
    if not test_twai_basic():
        print("\n❌ TWAI basic test failed!")
        return False
    
    print("\n🎉 All TWAI checks passed successfully!")
    print("\nTWAI is properly configured and ready to use.")
    print("\nNext steps:")
    print("1. Connect CAN transceiver hardware")
    print("2. Run twai_example.py for usage examples")
    print("3. Use test_twai.py for comprehensive testing")
    
    return True

if __name__ == "__main__":
    success = main()
    if not success:
        sys.exit(1)
