#include <SFML/Window.hpp>
int main() {
	auto window = sf::Window{sf::VideoMode{800,600}, "Assteroids"};
	while(window.isOpen()) {
		sf::Event event;
		while(window.pollEvent(event)) {
			if(event.type == sf::Event::Closed)
				window.close();
		}
	}

	return 0;
}