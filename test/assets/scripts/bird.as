external shared abstract class Component;

class Bird : Component {
    protected void start() override {
        print('Hello brother!');
    }

    protected void update(float lol) override {
        print('Hello from update in bird!');
    }
}
