#include "power.hpp"
#include "analog.hpp"
#include <stmcpp/register.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/dac.hpp>
#include <stmcpp/adc.hpp>
#include <stmcpp/dma.hpp>
#include <stmcpp/dmamux.hpp>
#include <array>



//using namespace stmcpp::units;

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

    bool is1V2Enabled() {
        return en_1V2.read();
    }
} 

namespace hv {
    stmcpp::gpio::pin<stmcpp::gpio::port::porth, 5> shutdown_n(stmcpp::gpio::mode::output);
    stmcpp::gpio::pin<stmcpp::gpio::port::porta, 4> control(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 11> isense(stmcpp::gpio::mode::analog);
    stmcpp::gpio::pin<stmcpp::gpio::port::portf, 12> usense(stmcpp::gpio::mode::analog);

    stmcpp::adc::adc<stmcpp::adc::peripheral::adc1> adc1 (stmcpp::adc::resolution::sixteenBit, stmcpp::adc::dataManegment::oneShotDMA, true, false, 256, 0, 8, true);

    static constexpr stmcpp::adc::channel hv_current(2, stmcpp::adc::channel::samplingTime::sixtyFourAndHalfClocks);
    static constexpr stmcpp::adc::channel hv_voltage(6, stmcpp::adc::channel::samplingTime::sixtyFourAndHalfClocks);

    static constexpr std::array adc1_sequence = {hv_current, hv_voltage};

    static constexpr auto adcMeasurementsSize = adc1_sequence.size();
    __attribute__((section(".dma_buffer"))) uint32_t adcMeasurements[adcMeasurementsSize];

    stmcpp::dac::dac<stmcpp::dac::channel::ch1> dac1_ch1;

    stmcpp::dmamux1::dmamux<stmcpp::dmamux1::channel::channel2> dmamux1ch2(stmcpp::dmamux1::request::adc1_dma);
    stmcpp::dma::dma<stmcpp::dma::peripheral::dma1, stmcpp::dma::stream::stream2> adc1_dma(stmcpp::dma::mode::periph2mem, stmcpp::dma::datasize::word, false, static_cast<uint32_t>(ADC1_BASE) + offsetof(ADC_TypeDef, DR), stmcpp::dma::datasize::word, true, (uint32_t)&adcMeasurements[0], 0, adcMeasurementsSize, stmcpp::dma::priority::low, true, stmcpp::dma::pincOffset::psize, false);

    float hvVoltage_;
    float hvCurrent_;

    static constexpr float hvVoltageMax_ = 75;
    static constexpr float hvVoltageMin_ = 0;

    // PID controller setup
    /*static constexpr*/ float P_ = 10;
    /*static constexpr*/ float I_ = 0.8;
    /*static constexpr*/ float D_ = 0;
    static float pidSetPoint_ = 0;
    static float measurementOld_ = 0;
    static float integral_ = 0;

    void init(){

        // Set up timer 12 used to trigger the ADCs to generate an event at 1000Hz
        stmcpp::reg::write(std::ref(TIM12->PSC), 240-1);   
        stmcpp::reg::write(std::ref(TIM12->ARR), 1000-1);

        // Calibrate the ADC
        adc1.calibrate(stmcpp::adc::calibration::singleEnded, true);

        // Set up the ADC sequences with the propper trigger - timer 15
        adc1.setupRegularSequence(adc1_sequence);

        // Set up the DMA
        adc1_dma.setNumberOfData(adc1_sequence.size());
        adc1_dma.enable();

        // Enable the ADC
        adc1.enable();
        
        // Enable timer update interrupt
        stmcpp::reg::set(std::ref(TIM12->DIER), TIM_DIER_UIE);
        NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);

        // Enable DMA interrupt
        adc1_dma.enableInterrupt(stmcpp::dma::interrupt::transferComplete);
        NVIC_EnableIRQ(DMA1_Stream2_IRQn);

        // Enable the DAC
        dac1_ch1.enable();
        dac1_ch1.setValue(0);

        // Keep the HV supply disabled
        shutdown_n.clear();

        // Start the timer to begin sampling
        stmcpp::reg::set(std::ref(TIM12->CR1), TIM_CR1_CEN);
        
    } 

    bool isEnabled() {
        return shutdown_n.read();
    }

    void enable() {
        shutdown_n.set();
    }

    void disable() {
        shutdown_n.clear();
    }

    float getVoltage() {
        return hvVoltage_;
    }

    float getCurrent() {
        return hvCurrent_;
    }

    void setPID(float P, float I, float D){
        P_ = P;
        I_ = I;
        D_ = D;
    }

    void getPID(float & P, float & I, float & D){
        P = P_;
        I = I_; 
        D = D_;
    }

    bool setVoltage(float value){
        // Check the bounds
        if (value < hvVoltageMin_ || value > hvVoltageMax_) {
            return false;
        }

        // Update the setpoint
        pidSetPoint_ = value;

        return true;
    }

    float pidProcess(float setpoint, float measurement){
        // Calculate the error from the setpoint
        float error = setpoint - measurement;
        // Integrate the error
        integral_ += error;
        // Calculate the derivative
        float derivative = measurement - measurementOld_;
        // Update the old measurement
        measurementOld_ = measurement;

        // Calculate the output
        uint32_t output = (float)(P_ * error + I_ * integral_ + D_ * derivative);

        // Limit the output to 12-bit (0 to 4095)
        if (output < 0) {
            output = 0;
        } else if (output > 4095) {
            output = 4095;
        }

        return output;
    }
}


extern "C" void DMA_STR2_IRQHandler(){
    // Get the float voltages
    hv::hvVoltage_ = hv::adcMeasurements[1] * analog::getVoltageMultiplier() * 51;
    hv::hvCurrent_ = hv::adcMeasurements[0] * analog::getVoltageMultiplier() * 5000;

    // Set the DAC based on the setpoint
    hv::dac1_ch1.setValue(hv::pidProcess(hv::pidSetPoint_, hv::hvVoltage_));

    hv::adc1_dma.clearInterruptFlag(stmcpp::dma::interrupt::transferComplete);
    NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
    
}


extern "C" void TIM8_BRK_TIM12_IRQHandler() {
    if (stmcpp::reg::read(std::ref(TIM12->SR), TIM_SR_UIF_Msk)) {
        // Clear the update interrupt flag
        stmcpp::reg::clear(std::ref(TIM12->SR), TIM_SR_UIF_Msk);

        // Start the ADC measurement
        hv::adc1.startRegular();
    }

    NVIC_ClearPendingIRQ(TIM8_BRK_TIM12_IRQn);
}