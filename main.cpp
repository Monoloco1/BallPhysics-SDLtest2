#include <SDL2/SDL.h>
#include <algorithm>
#include <vector>
#include <math.h>
#include <iostream>
#include <thread>
using namespace std;

#define WINDOW_HEIGHT 600
#define WINDOW_WIDTH 800
#define FRAMERATE 60
#define COOL_BLEND 200                  //0-255
#define GRAB_RADIUS 30
#define MAX_ENTITIES 1024
//#define PRINT_SUM_VELOCITIES
//#define DISPLAY_SPRING_REACTION
//#define DISPLAY_VELOCITY
//#define DISPLAY_DAMPER_REACTION
#define AIR_RESISTANCE 0              //0-1; ~v^2
#define BOUNCE_FROM_BORDER
//#define EXACT_BOUNCE_FROM_BORDER
#define BOUNCE_FROM_BORDER_ENERGY 1   //0-1
#define BOUNCE_FROM_BALL_ENERGY .5     //0-1
SDL_Window *window;
SDL_Renderer *renderer;

void rect(int x, int y, int w, int h);
void circle(int x, int y, int r);
void circleFill(int x, int y, int r);
void line(int x1, int y1, int x2, int y2);


template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

struct ball {
	float x, y;
	float vx,vy;
	float r, m;
};
void drawBalls(unsigned int i_, unsigned int iend, vector<ball> *balls);
ball *selectedBall1=NULL, *selectedBall2=NULL, *movedBall=NULL, *hoveredBall=NULL, *selectedBall1d=NULL, *selectedBall2d=NULL;
ball ballGen(float x, float y, float vx, float vy) {
	ball ballTemp;
	ballTemp.x=x;
	ballTemp.y=y;
	ballTemp.vx=vx;
	ballTemp.vy=vy;
	ballTemp.r=GRAB_RADIUS;
	ballTemp.m=1;
	return ballTemp;
}
struct spring {
	ball* ball1, *ball2;
	float len, k;
};
struct damper {
	ball* ball1, *ball2;
	float k;
};
spring springGen(ball* ball1, ball* ball2, float len) {
	spring springTemp;
	springTemp.ball1 = ball1;
	springTemp.ball2 = ball2;
	springTemp.len=len;
	springTemp.k=1;
	return springTemp;
}
damper damperGen(ball* ball1, ball* ball2) {
	damper damperTemp;
	damperTemp.ball1 = ball1;
	damperTemp.ball2 = ball2;
	damperTemp.k=.01;
	return damperTemp;
}
float dist(float x1, float y1, float x2, float y2) {
	return sqrt(pow(x1-x2, 2)+pow(y1-y2, 2));
}
float deltaV(float vx1, float vy1, float vx2, float vy2) {
	float dvx=vx1-vx2;
	float dvy=vy1-vy2;
	return (dvx*abs(dvx)+dvy*abs(dvy));
}
struct ballAndDistance {
	ball *ball1;
	float dist1;
};
ballAndDistance closestBallAndDistance(int x, int y, vector<ball> *balls) {

	ballAndDistance banddTemp;
	//if(!balls->empty()) {
	banddTemp.dist1 = dist(balls->at(0).x, balls->at(0).y, x, y);
	banddTemp.ball1 = &balls->at(0);
	for(unsigned int i=1; i<balls->size(); i++) {
		if(dist(balls->at(i).x, balls->at(i).y, x, y) < banddTemp.dist1) {
			banddTemp.dist1 = dist(balls->at(i).x, balls->at(i).y, x, y);
			banddTemp.ball1 = &balls->at(i);
		}
	}
	//}
	return banddTemp;
}

