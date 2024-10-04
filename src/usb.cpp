#include "usb.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/error.hpp>

#include <tusb_config.h>
#include <tinyusb/src/tusb.h>

//#include "usb_device.h"
std::uint32_t data = 0;

stmcpp::gpio::pin<stmcpp::gpio::port::porte, 15> led3(stmcpp::gpio::mode::output);


// Here is some magic that should allow us to read / write ulpi directly
#define USBULPI_PHYCR     ((uint32_t)(0x40040000 + 0x034))
#define USBULPI_D07       ((uint32_t)0x000000FF)
#define USBULPI_New       ((uint32_t)0x02000000)
#define USBULPI_RW        ((uint32_t)0x00400000) 
#define USBULPI_S_BUSY    ((uint32_t)0x04000000) 
#define USBULPI_S_DONE    ((uint32_t)0x08000000)

#define USB_OTG_READ_REG32(reg)  (*(__IO uint32_t *)(reg))
#define USB_OTG_WRITE_REG32(reg,value) (*(__IO uint32_t *)(reg) = (value))

/**
  * @brief  Read CR value
  * @param  Addr the Address of the ULPI Register
  * @retval Returns value of PHY CR register
  */
extern "C" uint32_t USB_ULPI_Read(uint32_t Addr)
{ 
   __IO uint32_t val = 0;
   __IO uint32_t timeout = 1000; /* Can be tuned based on the Clock or etc... */
   
   USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | (Addr << 16));
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
   { 
       val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   } 
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);  
   return  (val & 0x000000ff);
}

/**
  * @brief  Write CR value
  * @param  Addr the Address of the ULPI Register
  * @param  Data Data to write
  * @retval Returns value of PHY CR register
  */
extern "C" uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data)   /* Parameter is the Address of the ULPI Register & Date to write */
{  
  __IO uint32_t val;
  __IO uint32_t timeout = 1000;   /* Can be tuned based on the Clock or etc... */
  
  USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | USBULPI_RW | (Addr << 16) | (Data & 0x000000ff));
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
  {
           val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  }
  
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  return 0; 
}

namespace usb {
    using namespace stmcpp::units;

    // Control interface pins
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 7> refsel0 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portb, 2> refsel1 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 5> refsel2 (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);

    // We need to generate 12MHz on this pin
    stmcpp::gpio::pin<stmcpp::gpio::port::porte, 9> refclk (stmcpp::gpio::mode::af1, stmcpp::gpio::otype::pushPull, stmcpp::gpio::speed::medium);


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
    stmcpp::gpio::pin<stmcpp::gpio::port::portc, 3>  ulpi_nxt (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 5>  ulpi_clk (stmcpp::gpio::mode::af10, stmcpp::gpio::speed::high);

    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 2>  ulpi_rst (stmcpp::gpio::mode::output, stmcpp::gpio::otype::pushPull);

    void init() {

        // Make sure that the analog switches for PC2 and PC3 are closed
        stmcpp::reg::clear(std::ref(SYSCFG->PMCR), SYSCFG_PMCR_PC2SO); 
        stmcpp::reg::clear(std::ref(SYSCFG->PMCR), SYSCFG_PMCR_PC3SO); 

        ulpi_rst.clear();

        // Set up the clock input to be 12MHz
        refsel0.clear();
        refsel1.set();
        refsel2.clear();
        
        // Set up the timer frequency
        stmcpp::reg::write(std::ref(TIM1->PSC), 0);   
        stmcpp::reg::write(std::ref(TIM1->ARR), 19);
        stmcpp::reg::write(std::ref(TIM1->CCR1), 10);

        // Set OC mode and enable preload
        stmcpp::reg::set(std::ref(TIM1->CCMR1), TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE);
        // Enable positive output
        stmcpp::reg::set(std::ref(TIM1->CCER), TIM_CCER_CC1E);
        // Load all the registers
        stmcpp::reg::set(std::ref(TIM1->EGR), TIM_EGR_UG);
        //Enable the timer and its output
        stmcpp::reg::set(std::ref(TIM1->CR1), TIM_CR1_CEN);
        stmcpp::reg::set(std::ref(TIM1->BDTR), TIM_BDTR_MOE);

        stmcpp::clock::systick::waitBlocking(20_ms);

        ulpi_rst.set();

        // Enable USB peripheral clock 
        stmcpp::reg::set(std::ref(RCC->AHB1ENR), RCC_AHB1ENR_USB1OTGHSEN | RCC_AHB1ENR_USB1OTGHSULPIEN);

        stmcpp::clock::systick::waitBlocking(100_ms);
    }

    void rst(){
        ulpi_rst.clear();

        stmcpp::clock::systick::waitBlocking(20_ms);

        ulpi_rst.set();

        stmcpp::clock::systick::waitBlocking(100_ms);

    }

    void coreInit(){
        stmcpp::reg::set(std::ref(PWR->CR3), PWR_CR3_USB33DEN);
        // Perform the core init according to the datasheet
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

        // Force device mode 
        //stmcpp::reg::set(std::ref(USB1_OTG_HS->GUSBCFG), USB_OTG_GUSBCFG_FDMOD/* | USB_OTG_GUSBCFG_ULPIIPD*/ | USB_OTG_GUSBCFG_ULPICSM | USB_OTG_GUSBCFG_ULPIAR);
        // Reset the core to clean interrupt flags
        while (!(USB1_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL)) {}
        USB1_OTG_HS->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
        while ((USB1_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_CSRST) == USB_OTG_GRSTCTL_CSRST) {}

    }

   

}


extern "C" void OTG_FS_IRQHandler(void) {
  tud_int_handler(0);
}

extern "C" void OTG_HS_IRQHandler(void) {
  tud_int_handler(1);
}

// Invoked when device is mounted
extern "C" void tud_mount_cb(void) {
  led3.set();
}

// Invoked when device is unmounted
extern "C" void tud_umount_cb(void) {
  led3.clear();
}


