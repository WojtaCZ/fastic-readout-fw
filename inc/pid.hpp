template<typename T>
class pid {
    private:
        T Kp, Ki, Kd;
        T integral, derivative, result;
        T error, errorOld;
    public:
        pid(T P, T I, T D) : Kp{P}, Ki{I}, Kd{D};
        T getOutput(T desiredValue, T actualValue) {
            error = desiredValue - actualValue;
            integral += error;
            derivative = error - errorOld;
            result = Kp*error + Ki*integral + Kd*derivative;
            errorOld = error;
            return result;
        }
}