int main(int argv, char** args) {
	SDL_CreateWindowAndRenderer(800, 600, SDL_WINDOW_SHOWN | SDL_RENDERER_PRESENTVSYNC, &window, &renderer);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	uint32_t ticksStart;
	int  mx, my;
	bool mlpressed=0, mrpressed=0, mmpressed=0, mlpressedbefore=0, mrpressedbefore=0, mmpressedbefore=0, mlclick=0, mrclick=0, mmclick=0;
	bool m4pressed=0, m5pressed=0;
	//ball *selectedBall1=NULL, *selectedBall2=NULL, *movedBall=NULL, *hoveredBall=NULL, *selectedBall1d=NULL, *selectedBall2d=NULL;
	vector<ball> balls;
	balls.reserve(MAX_ENTITIES);
	vector<spring> springs;
	springs.reserve(MAX_ENTITIES);
	vector<damper> dampers;
	dampers.reserve(MAX_ENTITIES);

	//balls.push_back(ballGen(100, 200, 0, 0));
	//balls.push_back(ballGen(150, 300, 0, 0));
	//springs.push_back( springGen(&balls[0], &balls[1]) );
	//dampers.push_back( damperGen(&balls[0], &balls[1]) );
	SDL_Event event;
	bool quit=0;

	while(!quit) {
		ticksStart=SDL_GetTicks();
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_QUIT:
				quit=1;
				break;
			case SDL_MOUSEMOTION :
				if(event.button.state==SDL_PRESSED) {
					if(event.button.button==SDL_BUTTON_LEFT) {
						mlpressed=1;
					}
					if(event.button.button==SDL_BUTTON_RIGHT) {
						mrpressed=1;
					}
					if(event.button.button==SDL_BUTTON_MIDDLE) {
						mmpressed=1;
					}
					if(event.button.button==SDL_BUTTON_LEFT) {
						mlpressed=1;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(event.button.button==SDL_BUTTON_LEFT) {
					mlpressed=1;
				}
				if(event.button.button==SDL_BUTTON_RIGHT) {
					mrpressed=1;
				}
				if(event.button.button==SDL_BUTTON_MIDDLE) {
					mmpressed=1;
				}
				break;
			default:
				mlpressed=0;
				mrpressed=0;
				mmpressed=0;
				break;
			}
			//add more event types here as you see fit
		}

		if(!mlpressed) {
			mlclick=0;
			mlpressedbefore=0;
		}
		else if(!mlpressedbefore) {
			mlclick=1;
			mlpressedbefore=1;
		}
		else mlclick=0;

		if(!mrpressed) {
			mrclick=0;
			mrpressedbefore=0;
		}
		else if(!mrpressedbefore) {
			mrclick=1;
			mrpressedbefore=1;
		}
		else mrclick=0;

		if(!mmpressed) {
			mmclick=0;
			mmpressedbefore=0;
		}
		else if(!mmpressedbefore) {
			mmclick=1;
			mmpressedbefore=1;
		}
		else mmclick=0;

		SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);

#ifndef COOL_BLEND
		SDL_SetRenderDrawColor(renderer, 50, 0, 0, 255);
		SDL_RenderClear(renderer);
#else
		SDL_Rect rect1;
		rect1.x=0;
		rect1.y=0;
		rect1.w=WINDOW_WIDTH;
		rect1.h=WINDOW_HEIGHT;
		SDL_SetRenderDrawColor(renderer, 200, 200, 200, COOL_BLEND);
		SDL_RenderFillRect(renderer, &rect1);
#endif
		//SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		if(!balls.empty()) {
			ballAndDistance BDTemp = closestBallAndDistance(mx, my, &balls);
			if(BDTemp.dist1<=GRAB_RADIUS) {
				if(mlclick) movedBall=BDTemp.ball1;
				else hoveredBall=BDTemp.ball1;
			}
			else {
				hoveredBall=NULL;
				if(mlclick) {
					if(balls.capacity() > balls.size()+1) balls.push_back(ballGen(mx, my, 0, 0));
					else cout << "Max ball amount reached\n";
				}
			}

			if(mlpressed) {
				hoveredBall=NULL;
				if(movedBall!=NULL) {
					movedBall->vx+=.2*(float(mx)-movedBall->x);
					movedBall->vy+=.2*(float(my)-movedBall->y);
					movedBall->vx*=.9;
					movedBall->vy*=.9;
				}
			}
			else movedBall=NULL;
			if(!mlpressed) {
				if(!mrclick) {
					if(selectedBall1!=NULL) {
						SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
						line(selectedBall1->x, selectedBall1->y, mx, my);
					}
				}
				else if(selectedBall1!=NULL) {                                      //select second ball
					ballAndDistance closestBDTemp=closestBallAndDistance(mx, my, &balls);
					if(closestBDTemp.ball1!=selectedBall1 && closestBDTemp.dist1<=GRAB_RADIUS) {            //all balls selected
						selectedBall2=closestBDTemp.ball1;
						springs.push_back(springGen(selectedBall1, selectedBall2, dist(selectedBall1->x, selectedBall1->y, selectedBall2->x, selectedBall2->y)));
						selectedBall1=NULL;
						selectedBall2=NULL;
					}
				}
				else selectedBall1=closestBallAndDistance(mx, my, &balls).ball1;    //select first ball

				if(!mmclick) {
					if(selectedBall1d!=NULL) {
						SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
						line(selectedBall1d->x, selectedBall1d->y, mx, my);
					}
				}
				else if(selectedBall1d!=NULL) {                                      //select second ball
					ballAndDistance closestBDTemp=closestBallAndDistance(mx, my, &balls);
					if(closestBDTemp.ball1!=selectedBall1d && closestBDTemp.dist1<=GRAB_RADIUS) {            //all balls selected
						selectedBall2d=closestBDTemp.ball1;
						dampers.push_back(damperGen(selectedBall1d, selectedBall2d));
						selectedBall1d=NULL;
						selectedBall2d=NULL;
					}
				}
				else selectedBall1d=closestBallAndDistance(mx, my, &balls).ball1;    //select first ball
			}

			for(unsigned int i=0; i<balls.size()-1; i++) {  //ball bounce
				for(unsigned int j=i+1; j<balls.size(); j++) {

					float distBalls = dist(balls[i].x, balls[i].y, balls[j].x, balls[j].y);
					if( distBalls < (balls[i].r+balls[j].r) ) {
						//cout << "IMPACT\n";
						float angle=atan2(balls[i].y-balls[j].y, balls[i].x-balls[j].x);
						float normalMomentumI=BOUNCE_FROM_BALL_ENERGY*balls[i].m*(cos(angle)*balls[i].vx+sin(angle)*balls[i].vy);
						float normalMomentumJ=BOUNCE_FROM_BALL_ENERGY*balls[j].m*(cos(angle)*balls[j].vx+sin(angle)*balls[j].vy);
						balls[i].vx+=normalMomentumJ*cos(angle)/balls[i].m-normalMomentumI*cos(angle)/balls[i].m;
						balls[i].vy+=normalMomentumJ*sin(angle)/balls[i].m-normalMomentumI*sin(angle)/balls[i].m;
						balls[j].vx+=normalMomentumI*cos(angle)/balls[j].m-normalMomentumJ*cos(angle)/balls[j].m;
						balls[j].vy+=normalMomentumI*sin(angle)/balls[j].m-normalMomentumJ*sin(angle)/balls[j].m;
						distBalls = (balls[i].r+balls[j].r-distBalls)/2;
						distBalls = max(distBalls, 0.0001f);
						balls[i].x+=cos(angle)*distBalls;
						balls[i].y+=sin(angle)*distBalls;
						balls[j].x-=cos(angle)*distBalls;
						balls[j].y-=sin(angle)*distBalls;
					}
				}
			}
			for(unsigned int i=0; i<springs.size(); i++) {  //spring action
				float angle=atan2(springs[i].ball1->y-springs[i].ball2->y, springs[i].ball1->x-springs[i].ball2->x);
				float force=springs[i].k*(springs[i].len-dist(springs[i].ball1->x, springs[i].ball1->y, springs[i].ball2->x, springs[i].ball2->y))/springs[i].len;
#ifdef DISPLAY_SPRING_REACTION
				line(springs[i].ball1->x, springs[i].ball1->y, springs[i].ball1->x+sgn(force)*30*cos(angle), springs[i].ball1->y+sgn(force)*30*sin(angle));
				line(springs[i].ball2->x, springs[i].ball2->y, springs[i].ball2->x-sgn(force)*30*cos(angle), springs[i].ball2->y-sgn(force)*30*sin(angle));
#endif
				springs[i].ball1->vx+=force*cos(angle);
				springs[i].ball1->vy+=force*sin(angle);
				springs[i].ball2->vx-=force*cos(angle);
				springs[i].ball2->vy-=force*sin(angle);
			}

			for(unsigned int i=0; i<dampers.size(); i++) {  //damper action
				float angle=atan2(dampers[i].ball1->y-dampers[i].ball2->y, dampers[i].ball1->x-dampers[i].ball2->x);
				//float force=dampers[i].k*deltaV(dampers[i].ball1->vx, dampers[i].ball1->vy, dampers[i].ball2->vx, dampers[i].ball2->vy);
				float dvx=dampers[i].ball1->vx-dampers[i].ball2->vx;
				float dvy=dampers[i].ball1->vy-dampers[i].ball2->vy;
				float force=dampers[i].k*(-cos(angle)*dvx-sin(angle)*dvy);
#ifdef DISPLAY_DAMPER_REACTION
				SDL_SetRenderDrawColor(renderer, 255, 128, 128, 255);
				line(dampers[i].ball1->x, dampers[i].ball1->y, dampers[i].ball1->x+force*300*cos(angle), dampers[i].ball1->y+force*300*sin(angle));
				line(dampers[i].ball2->x, dampers[i].ball2->y, dampers[i].ball2->x-force*300*cos(angle), dampers[i].ball2->y-force*300*sin(angle));
#endif
				dampers[i].ball1->vx+=force*cos(angle);
				dampers[i].ball1->vy+=force*sin(angle);
				dampers[i].ball2->vx-=force*cos(angle);
				dampers[i].ball2->vy-=force*sin(angle);
			}
#ifdef PRINT_SUM_VELOCITIES
			float sumVelocity=0;
			float sumEnergy=0;
#endif // PRINT_SUM_VELOCITIES

      drawBalls(0, balls.size(), &balls);
//      if(balls.size()>1) {//balls.size()>1) {
//        thread b1(drawBalls, 0, balls.size()/2, &balls);
//        b1.join();
//        thread b2(drawBalls, balls.size()/2, balls.size(), &balls);
//        b2.join();
//      }
//      else {
//        thread b1(drawBalls, 0, balls.size(), &balls);
//        b1.join();
//      }
			for(unsigned int i=0; i<springs.size(); i++) {  //draw springs
				SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
				line(springs[i].ball1->x, springs[i].ball1->y, springs[i].ball2->x, springs[i].ball2->y);
			}
			for(unsigned int i=0; i<dampers.size(); i++) {  //draw dampers
				SDL_SetRenderDrawColor(renderer, 00, 200, 0, 255);
				line(dampers[i].ball1->x, dampers[i].ball1->y, dampers[i].ball2->x, dampers[i].ball2->y);
			}
		} //if(!balls.empty())

		else if(mlclick) {
			if(balls.capacity() > balls.size()+1) balls.push_back(ballGen(mx, my, 0, 0));
			else cout << "Max ball amount reached\n";
		}
		SDL_RenderPresent(renderer);
		if(SDL_GetTicks() - ticksStart<1000/FRAMERATE) SDL_Delay(1000/FRAMERATE - SDL_GetTicks() + ticksStart);
	}
	return 0;
}

