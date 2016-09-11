class Button {
  public:
    Button(int p) {
      previous = HIGH;
      pin = p;
    }
    void setup() {
      pinMode(pin, INPUT_PULLUP);
    }
    boolean low() {
      int value = digitalRead(pin);
      boolean go_low = (previous == HIGH && value == LOW);
      if(previous != value) delay(50);
      previous = value;

      return go_low;
    }
  private:
    int pin;
    int previous;
};

