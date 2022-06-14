#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <fmt/core.h>
#include <functional>
#include <random>
#include <vector>

struct Entity
{
	sf::Vector2f pos;
	sf::Vector2f dir;
	float rot;
	float speed;
	float radius;
	sf::Color color;
	void draw(sf::RenderTarget& rt)
	{
		auto circle = sf::CircleShape{ radius };
		circle.setFillColor(color);
		circle.setOrigin(radius / 2, radius / 2);
		circle.setPosition(pos);
		rt.draw(circle);
	}
};

struct Player : public Entity
{
	bool shoot;
	void update()
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) rot -= 0.1;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) rot += 0.1;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			dir = sf::Vector2f{ cos(rot), sin(rot) };
			speed += 0.05;
		} else
			speed *= 0.99;
		shoot = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
		speed = std::clamp(speed, 0.f, 5.f);

		pos += dir * speed;
	}
};

struct Bullet : public Entity
{
	bool to_be_destroyed;
	void update()
	{
		auto direction = sf::Vector2f{ cos(rot), sin(rot) };
		pos += direction * speed;
	}
};

struct Asteroid : public Entity
{
	uint8_t size;
	bool to_be_destroyed;
	void update()
	{
		auto direction = sf::Vector2f{ cos(rot), sin(rot) };
		pos += direction * speed;
	}
};

bool
is_colliding(const Entity& lhs, const Entity& rhs)
{
	auto x = rhs.pos.x - lhs.pos.x;
	auto y = rhs.pos.y - lhs.pos.y;
	auto distance = sqrt((x * x) + (y * y));
	return distance <= (lhs.radius + rhs.radius);
}

template<class T>
void
wrap(T& t, sf::Vector2f dim)
{
	t.pos.x += t.pos.x < 0 ? dim.x : 0;
	t.pos.x -= t.pos.x > dim.x ? dim.x : 0;
	t.pos.y += t.pos.y < 0 ? dim.y : 0;
	t.pos.y -= t.pos.y > dim.y ? dim.y : 0;
}
struct World
{
	sf::Vector2f dimensions;
	Player p;
	std::vector<Bullet> bullets;
	std::vector<Asteroid> asteroids;
	sf::Clock shoot_cooldown_clock{};

	Asteroid generate_asteroid(uint8_t size = 4,
							   sf::Vector2f position = { 0, 0 })
	{
		static std::random_device rd{};
		static std::mt19937 mt{ rd() };
		auto dist_x = std::uniform_real_distribution<float>(0.f, dimensions.x);
		auto dist_y = std::uniform_real_distribution<float>(0.f, dimensions.y);
		auto pos = position == sf::Vector2f{ 0, 0 }
					   ? sf::Vector2f{ dist_x(mt), dist_y(mt) }
					   : position;
		float radius;
		switch (size) {
			case 4:
				radius = 40;
				break;
			case 2:
				radius = 20;
				break;
			case 1:
				radius = 10;
				break;
		}
		return Asteroid{ { .pos = pos,
						   .rot = dist_x(mt),
						   .speed = 2,
						   .radius = radius,
						   .color = sf::Color::White },
						 size };
	}
	bool check_collisions()
	{
		return std::ranges::any_of(asteroids,
								   [p = std::ref(this->p)](const Asteroid& a) {
									   return is_colliding(p, a);
								   });
	}

	void check_bullet_asteroid()
	{
		std::vector<Asteroid> new_asteroids{};
		for (auto& b : bullets)
			for (auto& a : asteroids) {
				if (is_colliding(b, a)) {
					a.to_be_destroyed = true;
					b.to_be_destroyed = true;
					if (a.size != 1) {
						new_asteroids.push_back(
							generate_asteroid(a.size / 2, a.pos));
						new_asteroids.push_back(
							generate_asteroid(a.size / 2, a.pos));
					}
				}
			}
		std::ranges::move(new_asteroids, std::back_inserter(asteroids));
	}

	bool update()
	{
		p.update();
		wrap(p, dimensions);
		if (p.shoot &&
			shoot_cooldown_clock.getElapsedTime().asSeconds() > 0.2f) {

			bullets.push_back(Bullet{ { .pos = p.pos,
										.rot = p.rot,
										.speed = 5,
										.radius = 5,
										.color = sf::Color::Cyan } });
			shoot_cooldown_clock.restart();
		}
		std::ranges::for_each(bullets, [](Bullet& b) { b.update(); });
		auto [first, last] = std::ranges::remove_if(
			bullets, [dim = this->dimensions](Bullet& b) {
				auto [x, y] = b.pos;
				return b.to_be_destroyed || x < 0 || y < y || x > dim.x ||
					   y > dim.y;
			});
		bullets.erase(first, last);
		if (asteroids.size() < 10) {
			asteroids.push_back(generate_asteroid());
		}

		std::ranges::for_each(asteroids, [dim = this->dimensions](Asteroid& a) {
			a.update();
			wrap(a, dim);
		});
		auto [first_a, last_a] = std::ranges::remove_if(
			asteroids, std::identity{}, &Asteroid::to_be_destroyed);
		asteroids.erase(first_a, last_a);
		check_bullet_asteroid();
		return check_collisions();
	}
	void draw(sf::RenderTarget& rt)
	{
		std::ranges::for_each(bullets, [&rt](Bullet& b) { b.draw(rt); });
		std::ranges::for_each(asteroids, [&rt](Asteroid& a) { a.draw(rt); });
		p.draw(rt);
	}
};

int
main()
{
	const auto width = 1000;
	const auto height = 1000;
	auto window =
		sf::RenderWindow{ sf::VideoMode{ width, height }, "Assteroids" };
	window.setFramerateLimit(60);

	auto world = World{ .dimensions = { width, height },
						.p = Player{ { .pos = { width / 2, height / 2 },
									   .radius = 30,
									   .color = sf::Color::Red } } };
	sf::Clock points{};
	while (window.isOpen() && !world.update()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) window.close();
		}
		window.clear();

		auto& player = world.p;

		world.draw(window);
		window.display();
	}
	fmt::print("Score : {}", floor(points.getElapsedTime().asSeconds()));
	return 0;
}