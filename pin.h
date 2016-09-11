class Pin {
  public:
    Pin(int p) { pin = p; }

    bool was(int value) { return previous == value; }

    bool is(int value) { return state == value; }

    bool changed_to(int value) { return (!was(value) && is(value)); }

    bool is_changed(int new_state) {
      state = new_state;
      bool has_changed = changed_to(HIGH) || changed_to(LOW);
      previous = state;
      return has_changed;
    }

    int pin;
  private:
    int previous;
    int state;
};
