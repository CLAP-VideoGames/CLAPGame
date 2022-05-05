﻿#include "TurnSystem.h"

#include <objects/Player.h>
#include <components/PlayerInfo.h>
#include <components/Controller.h>
#include <components/GameManager.h>
#include <components/CameraMovement.h>
#include <components/Indicator.h>

#include <ecs_prj/Entity.h>
#include <utils_prj/K_Map.h>
#include <render_prj/Camera.h>
#include <components_prj/Transform.h>

#include <iostream>

namespace K_Engine {
	//Required
	std::string TurnSystem::name = "TurnSystem";

	std::string TurnSystem::GetId() {
		return name;
	}

	TurnSystem::TurnSystem() : Component() {};

	TurnSystem::TurnSystem(Entity* e, bool firstStarts, float countDownTime): Component(e),
		firstTeamStarts(firstStarts), timeLimit(countDownTime)
	{
		player1 = new Player(0);
		player2 = new Player(1);
	}

	TurnSystem::~TurnSystem()
	{
		delete player1;
		delete player2;
	}

	void TurnSystem::init(K_Map* information)
	{
		firstTeamStarts = information->valueToBool("firstTeamStarts");
		timeLimit = information->valueToNumber("timeLimit");

		player1 = new Player(0);
		player2 = new Player(1);
	}

	void TurnSystem::start()
	{
		gMInstance = GameManager::GetInstance();

		startingZAxis = gMInstance->getRenderCamera()->getCameraPosition()[2];

		teamStarting = (firstTeamStarts) ? 0 : 1;
		player1Turn = player2Turn = 0;
		turn = Turn({ teamStarting, 0});
		countDown = timeLimit;
		timeStop = false;
		round = 0;
	}

	void TurnSystem::update(int deltaTime)
	{
		if (!hasEnded()) {
			if (firsTurn) {
				initTeamSize = player1->getTeamSize();
				setFocusOnPlayer();
				firsTurn = false;
			}

			if (!timeStop) {
				countDown -= (float)(deltaTime / 1000.0f);

				if (countDown <= 0.0f)
					endTurn();
			}

		}
		else {
			if(player1->getTeamSize() == 0)
				gMInstance->gameHasEnded(0);
			else
				gMInstance->gameHasEnded(1);
		}
	}

	void TurnSystem::resetCountdown()
	{
		countDown = timeLimit;
	}

	void TurnSystem::resumeCountdown()
	{
		timeStop = false;
	}

	void TurnSystem::stopCountdown()
	{
		timeStop = true;
	}

	void TurnSystem::RegisterMonkey(int team, int order, Entity* ent){
		if (team == 0)
			player1->addToTeam(ent, order);
		else
			player2->addToTeam(ent, order);

	}

	void TurnSystem::endTurn()
	{
		if (!end) {
		lostFocusOnPlayer();

			Player* p;
			turn.team = (turn.team + 1) % 2;
			p = (!turn.team) ? player1 : player2;	//Si es falso = 0 -> primer equipo

			////Siguiente jugador del equipo que corresponda
			if (turn.team == teamStarting) {
				//Comprobación del siguiente jugador
				nextPlayer();
			}
			std::cout << "\nTurno de Equipo: " << turn.team << " ;Jugador: " << turn.player << "\n";

			//Avanza una ronda si ha llegado al primer player del equipo que empezo
			if (turn.player == p->getOrder()[0] && turn.team == teamStarting) {
				round++;
				GameManager::GetInstance()->endRound();
				std::cout << "INICIA RONDA: " << round << "\n";
			}

			setFocusOnPlayer();

			resetCountdown();
		}
	}

	void TurnSystem::endTurnByWeapon()
	{
		countDown = 2.0f;
		resumeCountdown();
	}

	int TurnSystem::getRound()
	{
		return round;
	}

	void TurnSystem::killPlayer(Entity* e)
	{
		int t = e->getComponent<PlayerInfo>()->getTeam();
		int o = e->getComponent<PlayerInfo>()->getOrder();

		if (t == 0)
			player1->eraseFromTeam(e);
		else
			player2->eraseFromTeam(e);

		isEnded();
		std::cout << "Acuestate, Mono: "<<turn.player<<" del equipo "<< turn.team<< "\n" ;
		if(turn.team == t && turn.player == o && !hasEnded())
			endTurn();
	}

