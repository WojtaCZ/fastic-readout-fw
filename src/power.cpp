#include "power.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/dac.hpp>
#include <stmcpp/adc.hpp>

namespace power { 

    // 1V2 power control pins
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 1> pg_1V2D(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 2> pg_1V2T(stmcpp::gpio::mode::input);
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 2> pg_1V2A(stmcpp::gpio::mode::input);
    
    stmcpp::gpio::pin<stmcpp::gpio::port::portk, 5> en_1V2(stmcpp::gpio::mode::output);

    // 1V8 power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portd, 9> pg_1V8D(stmcpp::gpio::mode::input);

    // 3V3A power control pin
    stmcpp::gpio::pin<stmcpp::gpio::port::portg, 10> pg_3V3A(stmcpp::gpio::mode::input);


    bool isPowerGood(ldo regulator) {
        switch(regulator) {
            case ldo::D1V2:
                return pg_1V2D.read();
                break;
            case ldo::T1V2:
                return pg_1V2T.read();
                break;
            case ldo::A1V2:
                return pg_1V2A.read();
                break;
            case ldo::D1V8:
                return pg_1V8D.read();
                break;
            case ldo::A3V3:
                return pg_3V3A.read();
                break;

            default:
                return false;
                break;
        }
    }

    void enable1V2() { 
        en_1V2.set();
    }

    void disable1V2() {
        en_1V2.clear();
    }
} 

namespace hv {

    uint16_t measurements[2];
    uint8_t measurementIdx = 0;

    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 5> shutdown_n(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 4> control(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 11> isense(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 12> usense(stmcpp::gpio::mode::analog);

    stmcpp::adc::adc<stmcpp::adc::peripheral::adc1> adc1 (stmcpp::adc::resolution::sixteenBit);

    static constexpr stmcpp::adc::channel adc1_inp2(2, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);
    static constexpr stmcpp::adc::channel adc1_inp6(6, stmcpp::adc::channel::samplingTime::eightHundretTenAndHalfClocks);

    std::vector<stmcpp::adc::channel> adc1_sequence = {adc1_inp2, adc1_inp6};

    stmcpp::dac::dac<stmcpp::dac::channel::ch1> dac1_ch1;

    void init(){

        // Timer 15 is used as a trigger (generate a 100kHz clock) 
        stmcpp::reg::write(std::ref(TIM12->PSC), 24-1);   
        stmcpp::reg::write(std::ref(TIM12->ARR), 10000);
        // Load all the registers
        stmcpp::reg::set(std::ref(TIM12->EGR), TIM_EGR_UG);

        // Enable Timer 12 interrupt
        stmcpp::reg::set(std::ref(TIM12->DIER), TIM_DIER_UIE);
        //NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);

        //Set up the reference buffer
        stmcpp::reg::write(std::ref(VREFBUF->CSR), VREFBUF_CSR_ENVR | VREFBUF_CSR_VRS_OUT3);   
        // Set up the ADC
        stmcpp::reg::set(std::ref(ADC12_COMMON->CCR), 0b1001, ADC_CCR_PRESC_Pos);
        adc1.calibrate(stmcpp::adc::calibration::singleEnded);
        adc1.setupRegularSequence(adc1_sequence);
        adc1.enableInterrupt(stmcpp::adc::interrupt::endOfConversion);
        adc1.enableInterrupt(stmcpp::adc::interrupt::endOfSequence);
        adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
        adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfSequence);
        NVIC_EnableIRQ(ADC_IRQn);

        dac1_ch1.enable();
        adc1.enable();

        hv::measurementIdx = 0;
        hv::adc1.startRegular();

        //Enable the timer and its output
        //stmcpp::reg::set(std::ref(TIM12->CR1), TIM_CR1_CEN);
    } 
      
}

extern "C" void ADC1_2_IRQHandler(){
	

    if(stmcpp::reg::read(std::ref(ADC1->ISR), ADC_ISR_EOS)) {
        hv::measurementIdx = 0;
        
        hv::adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfSequence);
    }else if(stmcpp::reg::read(std::ref(ADC1->ISR), ADC_ISR_EOC)){
        stmcpp::reg::waitForBitSet(std::ref(ADC1->ISR), ADC_ISR_EOC);
        hv::measurements[hv::measurementIdx++] = ADC1->DR;
        hv::adc1.clearInterruptFlag(stmcpp::adc::interrupt::endOfConversion);
        hv::adc1.startRegular();
    }else{
        __ASM volatile("bkpt");
    }


    NVIC_ClearPendingIRQ(ADC_IRQn);
}



extern "C" void TIM8_BRK_TIM12_IRQHandler() {
    if (stmcpp::reg::read(std::ref(TIM12->SR), TIM_SR_UIF_Msk)) {
        // Clear the update interrupt flag
        stmcpp::reg::clear(std::ref(TIM12->SR), TIM_SR_UIF_Msk);
        hv::measurementIdx = 0;
        hv::adc1.startRegular();
    }

    NVIC_ClearPendingIRQ(TIM8_BRK_TIM12_IRQn);
}