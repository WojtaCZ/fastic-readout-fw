#include "usb.hpp"
#include "communication.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/error.hpp>

#include <tusb_config.h>
#include <tinyusb/src/tusb.h>
#include <tinyusb/src/device/usbd.h>


// This is required by the tinyusb library
uint32_t SystemCoreClock = 480000000;

namespace usb {
    using namespace stmcpp::units;
    
    void init() {
      // Init the interface and core
      usb::interface::init();
      usb::core::init();
      usb::interface::reset();

      // Check that we can correctly read the VID and PID 
      if(usb::interface::readRegister(0x00) != 0x24 || usb::interface::readRegister(0x01) != 0x04 || usb::interface::readRegister(0x02) != 0x07 || usb::interface::readRegister(0x03) != 0x00){
        errorHandler.hardThrow(error::interface_test_error);
      }

      // Try to write a test pattern to the scratch register and read it (if this is correct, we consider the interface as working)
      usb::interface::writeRegister(0x16, 0xAA);
      if(usb::interface::readRegister(0x16) != 0xAA){
        errorHandler.hardThrow(error::interface_test_error);
      }

      // Reset the core states
      //usb::core::hardReset();

      // Hand it over to tinyusb
      tusb_init();
    }

    namespace interface {
      // ULPI interface pins (high speed should be more than enough, no need for fast slew rates)
      stmcpp::gpio::pin<stmcpp::gpio::port::porta, 3>  ulpi_d0 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 0>  ulpi_d1 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 1>  ulpi_d2 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 10> ulpi_d3 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 11> ulpi_d4 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 12> ulpi_d5 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 13> ulpi_d6 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portb, 5>  ulpi_d7 (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);

