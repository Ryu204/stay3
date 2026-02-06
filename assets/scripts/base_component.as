abstract shared class Component {
    private Entity entity = EntityNull;
    private bool isValid = false;
    private TreeContext@ treeContext;

    private void preLifecycleSetup(TreeContext@ treeContext) {
        @this.treeContext = @treeContext;
    }
    
    private void onAttached(Entity entity) {
        this.entity = entity;
        this.isValid = true;
    }

    private void onDetached() {
        this.entity = EntityNull;
        this.isValid = false;
    }

    Node node {
        get {
            return this.treeContext.getNode(this.entity);
        }
    }

    protected void start() { 
        print("This is abstract start(), should not be called");
    }
    protected void update(float dt) { 
        print("This is abstract update(), should not be called");
    }
    protected void postUpdate() { 
        print("This is abstract postUpdate(), should not be called");
    }
    protected void input() { 
        print("This is abstract input(), should not be called");
    }
}
