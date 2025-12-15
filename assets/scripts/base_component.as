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
    void start() { }
    void update(float dt) { }
    void postUpdate() { }
}
