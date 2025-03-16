#include <iostream>
import stay3;

using namespace st;

int main() {
    app my_app{
        {
            .window = {
                .size = {600u, 400u},
                .name = "My cool window",
            },
            .updates_per_second = 20,
        }};
    my_app.enable_default_systems();
    my_app.run();
}
