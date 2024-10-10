#include "SDL.h"
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <chrono>
#include <string>
#include <iostream>

#pragma execution_character_set("utf-8")

std::string str = "Сейчас играет: Черниковская хата - Нажми на кнопку";
const char *c = str.c_str();

bool keepRunning = true;

const int windowWidth = 1600;
const int windowHeight = 1000;
SDL_Event event;

const int BallWidth = 15;
const int BallHeigth = 15;

const int PaddleWidth = 15;
const int PaddleHeight = 100;

const float Paddle_speed = 1.0f;
const float Ball_speed = 1.0f;

enum Buttons
{
	PaddleOneUp = 0,
	PaddleOneDown,
	PaddleTwoUp,
	PaddleTwoDown
};

enum MouseState
{
	CLIP_MOUSEOVER = 0,
	CLIP_MOUSEOUT = 1,
	CLIP_MOUSEDOWN = 2,
	CLIP_MOUSEUP = 3
};

enum class CollisionType
{
	None,
	Top,
	Middle,
	Bottom,
	Left,
	Right
};

struct Contact
{
	CollisionType type;
	float penetration;
};

class Vec2
{
public:
	Vec2() :x(0.0f), y(0.0f)
	{}
	Vec2(float x, float y) :x(x), y(y)
	{}
	Vec2 operator+(Vec2 const &rhs)
	{
		return Vec2(x + rhs.x, y + rhs.y);
	}
	Vec2& operator +=(Vec2 const &rhs)
	{
		x += rhs.x;
		y += rhs.y;

		return *this;
	}
	Vec2 operator*(float rhs)
	{
		return Vec2(x*rhs, y*rhs);
	}

	float x, y;
};

class Ball
{
public:
	Ball(Vec2 position, Vec2 velocity) :position(position), velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = BallWidth;
		rect.h = BallHeigth;
	}

	void Draw(SDL_Renderer *render)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(render, &rect);
	}

	void Update(float delta_time)
	{
		position += velocity * delta_time*10;
	}

	void CollideWithPaddle(Contact const &contact)
	{
		position.x += contact.penetration;
		velocity.x = -velocity.x;

		if (contact.type == CollisionType::Top)
		{
			velocity.y = -0.75f*Ball_speed;
		}
		else if (contact.type == CollisionType::Bottom)
		{
			velocity.y = 0.75*Ball_speed;
		}
	}

	void CollideWithWall(Contact const& contact)
	{
		if ((contact.type == CollisionType::Top) || (contact.type == CollisionType::Bottom))
		{
			position.y += contact.penetration;
			velocity.y = -velocity.y;
		}
		else if (contact.type == CollisionType::Left)
		{
			position.x = windowWidth / 2.0f;
			position.y = windowHeight / 2.0f;
			velocity.x = Ball_speed;
			velocity.y = 0.75*Ball_speed;
		}
		else if (contact.type == CollisionType::Right)
		{
			position.x = windowWidth / 2.0f;
			position.y = windowHeight / 2.0f;
			velocity.x = -Ball_speed;
			velocity.y = 0.75*Ball_speed;
		}
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect;
};

class Paddle
{
public:
	Paddle(Vec2 position, Vec2 velocity) : position(position), velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = PaddleWidth;
		rect.h = PaddleHeight;
	}

	void Draw(SDL_Renderer *render)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(render, &rect);
	}

	void Update(float delta_time)
	{
		position += velocity * delta_time*10;
		if (position.y < 0)
		{
			position.y = 0;
		}
		else if (position.y > (windowHeight - PaddleHeight))
		{
			position.y = windowHeight - PaddleHeight;
		}
	}

	Vec2 position;
	Vec2 velocity;
	SDL_Rect rect;
};