      stmcpp::gpio::pin<stmcpp::gpio::port::portc, 0>  ulpi_stp (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::portc, 2>  ulpi_dir (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::porth, 4>  ulpi_nxt (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      stmcpp::gpio::pin<stmcpp::gpio::port::porta, 5>  ulpi_clk (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
      
      // Reset pin of the interface
      stmcpp::gpio::pin<stmcpp::gpio::port::portj, 3>  ulpi_rst (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);

      // Hidden ULPI registers that can be used to read/wrte the PHY
      #define USBULPI_PHYCR *(__IO uint32_t *)  (((uint32_t)(0x40040000 + 0x034)))
      #define USBULPI_NEW_Msk                   ((uint32_t)0x02000000)
      #define USBULPI_WRITE_Msk                 ((uint32_t)0x00400000) 
      #define USBULPI_S_BUSY_Msk                ((uint32_t)0x04000000) 
      #define USBULPI_S_DONE_Msk                ((uint32_t)0x08000000)

      // Initialize all the required interface related stuff
      void init() {
        // Set up the reference clock to 24MHz (all refsel pins high)
        ulpi_rst.clear();
        stmcpp::clock::systick::waitBlocking(1_ms);
        ulpi_rst.set();
        stmcpp::clock::systick::waitBlocking(30_ms);
      }

      // Reset the interface
      void reset() {
        ulpi_rst.clear();
        stmcpp::clock::systick::waitBlocking(1_ms);
        ulpi_rst.set();
        stmcpp::clock::systick::waitBlocking(30_ms);
      }

      // Read an ULPI register and return it's value
      std::uint8_t readRegister(std::uint8_t address) {
        stmcpp::reg::write(std::ref(USBULPI_PHYCR), USBULPI_NEW_Msk | (address << 16));
        stmcpp::reg::waitForBitSet(std::ref(USBULPI_PHYCR), USBULPI_S_DONE_Msk, []() { errorHandler.hardThrow(usb::error::ulpi_timeout); });
        return stmcpp::reg::read(std::ref(USBULPI_PHYCR), 0xff);
      }

      // Write an ULPI register
      void writeRegister(std::uint8_t address, std::uint8_t data) {
        stmcpp::reg::write(std::ref(USBULPI_PHYCR), USBULPI_NEW_Msk | USBULPI_WRITE_Msk | (address << 16) | data);
        stmcpp::reg::waitForBitSet(std::ref(USBULPI_PHYCR), USBULPI_S_DONE_Msk, []() { errorHandler.hardThrow(usb::error::ulpi_timeout); });
      }
    }

    namespace core{
      // Performs the USB core init according to the datasheet
      void init(){
        //Enable the USB voltage detector
        stmcpp::reg::set(std::ref(PWR->CR3), PWR_CR3_USB33DEN);
        // Unmask global interrupt and indicate that periodic fifo is empty
        stmcpp::reg::set(std::ref(USB1_OTG_HS->GAHBCFG), USB_OTG_GAHBCFG_GINT | USB_OTG_GAHBCFG_PTXFELVL);
        // Set RX FIFO not empty flag
        stmcpp::reg::set(std::ref(USB1_OTG_HS->GINTSTS), USB_OTG_GINTSTS_RXFLVL);
      
        // Configure the basics
        stmcpp::reg::write(std::ref(USB1_OTG_HS->GUSBCFG), (1 << USB_OTG_GUSBCFG_ULPIAR_Pos) |
                                                           (0 << USB_OTG_GUSBCFG_HNPCAP_Pos) |  // Not HNP capable
                                                           (0 << USB_OTG_GUSBCFG_SRPCAP_Pos) |  // Not SRP capable
                                                           (9 << USB_OTG_GUSBCFG_TRDT_Pos) |    // Turnaround time
                                                           (7 << USB_OTG_GUSBCFG_TOCAL_Pos));   // Timeout calibration

        // Unmask global and mode mismatch interrupt
        stmcpp::reg::set(std::ref(USB1_OTG_HS->GINTMSK), USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_MMISM);

        usb::core::softReset();
      }

      void softReset() {
        // Reset the core to clean interrupt flags
        stmcpp::reg::waitForBitSet(std::ref(USB1_OTG_HS->GRSTCTL), USB_OTG_GRSTCTL_AHBIDL_Msk, []() { errorHandler.hardThrow(usb::error::ahb_idle_timeout); });
        stmcpp::reg::set(std::ref(USB1_OTG_HS->GRSTCTL), USB_OTG_GRSTCTL_CSRST);
        stmcpp::reg::waitForBitClear(std::ref(USB1_OTG_HS->GRSTCTL), USB_OTG_GRSTCTL_CSRST_Msk, []() { errorHandler.hardThrow(usb::error::core_reset_timeout); });
      }

      void hardReset() {
        stmcpp::reg::set(std::ref(RCC->AHB1RSTR), RCC_AHB1RSTR_USB1OTGHSRST);
        
        // Reenable the clocks
        stmcpp::clock::enablePeripherals(
          stmcpp::clock::peripheral::usb1otg,
          stmcpp::clock::peripheral::usb1ulpi
        );
      }

    }
}


extern "C" void OTG_FS_IRQHandler(void) {
  tud_int_handler(0);
}

extern "C" void OTG_HS_IRQHandler(void) {
  tud_int_handler(1);
}

void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}


// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
  } else {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
  (void) itf;
}

/*
  D[7] Data Phase Transfer Direction
    0 = Host to Device
    1 = Device to Host
  D[5:6] Type
    0 = Standard
    1 = Class
    2 = Vendor
    3 = Reserved
  D[0:4] Recipient
    0 = Device
    1 = Interface
    2 = Endpoint
    3 = Other
    4..31 = Reserved
*/

char messageBuffer[512];
uint32_t messageLength;

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
  switch (request->bmRequestType_bit.type) {
    case TUSB_REQ_TYPE_VENDOR:

      if(request->bmRequestType_bit.direction == TUSB_DIR_IN){
        // If data is requested from the device
        switch (stage)
        {
          // If the setup is completed, we should provide some data back
          case CONTROL_STAGE_SETUP:
            communication::processBinaryCommand(static_cast<communication::command>(request->bRequest), communication::direction::GET, messageBuffer, &messageLength);
            return tud_control_xfer(rhport, request, (void*) (uintptr_t) messageBuffer, messageLength);
            break;
          
          // Nothing to do here
          case CONTROL_STAGE_DATA:
          case CONTROL_STAGE_ACK:
            return true;

          default:
            break;
        }
      } else if (request->bmRequestType_bit.direction == TUSB_DIR_OUT){
        // If data was sent to the device
        switch (stage)
        {
          // If the setup is completed, deal with what we've received
          case CONTROL_STAGE_SETUP:
            messageLength = request->wLength;
            return tud_control_xfer(rhport, request, &messageBuffer, request->wLength);
            break;

          // Nothing to do here
          case CONTROL_STAGE_DATA:
            return communication::processBinaryCommand(static_cast<communication::command>(request->bRequest), communication::direction::SET, messageBuffer, &messageLength);
            break;

          case CONTROL_STAGE_ACK:
            return true;
          
          default:
            break;
        }
      }
      
      
      // Class request in the vendor specific range (0x80 - 0xff)
      //__ASM volatile("bkpt");
      break;

    case TUSB_REQ_TYPE_CLASS:
      // Class request in the CDC range (0x20 - 0x2f)
      __ASM volatile("bkpt");
      break;

    default: break;
  }

  // stall unknown request
  return false;
}