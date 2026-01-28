abstract shared class Component {
    private Entity entity = EntityNull;
    private bool isValid = false;

    void onAttached(Entity entity) {
        this.entity = entity;
        this.isValid = true;
    }
    void onDetached() {
        this.entity = EntityNull;
        this.isValid = false;
    }
    void start() { 
        print("This is abstract start(), should not be called");
    }
    void update(float dt) { 
        print("This is abstract update(), should not be called");
    }
    void postUpdate() { 
        print("This is abstract postUpdate(), should not be called");
    }
    void input() { 
        print("This is abstract input(), should not be called");
    }
}