class PlayerScore
{
public:
	PlayerScore(Vec2 position, SDL_Renderer *render, TTF_Font *font) :render(render), font(font)
	{
		surface = TTF_RenderText_Solid(font, "0", { 0,128,0 });
		
		texture = SDL_CreateTextureFromSurface(render, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}
	

	~PlayerScore()
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);
	}

	void DrawScore()
	{
		SDL_RenderCopy(render, texture, nullptr, &rect);
	}

	void SetScore(int score)
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

		surface = TTF_RenderText_Solid(font, std::to_string(score).c_str(), { 0 ,128, 0 });
		texture = SDL_CreateTextureFromSurface(render, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.w = width;
		rect.h = height;
	
	}   

	SDL_Renderer *render;
	TTF_Font *font;
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_Rect rect;
};

class Hints
{
public:
	Hints(Vec2 position, SDL_Renderer *render, TTF_Font *font, const char* text) :render(render), font(font)
	{
		setlocale(LC_ALL, "Russian");

		surface = TTF_RenderUTF8_Blended(font, text, { 0xFF,128,0 });
		//surface = TTF_RenderText_Solid(font, (text), { 0,128,0 });
		texture = SDL_CreateTextureFromSurface(render, surface);
		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;

		
	}

	~Hints()
	{
		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

	}

	void DrawHint()
	{
		SDL_RenderCopy(render, texture, nullptr, &rect);
		
	}

	SDL_Renderer *render;
	TTF_Font *font;
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_Rect rect;
};


Contact CheckPaddleCollision(Ball const& ball, Paddle const& paddle)
{
	float ballleft = ball.position.x;
	float ballright = ball.position.x + BallWidth;
	float ballTop = ball.position.y;
	float ballBottom = ball.position.y + BallHeigth;

	float paddleleft = paddle.position.x;
	float paddleright = paddle.position.x + PaddleWidth;
	float paddleTop = paddle.position.y;
	float paddleBottom = paddle.position.y + PaddleHeight;

	Contact contact{};

	if (ballleft >= paddleright)
	{
		return contact;
	}
	if (ballright <= paddleleft)
	{
		return contact;
	}
	if (ballTop >= paddleBottom)
	{
		return contact;
	}
	if (ballBottom <= paddleTop)
	{
		return contact;
	}

	float paddleRangeUpper = paddleBottom - (2.0f*PaddleHeight / 3.0f);
	float paddleRangeMiddle = paddleBottom - (PaddleHeight / 3.0f);

	if (ball.velocity.x < 0)
	{
		contact.penetration = paddleright - ballleft;
	}
	else if (ball.velocity.x > 0)
	{
		contact.penetration = paddleleft - ballright;
	}
	if ((ballBottom > paddleTop) && (ballBottom < paddleRangeUpper))
	{
		contact.type = CollisionType::Top;
	}
	else if ((ballBottom > paddleRangeUpper) && (ballBottom < paddleRangeMiddle))
	{
		contact.type = CollisionType::Middle;
	}
	else
	{
		contact.type = CollisionType::Bottom;
	}

	return contact;
}

Contact checkWallCollision(Ball const& ball)
{
	Contact contact{}; 

	float ballLeft = ball.position.x;
	float ballRight = ball.position.x + BallWidth;
	float ballBotoom = ball.position.y;
	float ballTop = ball.position.y + BallHeigth;

	if (ballLeft < 0.0f)
	{
		contact.type = CollisionType::Left;
	}
	else if (ballRight > windowWidth)
	{
		contact.type = CollisionType::Right;
	}
	else if (ballTop < 0.0f)
	{
		contact.type = CollisionType::Top;
		contact.penetration = -ballTop;
	}
	else if (ballBotoom > windowHeight)
	{
		contact.type = CollisionType::Bottom;
		contact.penetration = windowHeight - ballBotoom;
	}

	return contact;
}