void drawBalls(unsigned int i_, unsigned int iend, vector<ball> *balls) {
	for(unsigned int i=i_; i<iend; i++) {    //draw balls
    //cout << "Ball nr: " << i << "\n";
#ifdef BOUNCE_FROM_BORDER
		if(balls->at(i).x+balls->at(i).r>WINDOW_WIDTH) {
			balls->at(i).vx=-BOUNCE_FROM_BORDER_ENERGY*balls->at(i).vx;
			balls->at(i).vy*=BOUNCE_FROM_BORDER_ENERGY;
			balls->at(i).x=WINDOW_WIDTH-balls->at(i).r;
		}
		if(balls->at(i).x<balls->at(i).r) {
			balls->at(i).vx=-BOUNCE_FROM_BORDER_ENERGY*balls->at(i).vx;
			balls->at(i).vy*=BOUNCE_FROM_BORDER_ENERGY;
			balls->at(i).x=balls->at(i).r;
		}
		if(balls->at(i).y+balls->at(i).r>WINDOW_HEIGHT) {
			balls->at(i).vy=-BOUNCE_FROM_BORDER_ENERGY*balls->at(i).vy;
			balls->at(i).vx*=BOUNCE_FROM_BORDER_ENERGY;
			balls->at(i).y=WINDOW_HEIGHT-balls->at(i).r;
		}
		if(balls->at(i).y<balls->at(i).r) {
			balls->at(i).vy=-BOUNCE_FROM_BORDER_ENERGY*balls->at(i).vy;
			balls->at(i).vx*=BOUNCE_FROM_BORDER_ENERGY;
			balls->at(i).y=balls->at(i).r;
		}
#endif // BOUNCE_FROM_BORDER
#ifdef EXACT_BOUNCE_FROM_BORDER
		if(balls->at(i).x>WINDOW_WIDTH) {
			if() balls->at(i).vx=-BOUNCE_FROM_BORDER_ENERGY*balls->at(i).vx;
		}
#endif // EXACT_BOUNCE_FROM_BORDER
		balls->at(i).x+=balls->at(i).vx;
		balls->at(i).y+=balls->at(i).vy;
#ifdef DISPLAY_VELOCITY
		line(balls->at(i).x, balls->at(i).y, balls->at(i).x+10*balls->at(i).vx, balls->at(i).y+10*balls->at(i).vy);
#endif
		balls->at(i).vx-=AIR_RESISTANCE*balls->at(i).vx*abs(balls->at(i).vx);
		balls->at(i).vy-=AIR_RESISTANCE*balls->at(i).vy*abs(balls->at(i).vy);
		//if(&balls[i] == movedBall) SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		//else
		if(&(balls->at(i)) == hoveredBall) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		else if(&(balls->at(i)) == selectedBall1d) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		else if(&(balls->at(i)) == selectedBall1) SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		else {
			uintmax_t modI=(i)%6;
			SDL_SetRenderDrawColor(renderer, 255*(modI==0||modI==3||modI==4), 255*((modI)%2), 255*( (modI==2)||(modI==4)||(modI==5) ), 255);
		}
		circle(balls->at(i).x, balls->at(i).y, balls->at(i).r);
		circle(balls->at(i).x, balls->at(i).y, balls->at(i).r-1);
		circle(balls->at(i).x, balls->at(i).y, balls->at(i).r-2);
#ifdef PRINT_SUM_VELOCITIES
		sumVelocity+=sqrt(pow(balls->at(i).vx,2) + pow(balls->at(i).vy,2));
		sumEnergy+=balls->at(i).m*(pow(balls->at(i).vx,2) + pow(balls->at(i).vy,2))/2;
		if(i==balls.size()-1) {
			cout << "Sum of velocity:\t" << sumVelocity;
			cout << "\tSum of energy: " << sumEnergy << "\n";
		}
#endif // PRINT_SUM_VELOCITIES
	}
}

