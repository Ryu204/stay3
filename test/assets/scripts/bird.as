external shared abstract class Component;

class Bird : Component {
    void start() override {
        print('Hello brother!');
    }

    void update(float lol) override {
        print('Hello from update in bird!');
    }
}
