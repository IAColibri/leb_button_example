class Device {
  public:
    void set(const char* n, const char* i) {
      name = n;
      id = i;
    }

    char* publish_channel(){
      return channel("out");
    }

    char* subscribe_channel(){
      return channel("in");
    }

    const char* name;
    const char* id;

  private:
    char* channel(const char* topic){
      char channel[100] = "";
      snprintf(channel, 100, "%s/%s/%s", name, id, topic);
      return channel;
    }
};