void rect(int x, int y, int w, int h) {
	SDL_Rect rect1;
	rect1.x=x;
	rect1.y=y;
	rect1.w=w;
	rect1.h=h;
	SDL_RenderDrawRect(renderer, &rect1);
}
void line(int x1, int y1, int x2, int y2) {
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
void circle(int x, int y, int r) {
	int dx=r, dy=0, rsq=r*r;
	while(dx>=dy) {
		SDL_RenderDrawPoint(renderer, x+dx, y+dy);
		SDL_RenderDrawPoint(renderer, x-dy, y+dx);
		SDL_RenderDrawPoint(renderer, x-dx, y-dy);
		SDL_RenderDrawPoint(renderer, x+dy, y-dx);
		SDL_RenderDrawPoint(renderer, x+dx, y-dy);
		SDL_RenderDrawPoint(renderer, x-dy, y-dx);
		SDL_RenderDrawPoint(renderer, x-dx, y+dy);
		SDL_RenderDrawPoint(renderer, x+dy, y+dx);
		if( (pow(dx-1, 2)+pow(dy, 2) - rsq ) >= 0) dx--;
		else dy++;
	}
}
void circleFill(int x, int y, int r) {
	int dx, dy=0, rsq=r*r, dxmax=r;
	SDL_RenderDrawPoint(renderer, x, y);
	for(uintmax_t yy=0; yy<dxmax; yy++) {
		//cout<<"\tyy Loop\n";
		if(dxmax*dxmax + yy*yy > rsq) dxmax--;
		dx=dxmax;
		while(1) {
			SDL_RenderDrawPoint(renderer, x+dx, y+yy);
			SDL_RenderDrawPoint(renderer, x-yy, y+dx);
			SDL_RenderDrawPoint(renderer, x-dx, y-yy);
			SDL_RenderDrawPoint(renderer, x+yy, y-dx);

			SDL_RenderDrawPoint(renderer, x+dx, y-yy);
			SDL_RenderDrawPoint(renderer, x-yy, y-dx);
			SDL_RenderDrawPoint(renderer, x-dx, y+yy);
			SDL_RenderDrawPoint(renderer, x+yy, y+dx);
			if(dx-1<yy || dx==1) break;
			else dx--;
		}
	}
}