	void TurnSystem::nextPlayer()
	{
		//Player1
		std::cout << "\nAyuda1 " << player1Turn << "\n";
		/*if (player1->getTeamPlayer(player1Turn) != nullptr)
			player1Turn = (player1->getTeamPlayer(player1Turn)->getComponent<PlayerInfo>()->getOrder() + 1) % (player1->getOrder()[player1->getTeamSize() - 1] + 1);
		while (player1->getTeamPlayer(player1Turn) == nullptr) {
			player1Turn = (player1Turn + 1) % (player1->getOrder()[player1->getTeamSize() - 1] + 1);
			if (player1->getTeamPlayer(player1Turn) != nullptr)
			player1Turn = (player1->getTeamPlayer(player1Turn)->getComponent<PlayerInfo>()->getOrder());
		}*/
		std::vector<int>vectorOrder = player1->getOrder();
		std::cout << "\nTamano equipo 1: " << vectorOrder.size() << "\n";
		if (vectorOrder.size() > 1) {
			player1Turn++;
			if (player1Turn >= initTeamSize) player1Turn = 0;
			searchMonkey(player1, vectorOrder);
		}
		else {
			std::cout << "\n Yo no entiendo nada bro";
			player1Turn = vectorOrder[0];
		}

		std::cout << "\nAyudame1 " << player1Turn << "\n\n";
		turn.player = player1Turn;

		//Player2
		std::cout << "\nAyuda2 " << player2Turn << "\n";
		/*if (player2->getTeamPlayer(player2Turn) != nullptr)
			player2Turn = (player2->getTeamPlayer(player2Turn)->getComponent<PlayerInfo>()->getOrder() + 1) % (player2->getOrder()[player2->getTeamSize() - 1] + 1);
		while (player2->getTeamPlayer(player2Turn) == nullptr) {
			player2Turn = (player2Turn + 1) % (player2->getOrder()[player2->getTeamSize() - 1] + 1);
			if (player2->getTeamPlayer(player1Turn) != nullptr)
			player2Turn = (player2->getTeamPlayer(player2Turn)->getComponent<PlayerInfo>()->getOrder());
		}*/
		vectorOrder = player2->getOrder();
		std::cout << "\nTamano equipo 2: " << vectorOrder.size() << "\n";
		if (vectorOrder.size() > 1) {
			player2Turn++;
			if (player2Turn >= initTeamSize) player2Turn = 0;
			searchMonkey(player2, vectorOrder);
		}
		else {
			std::cout << "\n Yo no entiendo nada bro";
			player2Turn = vectorOrder[0];
		}

		turn.player = player2Turn;
		std::cout << "\nAyudame2 " << player2Turn << "\n\n";

	}

	void TurnSystem::setFocusOnPlayer()
	{
		//Activamos el controller del player
		Entity* e = nullptr;
		if (turn.team == 0)
			e = player1->getTeamPlayer(turn.player);
		else
			e = player2->getTeamPlayer(turn.player);

		if (e != nullptr) {
			e->getComponent<Controller>()->enable = true;
			//Posicion de la camara
			Vector3 pos = e->getComponent<Transform>()->getPosition();
			gMInstance->getCamera()->getComponent<CameraMovement>()->setLerpPosition(pos.x, pos.y, 150);
			Indicator* ind = e->addComponent<Indicator>();
			ind->create(24);
			std::cout << "\n Entendi bro fino";
		}
	}

	void TurnSystem::searchMonkey(Player* p, std::vector<int>& vectorOrder)
	{
		int pTurn = (p == player1) ? player1Turn : player2Turn;
		int i = 0;
		while (i < vectorOrder.size() && pTurn != vectorOrder[i]) i++;
		if (i < vectorOrder.size())
			p->getTeamPlayer(vectorOrder[i]);
		else {
			pTurn++;
			if (pTurn >= initTeamSize) pTurn = 0;

			if (p == player1)
				player1Turn = pTurn;
			else
				player2Turn = pTurn;

			searchMonkey(p, vectorOrder);
		}
	}
	
	void TurnSystem::lostFocusOnPlayer()
	{
		//Desactivamos el controller del player
		Entity* e = nullptr;
		if (turn.team == 0)
			e = player1->getTeamPlayer(turn.player);
		else
			e = player2->getTeamPlayer(turn.player);

		if (e != nullptr) {
			e->getComponent<Controller>()->enable = false;
			e->removeComponent<Indicator>();
		}
	}

	void TurnSystem::isEnded()
	{
		if (player1->getTeamSize() <= 0 || player2->getTeamSize() <= 0)
			end = true;
	}

	bool TurnSystem::hasEnded()
	{
		return end;
	}

}