void startMenu()
{
	SDL_Window *menuWindow = SDL_CreateWindow("Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	SDL_Event menuEvent;
	SDL_Renderer *menuRender = SDL_CreateRenderer(menuWindow, 0, -1);

	bool menuStatus = true;
	while (menuStatus)
	{
		while (SDL_PollEvent(&menuEvent) != 0)
		{
			if (menuEvent.type == SDL_QUIT) {
				menuStatus = false;
				keepRunning = false;
			}
			switch (menuEvent.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				menuStatus = false;
				break;
			}
		}
	}

	SDL_DestroyWindow(menuWindow);
}

int main(int argc, char *argv[])
{

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	TTF_Init();
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 8, 16400);

	

	Mix_Music *music = Mix_LoadMUS("321.wav");

	SDL_Window *window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_Renderer *render = SDL_CreateRenderer(window, -1, 0);

	Ball ball(Vec2((windowWidth / 2.0f) - (BallWidth / 2.0f), (windowHeight / 2.0f) - (BallWidth / 2.0f)), Vec2(Ball_speed, 0.0f));

	Paddle leftPaddle(Vec2(25.0f, windowHeight / 2 - PaddleHeight / 2), Vec2(0.0f, 0.0f));
	Paddle rightPaddle(Vec2(windowWidth - 50.0f, windowHeight / 2 - PaddleHeight / 2), Vec2(0.0f, 0.0f));

	TTF_Font *scoreFont = TTF_OpenFont("DejaVuSansMono.ttf", 40);
	TTF_Font *hintFont = TTF_OpenFont("DejaVuSansMono.ttf", 20);
	TTF_Font *musicFont = TTF_OpenFont("DejaVuSansMono.ttf", 15);

	PlayerScore playerOneScore(Vec2(windowWidth / 4, 40), render, scoreFont);
	PlayerScore playerTwoScore(Vec2(3 * windowWidth / 4, 40), render, scoreFont);
	/*
	Hints hint1(Vec2(windowWidth/2 + 5, windowHeight - 100), render, hintFont, "To Resume Music press 2"); //To exit the game press ESC
	Hints hint2(Vec2(windowWidth / 2 + 5, windowHeight - 70), render, hintFont, "To exit the game press ESC");  //To pause music press 1
	Hints hint3(Vec2(windowWidth / 4 +35, windowHeight - 100), render, hintFont, "To pause music press 1"); //To Resume Music press 2
	Hints hint4(Vec2(windowWidth / 4 -55, windowHeight - 70), render, hintFont, "You need to get 5 points to win"); //You need to get 5 points to win
	Hints hint5(Vec2(windowWidth / 2 - 55, windowHeight - 40), render, hintFont, "GOOD LUCK!");
	*/
	Hints hint1(Vec2(15, windowHeight - 100), render, hintFont, "WASD для управления первым игроком"); //To exit the game press ESC
	Hints hint2(Vec2(15, windowHeight - 70), render, hintFont, "Стрелочки для управления вторым игроком");  //To pause music press 1
	Hints hint3(Vec2(windowWidth/2 + 100, windowHeight - 100), render, hintFont, "Чтобы поставить на паузу музыку нажмите 1"); //To Resume Music press 2
	Hints hint4(Vec2(windowWidth/2 + 100, windowHeight - 70), render, hintFont, "Чтобы возобновить музыку нажмите 2"); //You need to get 5 points to win
	
	Hints musicTheme(Vec2(windowWidth / 32 -25, 10), render, musicFont, c);

	Hints soonAvaliable(Vec2(windowWidth / 2 + 15, 10), render, musicFont, "Скоро будет добавлено меню с множеством песен и уровней!");

	int playerOneScoreNumber = 0;
	int playerTwoScoreNumber = 0;

	bool buttons[4] = {}; 
	

	float delta_time = 0.0f;

	Mix_PlayMusic(music, -1);
	while (keepRunning)
	{
		
		auto startTime = std::chrono::high_resolution_clock::now();
		while (SDL_PollEvent(&event) != 0)
		{

			if (event.type == SDL_QUIT)
			{
				keepRunning = false;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					keepRunning = false;
				}
			}
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
					buttons[Buttons::PaddleOneUp] = true;
					break;
				case SDLK_s:
					buttons[Buttons::PaddleOneDown] = true;
					break;
				case SDLK_UP:
					buttons[Buttons::PaddleTwoUp] = true;
					break;
				case SDLK_DOWN:
					buttons[Buttons::PaddleTwoDown] = true;
					break;
				}
			}
			if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
					buttons[Buttons::PaddleOneUp] = false;
					break;
				case SDLK_s:
					buttons[Buttons::PaddleOneDown] = false;
					break;
				case SDLK_UP:
					buttons[Buttons::PaddleTwoUp] = false;
					break;
				case SDLK_DOWN:
					buttons[Buttons::PaddleTwoDown] = false;
					break;
				}
			}

			if (buttons[Buttons::PaddleOneUp])
			{
				leftPaddle.velocity.y = -Paddle_speed;
			}
			else if (buttons[Buttons::PaddleOneDown])
			{
				leftPaddle.velocity.y = Paddle_speed;
			}
			else
			{
				leftPaddle.velocity.y = 0.0f;
			}

			if (buttons[Buttons::PaddleTwoUp])
			{
				rightPaddle.velocity.y = -Paddle_speed;
			}
			else if (buttons[Buttons::PaddleTwoDown])
			{
				rightPaddle.velocity.y = Paddle_speed;
			}
			else
			{
				rightPaddle.velocity.y = 0.0f;
			}

			switch (event.key.keysym.sym)
			{
			case SDLK_1:
				Mix_PauseMusic();
				break;
			case SDLK_2:
				Mix_ResumeMusic();
				break;
			}
		}

		SDL_SetRenderDrawColor(render, 0x0, 0x0, 0x0, 0xFF);
		SDL_RenderClear(render);

		SDL_SetRenderDrawColor(render, 0, 128, 0, 0xFF);

		for (int y = 0; y <= windowHeight; y++)
		{
			if (y % 5)
			{
				SDL_RenderDrawPoint(render, windowWidth / 2, y);
			}  
		}

		ball.Draw(render);
		leftPaddle.Draw(render);
		rightPaddle.Draw(render);

		playerOneScore.DrawScore();
		playerTwoScore.DrawScore();

		hint1.DrawHint();
		hint2.DrawHint();
		hint3.DrawHint();
		hint4.DrawHint();
		//hint5.DrawHint();

		musicTheme.DrawHint();

		soonAvaliable.DrawHint();

		leftPaddle.Update(delta_time);
		rightPaddle.Update(delta_time);
		ball.Update(delta_time);

		Contact contact1 = CheckPaddleCollision(ball, leftPaddle);
		if (contact1.type != CollisionType::None)
		{
			ball.CollideWithPaddle(contact1);
		}
		
		Contact contact2 = CheckPaddleCollision(ball, rightPaddle);
		if (contact2.type != CollisionType::None)
		{
			ball.CollideWithPaddle(contact2);
		}

		Contact contact3 = checkWallCollision(ball);
		{
			if (contact3.type != CollisionType::None)
			{
				ball.CollideWithWall(contact3);
			}
		}

		if (contact3.type == CollisionType::Left)
		{
			++playerTwoScoreNumber;
			playerTwoScore.SetScore(playerTwoScoreNumber);
		}
		else if (contact3.type == CollisionType::Right)
		{
			++playerOneScoreNumber;
			playerOneScore.SetScore(playerOneScoreNumber);
		}

		if (playerOneScoreNumber == 8 )
		{
			
			SDL_ShowSimpleMessageBox(0, "Message", "Первый игрок победил!", window);
			keepRunning = false;
		}
		else if(playerTwoScoreNumber == 8)
		{
			SDL_ShowSimpleMessageBox(0, "Message", "Второй игрок победил!", window);
			keepRunning = false;
		}

		auto stopTime = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration<float, std::chrono::milliseconds::period>(stopTime - startTime).count();

		SDL_RenderPresent(render);


	}
	
	
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(window);
	TTF_CloseFont(scoreFont);
	Mix_Quit();
	TTF_Quit();
	SDL_Quit();

	return 0;
}