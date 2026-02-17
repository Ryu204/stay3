external shared abstract class Component;

class Bird : Component {
    protected void start() override {
        const Vec2i a(1, 2);
        print(format("{}, {} ", a.x, a.y));
        const Vec3f b(5.5);
        print(format("{}, {}, {}", b.x, b.y, b.z));
        const Vec3f c(1, 2, 3);
        print(format("{}, {}, {}", c.x, c.y, c.z));
    }

    protected void input() override {
        const Node newChild = this.node.addChild();
        print(format("New child id: {}, its parent: {}, our id: {}", newChild.id, newChild.parent.id, node.id));
    }
}
