typedef void (*OnBlink)(int);

class Turns {
  private:
    unsigned long turnLeftLo;
    unsigned long turnRightHi;
    unsigned long throttleLo;
    unsigned long throttleHi;
    unsigned long delay;

    unsigned long previousMillis = 0;

    OnBlink onTurn;
  public:
    Turns(unsigned long turnLeftLo, unsigned long turnRightHi, unsigned long throttleLo, unsigned long throttleHi, OnBlink onTurn, unsigned long delay = 3000):
      turnLeftLo(turnLeftLo),
      turnRightHi(turnRightHi),
      throttleHi(throttleHi),
      throttleLo(throttleLo),
      onTurn(onTurn),
      delay(delay)
      {}
    void evaluate(unsigned long currentMillis, unsigned long CH1, unsigned long CH2) {
      int turn = -1;

      if (this->previousMillis == 0) {
        this->previousMillis = currentMillis;
      }
      
      if ((CH2 < this->throttleHi) && (CH2 > this->throttleLo)) {
        if ((currentMillis - this->previousMillis) >= this->delay) {
          if (CH1 > this->turnLeftLo) {
            turn = 1;
          }
          if (CH1 < this->turnRightHi) {
            turn = 2;
          }
        }
      } else {
        this->previousMillis = currentMillis;
      }

      this->onTurn(turn);
    }
};
