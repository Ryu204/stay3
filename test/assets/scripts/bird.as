external shared abstract class Component;

class Bird : Component {
    protected void input() override {
        const Node newChild = this.node.addChild();
        print(format("New child id: {}, its parent: {}, our id: {}", newChild.id, newChild.parent.id, node.id));
    }
}
